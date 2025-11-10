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
#include "../db/portal/daemon.hpp"
#include "config.hpp"

namespace fs = std::filesystem;

namespace ns_portal::ns_child
{

namespace ns_daemon = ns_db::ns_portal::ns_daemon;
namespace ns_message = ns_db::ns_portal::ns_message;


/**
 * @brief Writes a value to a fifo given a file path
 *
 * Opens the fifo file with a timeout of SECONDS_TIMEOUT and writes the data
 *
 * @param value The value to write as a single integer constant
 * @param path_fifo The path to the fifo file to open and write the data to
 * @return Value<void> Success or error
 */
[[nodiscard]] inline Value<void> write_fifo(int const value, fs::path const& path_fifo)
{
  // Write to fifo
  ssize_t bytes_written = ns_linux::open_write_with_timeout(path_fifo
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<pid_t const>(&value, 1)
  );
  // Check success with number of written bytes
  qreturn_if(bytes_written != sizeof(value), Error("E::Failed to write pid: {}", strerror(errno)));
  return {};
}

/**
 * @brief Forks a child process and waits for it to complete
 *
 * Spawns a child process with the specified command and arguments, redirects
 * its stdio to pipes, and waits for it to exit. Communicates the child's PID
 * and exit code through FIFOs to the portal daemon.
 *
 * @param logs Logging configuration with paths for log files
 * @param vec_argv Arguments to the child process (first element is the command)
 * @param message Message containing FIFO paths and environment for IPC
 * @return Value<void> Success or error
 */
[[nodiscard]] inline Value<void> spawn(ns_daemon::ns_log::Logs logs
  , std::vector<std::string> const& vec_argv
  , ns_db::ns_portal::ns_message::Message const& message)
{
  using ns_subprocess::ArgsCallbackParent;
  using ns_subprocess::ArgsCallbackChild;
  // Split cmd / args
  std::string command = Pop(ns_env::search_path(Try(vec_argv.front())));
  std::vector<std::string> args(vec_argv.begin()+1, vec_argv.end());
  // Spawn child and wait in the parent callback which is executed in this thread
  auto child = ns_subprocess::Subprocess(command)
    .with_args(args)
    .with_env(message.get_environment())
    .with_stdio(ns_subprocess::Stream::Pipe)
    .with_log_file(logs.get_path_file_grand())
    .with_die_on_pid(getpid())
    .spawn();

  // Write pid to fifo
  pid_t pid_child = child->get_pid().value_or(-1);
  write_fifo(pid_child, message.get_pid()).discard("C::Failed to write pid to fifo");
  // Wait for child to finish
  int code = Pop(child->wait(), "E::Child exited abnormally");
  logger("D::Exit code: {}", code);
  // Send exit code of child through a fifo
  write_fifo(code, message.get_exit()).discard("C::Failed to write exit code to fifo");
  return {};
}

/**
 * @brief Forks and executes a new child process
 *
 * Sets up child logging, extracts the command from the message, and spawns
 * the child process. This function exits the parent process after spawning.
 *
 * @param logs Logging configuration with paths for log files
 * @param message The message containing the command, environment, and FIFO paths
 * @return Value<void> Success or error (note: exits on success via _exit(0))
 */
[[nodiscard]] inline Value<void> spawn(ns_daemon::ns_log::Logs logs, ns_message::Message const& message)
{
  // Setup child logging
  ns_log::set_sink_file(logs.get_path_file_child());
  // Get command from message
  auto vec_argv = message.get_command();
  // Ignore on empty command
  if ( vec_argv.empty() ) { return Error("E::Empty command"); }
  // Perform execve
  Pop(spawn(logs, vec_argv, message));
  return {};
}

} // namespace ns_portal::ns_child