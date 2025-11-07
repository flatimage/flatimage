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
#include <unordered_map>

#include "../macro.hpp"
#include "../std/expected.hpp"
#include "../lib/env.hpp"
#include "../lib/log.hpp"
#include "../lib/linux.hpp"
#include "../db/db.hpp"
#include "fifo.hpp"
#include "config.hpp"

extern char** environ;

namespace fs = std::filesystem;

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
 * @brief Populates the input file with the current environment
 * 
 * @param path_file_env Path to the environment file to create, discards existing contents
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] Value<void> set_environment(fs::path const& path_file_env)
{
  std::error_code ec;
  fs::path path_dir_env = path_file_env.parent_path();
  qreturn_if(not fs::exists(path_dir_env, ec) and not fs::create_directories(path_dir_env, ec)
    , Error("E::Could not create upper directories of file '{}': '{}'", path_file_env.string(), ec.message())
  );
  std::ofstream ofile_env(path_file_env, std::ios::out | std::ios::trunc);
  qreturn_if(not ofile_env.is_open(), Error("E::Could not open file '{}'", path_file_env));
  for(char **env = environ; *env != NULL; ++env) { ofile_env << *env << '\n'; }
  ofile_env.close();
  return {};
}

/**
 * @brief Sends a message to the portal daemon
 * 
 * @param path_daemon_fifo Path to the fifo which the daemon receives commands from
 * @param command Command to send with arguments
 * @param hash_name_fifo The hash with process pipes to write/read to/from
 * @param path_file_log Path to the log file of the requested child process
 * @param path_file_env Path to the environment to use in the child process
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] Value<void> send_message(fs::path const& path_daemon_fifo
  , std::vector<std::string> command
  , std::unordered_map<std::string, fs::path> const& hash_name_fifo
  , fs::path const& path_file_log
  , fs::path const& path_file_env)
{
  logger("D::Sending message through pipe: {}", path_daemon_fifo);
  // Create command
  auto db = ns_db::Db();
  db("command") = command;
  db("stdin") = hash_name_fifo.at("stdin");
  db("stdout") = hash_name_fifo.at("stdout");
  db("stderr") = hash_name_fifo.at("stderr");
  db("exit") = hash_name_fifo.at("exit");
  db("pid") = hash_name_fifo.at("pid");
  db("log") = path_file_log.c_str();
  db("environment") = path_file_env;
  // Get json string
  std::string data = Pop(db.dump());
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
 * @param hash_name_fifo The hash with process pipes to write/read to/from
 * @return Value<int> Nothing on success or the respective error
 */
[[nodiscard]] Value<int> process_wait(std::unordered_map<std::string, fs::path> const& hash_name_fifo)
{
  pid_t pid_child;
  ssize_t bytes_read = ns_linux::open_read_with_timeout(hash_name_fifo.at("pid")
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<pid_t>(&pid_child, 1)
  );
  qreturn_if(bytes_read != sizeof(pid_child), Error("E::{}", strerror(errno)));
  opt_child = pid_child;
  logger("D::Child pid: {}", pid_child);
  // Connect to stdin, stdout, and stderr with fifos
  pid_t pid_stdin = redirect_fd_to_fifo(pid_child, STDIN_FILENO, hash_name_fifo.at("stdin"));
  pid_t pid_stdout = redirect_fifo_to_fd(pid_child, hash_name_fifo.at("stdout"), STDOUT_FILENO);
  pid_t pid_stderr = redirect_fifo_to_fd(pid_child, hash_name_fifo.at("stderr"), STDERR_FILENO);
  logger("D::Connected to stdin/stdout/stderr fifos");
  // Wait for processes to exit
  waitpid(pid_stdin, nullptr, 0);
  waitpid(pid_stdout, nullptr, 0);
  waitpid(pid_stderr, nullptr, 0);
  // Open exit code fifo and retrieve the exit code of the requested process
  int code_exit{};
  int bytes_exit = ns_linux::open_read_with_timeout(hash_name_fifo.at("exit").c_str()
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
  // Forward signal to spawned child
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
  // Create fifos
  fs::path path_dir_fifo = path_dir_portal / "fifo";
  std::unordered_map<std::string, fs::path> hash_name_fifo =
  {{
      {"stdin" ,  Pop(create_fifo(path_dir_fifo / "stdin"))}
    , {"stdout",  Pop(create_fifo(path_dir_fifo / "stdout"))}
    , {"stderr",  Pop(create_fifo(path_dir_fifo / "stderr"))}
    , {"exit"  ,  Pop(create_fifo(path_dir_fifo / "exit"))}
    , {"pid"   ,  Pop(create_fifo(path_dir_fifo / "pid"))}
  }};
  // Save environment
  fs::path path_file_env = path_dir_portal / "environment";
  Pop(set_environment(path_file_env));
  // Send message to daemon
  Pop(send_message(path_dir_portal / std::format("daemon.{}.fifo", daemon_target)
    , cmd
    , hash_name_fifo
    , path_file_log
    , path_file_env
  ));
  // Retrieve child pid
  return Pop(process_wait(hash_name_fifo));
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
