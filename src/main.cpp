#include <csignal>
#include <atomic>
#include <format>
#include <print>
#include <string_view>
#include <thread>
#include <chrono>

#include "mynet/make_srvsock.hpp"
#include "mynet/handles.hpp"
#include "myapp/response_helpers.hpp"
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

std::atomic_flag is_running = ATOMIC_FLAG_INIT;

void handle_sigint([[maybe_unused]] int sig_id) {
    is_running.clear();
    is_running.notify_all();
}


constexpr std::string_view server_hostname {"localhost"};


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

        return { .fd = -1, .events = {}, .revents = {} };
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

    is_running.test_and_set();

    signal(SIGINT, handle_sigint);

    std::string_view port_arg {argv[1]};
    auto checked_backlog = ([](char* argv[]) noexcept -> std::optional<int> {
        try {
            return {std::stoi(argv[2])};
        } catch (const std::invalid_argument& arg_err) {
            return {};
        } catch (const std::out_of_range& repr_err) {
            return {};
        }
    })(argv);

    if (!checked_backlog) {
        std::println("Setup ERR: invalid backlog count!");
        return 1;
    }

    const auto backlog_value = checked_backlog.value();
    App::Routes my_routes {server_hostname, port_arg};

    my_routes.set_handler("/", [](Http::Request req, [[maybe_unused]] const std::map<std::string, Uri::QueryValue>& query_params) {
        Http::Response res;

        if (const auto method = req.http_verb; method == Http::Verb::http_get) {
            if (auto file_opt = App::TextualFile::create("./www/index.html", "text/html", 512); file_opt) {
                App::ResponseUtils::response_put_all(res, file_opt.value(), Http::Status::http_ok);

                return res;
            }
        } else if (method == Http::Verb::http_post) {
            App::StringReply cat_msg {std::move(req.body), "text/plain"};
            App::ResponseUtils::response_put_all(res, cat_msg, Http::Status::http_ok);

            return res;
        } else {
            auto invalid_method_err = App::EmptyReply{Http::Status::http_method_not_allowed};

            App::ResponseUtils::response_put_all(res, invalid_method_err);

            return res;
        }

        auto internal_err = App::EmptyReply {Http::Status::http_server_error};

        App::ResponseUtils::response_put_all(res, internal_err);

        return res;
    });

    my_routes.set_handler("/index.js", [](Http::Request req, [[maybe_unused]] const std::map<std::string, Uri::QueryValue>& query_params) {
        Http::Response res;

        if (req.http_verb == Http::Verb::http_get) {
            if (auto file_opt = App::TextualFile::create("./www/index.js", "text/javascript", 512); file_opt) {
                App::ResponseUtils::response_put_all(res, file_opt.value(), Http::Status::http_ok);

                return res;
            }
        } else {
            auto invalid_method_err = App::EmptyReply {Http::Status::http_method_not_allowed};

            App::ResponseUtils::response_put_all(res, invalid_method_err);
        }

        auto internal_err = App::EmptyReply {Http::Status::http_server_error};

        App::ResponseUtils::response_put_all(res, internal_err);

        return res;
    });

    const auto serviced_ok = run_server(port_arg, backlog_value, my_routes);

    return MainReturn {
        (serviced_ok)
        ? ExitCode::ok
        : ExitCode::failure
    };
}
