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
#include "../lib/subprocess.hpp"
#include "../db/portal/message.hpp"
#include "config.hpp"

namespace fs = std::filesystem;

namespace ns_portal::ns_child
{

[[nodiscard]] inline Value<void> write_fifo(int const value, fs::path const& path_file_fifo)
{
  // Write to fifo
  ssize_t bytes_written = ns_linux::open_write_with_timeout(path_file_fifo
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<pid_t const>(&value, 1)
  );
  // Check success with number of written bytes
  qreturn_if(bytes_written != sizeof(value), Error("E::Failed to write pid: {}", strerror(errno)));
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
  write_fifo(pid, message.get_pid()).discard("C::Failed to write pid to fifo");
  // Wait for child to finish
  int status = -1;
  elog_if(::waitpid(pid, &status, 0) < 0, std::format("Could not wait for child: {}", strerror(errno)));
  // Get exit code
  int code = (not WIFEXITED(status))? 1 : WEXITSTATUS(status);
  logger("D::Exit code: {}", code);
  // Send exit code of child through a fifo
  write_fifo(code, message.get_exit()).discard("C::Failed to write exit code to fifo");
}

/**
 * @brief Forks a child
 *
 * @param vec_argv Arguments to the child process
 * @param message Message with the process details, e.g. (stdin/stdout/stderr) pipes
 * @return Value<void>
 */
[[nodiscard]] inline Value<void> spawn(std::vector<std::string> const& vec_argv, ns_db::ns_portal::ns_message::Message const& message)
{
  using ns_subprocess::ArgsCallbackParent;
  using ns_subprocess::ArgsCallbackChild;
  // Split cmd / args
  std::string command = Pop(ns_env::search_path(Try(vec_argv.front())));
  std::vector<std::string> args(vec_argv.begin()+1, vec_argv.end());
  // Spawn child and wait in the parent callback which is executed in this thread
  std::ignore = ns_subprocess::Subprocess(command)
    .with_args(args)
    .with_env(message.get_environment())
    .with_stdio(ns_subprocess::Stream::Inherit)
    .with_die_on_pid(getpid())
    .with_callback_parent([=](ArgsCallbackParent const& args) -> void
    {
      parent_wait(args.child_pid, message);
    })
    .with_callback_child([=](ArgsCallbackChild const&) -> void
    {
      auto f_open_fd = [](fs::path const& path_fifo, int const fileno, int const oflag) -> Value<void>
      {
        int fd = ns_linux::open_with_timeout(path_fifo, std::chrono::seconds(SECONDS_TIMEOUT), oflag);
        qreturn_if(fd < 0, Error("E::open fd failed"));
        qreturn_if(dup2(fd, fileno) < 0, Error("E::dup2 failed"));
        close(fd);
        return {};
      };
      auto __expected_fn = [](auto&&){ return; };
      Pop(f_open_fd(message.get_stdin(), STDIN_FILENO, O_RDONLY));
      Pop(f_open_fd(message.get_stdout(), STDOUT_FILENO, O_WRONLY));
      Pop(f_open_fd(message.get_stderr(), STDERR_FILENO, O_WRONLY));
    })
    .spawn();
  return {};
}

/**
 * @brief Forks and execve a new child
 *
 * @param path_dir_portal Path to the portal directory of the child process
 * @param message The message containing the command and process details
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> spawn(ns_db::ns_portal::ns_message::Message const& message)
{
  // Get command from message
  auto vec_argv = message.get_command();
  // Ignore on empty command
  if ( vec_argv.empty() ) { return Error("E::Empty command"); }
  // Perform execve
  Pop(spawn(vec_argv, message));
  // Parent should exit here
  _exit(0);
}

} // namespace ns_portal::ns_child