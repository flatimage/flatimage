/**
 * @file child.hpp
 * @author Ruan Formigoni
 * @brief Spawns a child process and connects its I/O to pipes
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <chrono>
#include <fcntl.h>
#include <string>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../std/expected.hpp"
#include "../lib/linux.hpp"
#include "../lib/env.hpp"
#include "../db/portal/message.hpp"
#include "config.hpp"

namespace fs = std::filesystem;

namespace child
{

/**
 * @brief Write a PID to a fifo
 *
 * @param pid The pid to write
 * @param message The message that contains the "pid" fifo path
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> write_fifo_pid(pid_t const pid, ns_db::ns_portal::ns_message::Message const& message)
{
  // Get pid fifo
  auto path_fifo_pid = message.get_pid();
  // Write to fifo
  ssize_t bytes_written = ns_linux::open_write_with_timeout(path_fifo_pid
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<pid_t const>(&pid, 1)
  );
  // Check success with number of written bytes
  elog_if(bytes_written != sizeof(pid), std::format("Failed to write pid: {}", strerror(errno)));
  return {};
}

/**
 * @brief Writes an exit code to a fifo
 *
 * @param code The exit code to write
 * @param message The message that contains the "exit" fifo path
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> write_fifo_exit(int const code, ns_db::ns_portal::ns_message::Message const& message)
{
  // Get exit code fifo
  auto path_file_fifo = message.get_exit();
  // Write to fifo
  ssize_t bytes_written = ns_linux::open_write_with_timeout(path_file_fifo
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<int const>(&code, 1)
  );
  // Check success with number of written bytes
  elog_if(bytes_written != sizeof(code), std::format("Failed to write code: {}", strerror(errno)));
  return {};
}

/**
 * @brief Waits for a child pid to exit
 *
 * @param pid The pid to wait for
 * @param message The message to write the pid and exit code into
 */
inline void parent_wait(pid_t const pid, ns_db::ns_portal::ns_message::Message const& message)
{
  // Write pid to fifo
  write_fifo_pid(pid, message).discard("C::Failed to write pid to fifo");
  // Wait for child to finish
  int status = -1;
  elog_if(::waitpid(pid, &status, 0) < 0
    , std::format("Could not wait for child: {}", strerror(errno))
  );
  // Get exit code
  int code = (not WIFEXITED(status))? 1 : WEXITSTATUS(status);
  logger("D::Exit code: {}", code);
  // Send exit code of child through a fifo
  write_fifo_exit(code, message).discard("C::Failed to write exit code to fifo");
}

/**
 * @brief Forks a child
 *
 * @param vec_argv Arguments to the child process
 * @param message Message with the process details, e.g. (stdin/stdout/stderr) pipes
 * @return Value<void>
 */
[[nodiscard]] inline Value<void> child_execve(std::vector<std::string> const& vec_argv, ns_db::ns_portal::ns_message::Message const& message)
{
  // Create arguments for execve
  auto argv_custom = std::make_unique<const char*[]>(vec_argv.size()+1);
  argv_custom[vec_argv.size()] = nullptr;
  std::ranges::transform(vec_argv
     , argv_custom.get()
     , [](auto&& e){ return e.c_str(); }
  );
  // Get environment vector from message
  std::vector<std::string> const& vec_environment = message.get_environment();
  auto env_custom = std::make_unique<const char*[]>(vec_environment.size()+1);
  env_custom[vec_environment.size()] = nullptr;
  std::ranges::transform(vec_environment
     , env_custom.get()
     , [](auto&& e){ return e.c_str(); }
  );
  // Configure pipes
  auto f_open_fd = [](fs::path const& path_fifo, int const fileno, int const oflag) -> Value<void>
  {
    int fd = ns_linux::open_with_timeout(path_fifo
      , std::chrono::seconds(SECONDS_TIMEOUT)
      , oflag
    );
    qreturn_if(fd < 0, Error("E::open fd failed"));
    qreturn_if(dup2(fd, fileno) < 0, Error("E::dup2 failed"));
    close(fd);
    return {};
  };
  Pop(f_open_fd(message.get_stdin(), STDIN_FILENO, O_RDONLY));
  Pop(f_open_fd(message.get_stdout(), STDOUT_FILENO, O_WRONLY));
  Pop(f_open_fd(message.get_stderr(), STDERR_FILENO, O_WRONLY));
  // Perform execve
  int ret_execve = execve(argv_custom[0], (char**) argv_custom.get(), (char**) env_custom.get());
  logger("E::Could not perform execve({}): {}", ret_execve, strerror(errno));
  _exit(1);
}

/**
 * @brief Forks and execve a new child
 *
 * @param path_dir_portal Path to the portal directory of the child process
 * @param message The message containing the command and process details
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> spawn(fs::path const& path_dir_portal, ns_db::ns_portal::ns_message::Message const& message)
{
  ns_log::set_sink_file(path_dir_portal / std::format("spawned_parent_{}.log", getpid()));
  ns_log::set_level(ns_log::Level::CRITICAL);
  // Get command from message
  auto vec_argv = message.get_command();
  // Ignore on empty command
  if ( vec_argv.empty() ) { return Error("E::Empty command"); }
  // Search for command in PATH and replace vec_argv[0] with the full path to the binary
  vec_argv[0] = Pop(ns_env::search_path(vec_argv[0]));
  // Create child
  pid_t ppid = getpid();
  pid_t pid = fork();
  // Failed to fork
  if(pid < 0)
  {
    return Error("E::Failed to fork");
  }
  // Is parent
  else if (pid > 0)
  {
    parent_wait(pid, message);
    _exit(0);
  }
  // Is child
  // Configure child logger
  ns_log::set_sink_file(path_dir_portal / std::format("spawned_child_{}.log", getpid()));
  ns_log::set_level(ns_log::Level::CRITICAL);
  // Die with parent
  qreturn_if(::prctl(PR_SET_PDEATHSIG, SIGKILL) < 0, Error("E::Could not set child to die with parent"));
  // Check if parent still exists
  qreturn_if(::kill(ppid, 0) < 0, Error("E::Parent pid is already dead"));
  // Perform execve
  Pop(child_execve(vec_argv, message));
  _exit(1);
}

} // namespace child