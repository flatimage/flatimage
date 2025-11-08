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
#include "../lib/env.hpp"
#include "../lib/log.hpp"
#include "../lib/linux.hpp"
#include "../db/portal/message.hpp"
#include "fifo.hpp"
#include "config.hpp"

namespace fs = std::filesystem;
namespace ns_message = ns_db::ns_portal::ns_message;

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
 * @brief Collects the current environment variables into a vector
 *
 * @return Value<std::vector<std::string>> The environment variables or the respective error
 */
[[nodiscard]] Value<std::vector<std::string>> get_environment()
{
  std::vector<std::string> environment;
  for(char **env = environ; *env != NULL; ++env)
  {
    environment.push_back(*env);
  }
  return environment;
}

/**
 * @brief Sends a message to the portal daemon
 *
 * @param path_daemon_fifo Path to the fifo which the daemon receives commands from
 * @param message The message to send
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] Value<void> send_message(fs::path const& path_daemon_fifo
  , ns_message::Message const& message)
{
  logger("D::Sending message through pipe: {}", path_daemon_fifo);
  // Serialize to json string
  std::string data = Pop(ns_message::serialize(message));
  logger("D::{}", data);
  // Write to fifo
  ssize_t size_writen = ns_linux::open_write_with_timeout(path_daemon_fifo
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
  pid_t pid_child;
  ssize_t bytes_read = ns_linux::open_read_with_timeout(message.get_pid()
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<pid_t>(&pid_child, 1)
  );
  qreturn_if(bytes_read != sizeof(pid_child), Error("E::{}", strerror(errno)));
  opt_child = pid_child;
  logger("D::Child pid: {}", pid_child);
  // Connect to stdin, stdout, and stderr with fifos
  pid_t pid_stdin = redirect_fd_to_fifo(pid_child, STDIN_FILENO, message.get_stdin());
  pid_t pid_stdout = redirect_fifo_to_fd(pid_child, message.get_stdout(), STDOUT_FILENO);
  pid_t pid_stderr = redirect_fifo_to_fd(pid_child, message.get_stderr(), STDERR_FILENO);
  logger("D::Connected to stdin/stdout/stderr fifos");
  // Wait for processes to exit
  waitpid(pid_stdin, nullptr, 0);
  waitpid(pid_stdout, nullptr, 0);
  waitpid(pid_stderr, nullptr, 0);
  // Open exit code fifo and retrieve the exit code of the requested process
  int code_exit{};
  int bytes_exit = ns_linux::open_read_with_timeout(message.get_exit().c_str()
    , std::chrono::seconds{SECONDS_TIMEOUT}
    , std::span<int>(&code_exit, 1)
  );
  qreturn_if(bytes_exit != sizeof(code_exit), Error("E::Incorrect number of bytes '{}' read", bytes_exit));
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
[[nodiscard]] Value<int> process_request(std::vector<std::string> const& cmd
  , std::string const& daemon_target
  , fs::path const& path_dir_instance)
{
  using namespace std::chrono_literals;

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
  // Set log level
  ns_log::set_level(ns_env::exists("FIM_DEBUG", "1")? ns_log::Level::DEBUG : ns_log::Level::ERROR);
  // Get portal directory
  fs::path path_dir_portal = path_dir_instance / "portal";
  // Create define log file for child
  fs::path path_file_log = path_dir_portal / "cli.log";
  qreturn_if(std::error_code ec; (fs::create_directories(path_file_log.parent_path(), ec))
    , Error("E::Error to create log file: {}", ec.message())
  );
  // Create fifos and build message
  fs::path path_dir_fifo = path_dir_portal / "fifo";
  auto message = ns_message::Message()
    .with_command(cmd)
    .with_stdin(Pop(create_fifo(path_dir_fifo / "stdin")))
    .with_stdout(Pop(create_fifo(path_dir_fifo / "stdout")))
    .with_stderr(Pop(create_fifo(path_dir_fifo / "stderr")))
    .with_exit(Pop(create_fifo(path_dir_fifo / "exit")))
    .with_pid(Pop(create_fifo(path_dir_fifo / "pid")))
    .with_log(path_file_log)
    .with_environment(Pop(get_environment()));
  // Send message to daemon
  Pop(send_message(path_dir_portal / std::format("daemon.{}.fifo", daemon_target), message));
  // Wait for child process and retrieve exit code
  return Pop(process_wait(message));
}

int main(int argc, char** argv)
{
  auto __expected_fn = [](auto&&){ return EXIT_FAILURE; };
  std::vector<std::string> args(argv+1, argv+argc);
  // Daemon target
  // "host" - Sends a command to the 'host' daemon
  // "guest" - Sends a command to the 'guest' daemon
  std::string daemon_target = "host";
  // Path to the current instance
  fs::path path_dir_instance;
  // Get instance path or acquire it from an environment variable
  if (args.size() >= 2 and Try(args.at(0)) == "--connect")
  {
    daemon_target = "guest";
    path_dir_instance = Try(args.at(1));
    args.erase(args.begin(), args.begin()+2);
  }
  else
  {
    path_dir_instance = ({
      auto ret = ns_env::get_expected("FIM_DIR_INSTANCE");
      if(not ret)
      {
        std::cerr << "FIM_DIR_INSTANCE is undefined\n";
        return EXIT_FAILURE;
      }
      ret.value();
    });
  }
  // Request process from daemon
  auto result = process_request(args, daemon_target, path_dir_instance);
  // Reflect the original process return code
  if(result) { return result.value(); }
  // Error on process request
  logger("E::{}", result.error());
  return EXIT_FAILURE;
}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
