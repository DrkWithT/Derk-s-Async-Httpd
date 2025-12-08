#include <csignal>
#include <atomic>
#include <print>
#include <fstream>
#include <string_view>
#include <thread>
#include <chrono>
#include <utility>

#include "mynet/make_srvsock.hpp"
#include "mynet/handles.hpp"
#include "myapp/msg_task.hpp"

enum class ExitCode {
    ok = 0,
    failure = 1,
};

class MainReturn {
private:
    [[maybe_unused]]
    ExitCode code;

public:
    constexpr MainReturn(ExitCode e) noexcept
    : code {e} {}

    operator int(this auto&& self) noexcept {
        return static_cast<int>(self.code);
    }
};

std::atomic_flag is_running {true};

void handle_sigint([[maybe_unused]] int sig_id) {
    is_running.clear();
    is_running.notify_all();
}

[[nodiscard]] auto read_file_as_blob(const std::string& path) -> DerkHttpd::Http::Blob {
    using namespace DerkHttpd;
    
    std::ifstream freader {path};

    if (!freader.is_open()) {
        return {};
    }

    Http::Blob contents;
    std::string temp_line;

    while (std::getline(freader, temp_line)) {
        contents.append_range(temp_line);
        contents.emplace_back('\n');
    }

    return contents;
}

[[nodiscard]] auto run_server(std::string_view port_sv, int backlog, const DerkHttpd::App::Routes& app_router) -> bool {
    using namespace DerkHttpd;

    constexpr auto recv_backoff_ms = 20;

    Net::CreateServerSocket listener_generator {port_sv, backlog, Net::PollEvent::hangup, Net::PollEvent::received};

    auto listener_pollfd = ([&listener_generator]() -> pollfd {
        while (true) {
            if (auto host_opt = listener_generator(); host_opt.has_value()) {
                if (auto temp_pollfd = *host_opt; temp_pollfd.fd != -1) {
                    return temp_pollfd;
                }
            } else {
                break;
            }
        }

        return { .fd = -1 };
    })();

    if (listener_pollfd.fd == -1) {
        std::println(std::cerr, "Startup ERR: failed to launch server- invalid listener pollfd created.");
        return false;
    }

    App::MsgExchangeTask<Net::IOTaskResult> io_worker_fn;
    Net::Handles fd_pool {listener_pollfd};

    while (is_running.test()) {
        if (auto sweep_res = fd_pool.dispatch_active_fds(io_worker_fn, app_router, Net::PollEvent::hangup, Net::PollEvent::received); !sweep_res.has_value()) {
            std::println(std::cerr, "Event Loop ERR:\n{}", sweep_res.error());
            break;
        } else if (const auto event_count = sweep_res.value(); event_count == 0) {
            // Sleep for inactive I/O periods, saving CPU cycles for other processes.
            std::this_thread::sleep_for(std::chrono::milliseconds {recv_backoff_ms});
        }
    }

    std::println(std::cout, "Event Loop LOG: Shutdown!");

    return true;
}

int main(int argc, char* argv[]) {
    using namespace DerkHttpd;

    if (argc != 3) {
        std::println(std::cerr, "usage: ./server <port> <backlog>");
        return 1;
    }

    signal(SIGINT, handle_sigint);

    auto checked_backlog = ([](char* argv[]) noexcept -> std::optional<int> {
        try {
            return {std::stoi(argv[2])};
        } catch (const std::invalid_argument& arg_err) {
            return {};
        } catch (const std::out_of_range& repr_err) {
            return {};
        }
    })(argv);

    App::Routes my_routes;

    my_routes.set_handler("/", [](const Http::Request& req, [[maybe_unused]] const std::map<std::string, Uri::QueryValue>& query_params) {
        Http::Response res;

        if (const auto method = req.http_verb; method == Http::Verb::http_get) {
            // GET case:
            auto page_blob = read_file_as_blob("./www/index.html");

            res.headers.emplace("Content-Length", std::to_string(page_blob.size()));
            res.headers.emplace("Content-Type", "text/html");

            res.body = std::move(page_blob);
            res.http_status = Http::Status::http_ok;
        } else if (method == Http::Verb::http_post) {
            // POST case:
            res.headers.emplace("Content-Length", std::to_string(req.body.size()));
            res.headers.emplace("Content-Type", "text/plain");

            res.body = req.body;
            res.http_status = Http::Status::http_ok;
        } else {
            res.headers.emplace("Content-Length", "0");
            res.headers.emplace("Content-Type", "*/*");

            res.body = {};
            res.http_status = Http::Status::http_method_not_allowed;
        }

        return res;
    });

    my_routes.set_handler("/index.js", [](const Http::Request& req, [[maybe_unused]] const std::map<std::string, Uri::QueryValue>& query_params) {
        Http::Response res;

        if (req.http_verb != Http::Verb::http_get) {
            res.headers.emplace("Content-Length", "0");
            res.headers.emplace("Content-Type", "*/*");

            res.body = {};

            res.http_status = Http::Status::http_method_not_allowed;
        } else if (auto page_blob = read_file_as_blob("./www/index.js"); !page_blob.empty()) {
            const auto page_size = page_blob.size();

            res.headers.emplace("Content-Length", std::to_string(page_size));
            res.headers.emplace("Content-Type", "text/javascript");

            res.body = std::move(page_blob);

            res.http_status = Http::Status::http_ok;
        } else {
            Http::Blob file_error_msg;
            file_error_msg.append_range(std::string {"Failed to read asset."});

            res.headers.emplace("Content-Length", std::to_string(file_error_msg.size()));
            res.headers.emplace("Content-Type", "text/plain");

            res.body = {};

            res.http_status = Http::Status::http_not_found;
        }

        return res;
    });

    const auto serviced_ok = run_server(argv[1], checked_backlog.value_or(1), my_routes);

    return MainReturn {
        (serviced_ok)
        ? ExitCode::ok
        : ExitCode::failure
    };
}
