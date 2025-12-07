#include <csignal>
#include <atomic>
#include <print>
#include <string_view>
#include <thread>
#include <chrono>

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

auto run_server(std::string_view port_sv, int backlog) -> bool {
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
        if (auto sweep_res = fd_pool.dispatch_active_fds(io_worker_fn, Net::PollEvent::hangup, Net::PollEvent::received); !sweep_res.has_value()) {
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

    return MainReturn {
        (run_server(argv[1], checked_backlog.value_or(1)))
        ? ExitCode::ok
        : ExitCode::failure
    };
}
