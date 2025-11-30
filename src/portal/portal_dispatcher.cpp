/**
 * @file portal_dispatcher.cpp
 * @author Ruan Formigoni
 * @brief Dispatches child process requests to the portal daemon
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <expected>
#include <fcntl.h>
#include <filesystem>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../macro.hpp"
#include "../std/expected.hpp"
#include "../std/filesystem.hpp"
#include "../lib/env.hpp"
#include "../lib/log.hpp"
#include "../lib/linux.hpp"
#include "../lib/linux/fifo.hpp"
#include "../lib/linux/fd.hpp"
#include "../db/portal/message.hpp"
#include "../db/portal/dispatcher.hpp"
#include "config.hpp"

namespace fs = std::filesystem;
namespace ns_message = ns_db::ns_portal::ns_message;
namespace ns_dispatcher = ns_db::ns_portal::ns_dispatcher;

extern char** environ;
std::optional<pid_t> opt_child = std::nullopt;

/**
 * @brief Forwards received signal to requested child process
 *
 * @param sig The signal to forward
 */
void signal_handler(int sig)
{
  if(opt_child)
  {
    kill(opt_child.value(), sig);
  }
}

/**
 * @brief Registers signal handlers for dispatcher cleanup
 *
 * Registers handlers for various signals (SIGINT, SIGTERM, etc.) to ensure
 * proper cleanup when the dispatcher receives termination signals.
 */
void register_signals()
{
  signal(SIGABRT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGCONT, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGIO, signal_handler);
  signal(SIGIOT, signal_handler);
  signal(SIGPIPE, signal_handler);
  signal(SIGPOLL, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGURG, signal_handler);
  signal(SIGUSR1, signal_handler);
  signal(SIGUSR2, signal_handler);
  signal(SIGVTALRM, signal_handler);
}

/**
 * @brief Creates FIFOs for child process I/O
 *
 * @param msg Message containing FIFO paths for stdin, stdout, and stderr
 * @return Value<void> Success or error
 */
[[nodiscard]] Value<void> fifo_create(ns_message::Message const& msg)
{
  Pop(ns_linux::ns_fifo::create(msg.get_stdin()));
  Pop(ns_linux::ns_fifo::create(msg.get_stdout()));
  Pop(ns_linux::ns_fifo::create(msg.get_stderr()));
  Pop(ns_linux::ns_fifo::create(msg.get_exit()));
  Pop(ns_linux::ns_fifo::create(msg.get_pid()));
  return {};
}

/**
 * @brief Sends a message to the portal daemon
 *
 * @param path_fifo_daemon Path to the fifo which the daemon receives commands from
 * @param message The message to send
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] Value<void> send_message(ns_message::Message const& message, fs::path const& path_fifo_daemon)
{
  logger("D::Sending message through pipe: {}", path_fifo_daemon);
  // Serialize to json string
  std::string data = Pop(ns_message::serialize(message));
  logger("D::{}", data);
  // Write to fifo
  ssize_t size_writen = ns_linux::open_write_with_timeout(path_fifo_daemon
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span(data.c_str(), data.length())
  );
  // Check for errors
  return (static_cast<size_t>(size_writen) != data.length())?
      Error("E::Could not write data to daemon({}): {}", size_writen, strerror(errno))
    : Value<void>{};
}

/**
 * @brief Waits for the requested process to finish
 *
 * Forwards the child's stdin/stdout/stderr to itself
 *
 * @param message The message containing the FIFO paths
 * @return Value<int> The process exit code or the respective error
 */
[[nodiscard]] Value<int> process_wait(ns_message::Message const& message)
{
  // Child pid received through a fifo
  pid_t pid_child;
  ssize_t bytes_read = ns_linux::open_read_with_timeout(message.get_pid()
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<pid_t>(&pid_child, 1)
  );
  return_if(bytes_read != sizeof(pid_child), Error("E::{}", strerror(errno)));
  // Forward signal to pid
  opt_child = pid_child;
  logger("D::Child pid: {}", pid_child);
  // Connect to stdin, stdout, and stderr with fifos and wait for them to exit
  {
    std::jthread thread_stdin([pid_child, message]
    {
      ns_linux::ns_fd::redirect_fd_to_file(pid_child, STDIN_FILENO, message.get_stdin()).discard();
    });
    std::jthread thread_stdout([pid_child, message]
    {
      ns_linux::ns_fd::redirect_file_to_fd(pid_child, message.get_stdout(), STDOUT_FILENO).discard();
    });
    std::jthread thread_stderr([pid_child, message]
    {
      ns_linux::ns_fd::redirect_file_to_fd(pid_child, message.get_stderr(), STDERR_FILENO).discard();
    });
    logger("D::Connected to stdin/stdout/stderr fifos");
  }
  // Open exit code fifo and retrieve the exit code of the requested process
  int code_exit{};
  int bytes_exit = ns_linux::open_read_with_timeout(message.get_exit().c_str()
    , std::chrono::seconds{SECONDS_TIMEOUT}
    , std::span<int>(&code_exit, 1)
  );
  return_if(bytes_exit != sizeof(code_exit), Error("E::Incorrect number of bytes '{}' read", bytes_exit));
  return code_exit;
}

/**
 * @brief Sends a request to the daemon to create a new process
 *
 * @param cmd Command to request, with it's respective arguments
 * @param daemon_target "host" sends a command to the 'host' daemon and "guest" sends a command to the 'guest' daemon
 * @param path_dir_instance Path to the instance directory to use
 * @return Value<int> The process return code or the respective error
 */
[[nodiscard]] Value<int> process_request(fs::path const& path_fifo_daemon
    , fs::path const& path_dir_fifo
    , std::vector<std::string> const& cmd
  )
{
  using namespace std::chrono_literals;
  // Get environment
  auto environment = std::ranges::subrange(environ, std::unreachable_sentinel)
    | std::views::take_while([](char* p) { return p != nullptr; })
    | std::ranges::to<std::vector<std::string>>();
  // Build message with dispatcher PID
  auto message = ns_message::Message(getpid(), cmd, path_dir_fifo, environment);
  // Create fifos and build message
  Pop(fifo_create(message));
  // Create parent directories
  Pop(ns_fs::create_directories(message.get_exit().parent_path()));
  // Send message to daemon
  Pop(send_message(message, path_fifo_daemon));
  // Wait for child process and retrieve exit code
  return Pop(process_wait(message));
}

/**
 * @brief Entry point for the portal dispatcher
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return int Exit code (0 for success, non-zero for failure)
 */
int main(int argc, char** argv)
{
  auto __expected_fn = [](auto&& e){ logger("E::{}", e.error()); return EXIT_FAILURE; };
  // Set log level
  ns_log::set_level(ns_env::exists("FIM_DEBUG", "1")? ns_log::Level::DEBUG : ns_log::Level::ERROR);
  // Collect arguments
  std::vector<std::string> args(argv+1, argv+argc);
  // No arguments for portal
  return_if(args.empty(), EXIT_FAILURE, "E::No arguments for dispatcher");
  // De-serialize FIM_DISPATCHER_CFG
  ns_dispatcher::Dispatcher arg_cfg = Pop(
    ns_dispatcher::deserialize(Pop(ns_env::get_expected("FIM_DISPATCHER_CFG")))
  );
  // Set log file
  ns_log::set_sink_file(arg_cfg.get_path_file_log());
  // Register signals
  register_signals();
  // Request process from daemon
  return Pop(process_request(arg_cfg.get_path_fifo_daemon()
    , arg_cfg.get_path_dir_fifo()
    , args
  ) , "E::Failure to dispatch process request");
}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
