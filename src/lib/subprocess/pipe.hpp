/**
 * @file pipe.hpp
 * @author Ruan Formigoni
 * @brief Pipe handling utilities for subprocess
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstring>
#include <istream>
#include <thread>
#include <chrono>
#include <functional>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <csignal>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <ranges>
#include <format>
#include <filesystem>

#include "../../macro.hpp"
#include "../../lib/log.hpp"

namespace ns_subprocess
{

namespace ns_pipe
{

/**
 * @brief Write from an input stream to a pipe file descriptor
 *
 * Reads lines from the input stream and writes them to the pipe.
 * Continues until the child process exits or stream errors occur.
 *
 * @param child_pid PID of the child process to monitor
 * @param pipe_fd File descriptor of the pipe write end
 * @param stream Input stream to read from
 */
inline void write_pipe(pid_t child_pid, int pipe_fd, std::istream& stream)
{
  for (std::string line; kill(child_pid, 0) == 0; )
  {
    if (std::getline(stream, line))
    {
      line += "\n";
      ssize_t written = ::write(pipe_fd, line.c_str(), line.length());
      logger("D::STDIN::{}", line);
      ebreak_if(written < 0, std::format("E::Failed to write to child stdin: {}", strerror(errno)));
    }
    else if (stream.eof())
    {
      stream.clear();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    else
    {
      break;
    }
  }
  close(pipe_fd);
}

/**
 * @brief Read from a pipe file descriptor and write to an output stream
 *
 * Reads data from the pipe and writes it to the output stream.
 * Handles line splitting and filtering of empty/whitespace-only lines.
 * Replaces carriage returns with newlines to handle Windows-style line endings
 * and progress updates.
 *
 * @param pipe_fd File descriptor of the pipe read end
 * @param stream Output stream to write to
 * @param path_file_log Log file path for logging output
 */
inline void read_pipe(int pipe_fd, std::ostream& stream, std::filesystem::path const& path_file_log)
{
  ns_log::set_sink_file(path_file_log);

  // Apply handler to incoming data from pipe
  char buffer[1024];
  ssize_t count;
  while ((count = ::read(pipe_fd, buffer, sizeof(buffer))) != 0)
  {
    // Failed to read
    ebreak_if(count == -1, std::format("broke parent read loop: {}", strerror(errno)));
    // Split on both newlines and carriage returns, filter empty/whitespace-only lines
    std::string chunk(buffer, count);
    // Replace all \r with \n to handle Windows-style line endings and progress updates
    std::ranges::replace(chunk, '\r', '\n');
    // Split by newline and process each line
    std::ranges::for_each(chunk
        | std::views::split('\n')
        | std::views::transform([](auto&& e){ return std::string{e.begin(), e.end()}; })
        | std::views::filter([](auto const& s){ return not s.empty(); })
        | std::views::filter([](auto const& s){ return not std::ranges::all_of(s, ::isspace); })
      , [&](auto line) { logger("D::STD(OUT|ERR)::{}", line); stream << line << std::endl; }
    );
  }
  close(pipe_fd);
}

/**
 * @brief Check if a stream is a standard stream (std::cin, std::cout, or std::cerr)
 *
 * @tparam Stream The stream type (std::istream or std::ostream)
 * @param stream The stream reference to check
 * @return true if the stream is std::cin, std::cout, or std::cerr
 */
template<typename Stream>
bool is_standard_stream(Stream& stream)
{
  if constexpr (std::is_same_v<Stream, std::istream>)
  {
    return (&stream == &std::cin);
  }
  else if constexpr (std::is_same_v<Stream, std::ostream>)
  {
    return (&stream == &std::cout) or (&stream == &std::cerr);
  }
  return false;
}

/**
 * @brief Setup pipe for child process (unified for stdin/stdout/stderr)
 *
 * @tparam Stream The stream type (std::istream or std::ostream)
 * @param is_istream True for input streams (stdin), false for output streams (stdout/stderr)
 * @param pipe Pipe array [read_end, write_end]
 * @param stream The stream reference to check
 * @param fileno The file descriptor to redirect to (STDIN_FILENO, STDOUT_FILENO, or STDERR_FILENO)
 */
template<typename Stream>
void pipes_child(bool is_istream, int pipe[2], Stream& stream, int fileno)
{
  // For input streams (stdin):  child uses read end [0], parent uses write end [1]
  // For output streams (stdout/stderr): child uses write end [1], parent uses read end [0]
  int idx_child  = is_istream ? 0 : 1;
  int idx_parent = is_istream ? 1 : 0;

  // Close the parent's end
  ereturn_if(close(pipe[idx_parent]) == -1, std::format("pipe[{}]: {}", idx_parent, strerror(errno)));

  // If using standard stream, don't redirect (allow terminal access)
  if (is_standard_stream(stream))
  {
    close(pipe[idx_child]);
    return;
  }

  // Redirect to the specified file descriptor
  ereturn_if(dup2(pipe[idx_child], fileno) == -1, std::format("dup2(pipe[{}], {}): {}", idx_child, fileno, strerror(errno)));
  ereturn_if(close(pipe[idx_child]) == -1, std::format("pipe[{}]: {}", idx_child, strerror(errno)));
}

/**
 * @brief Setup pipe for parent process (unified for stdin/stdout/stderr)
 *
 * @tparam Stream The stream type (std::istream or std::ostream)
 * @param child_pid PID of child process (needed for stdin writer)
 * @param is_istream True for input streams (stdin), false for output streams (stdout/stderr)
 * @param pipe Pipe array [read_end, write_end]
 * @param stream The stream reference to use
 * @param path_file_log Optional log file path (unused for stdin)
 */
template<typename Stream>
void pipes_parent(
  pid_t child_pid,
  bool is_istream,
  int pipe[2],
  Stream& stream,
  std::filesystem::path const& path_file_log)
{
  // For input streams (stdin):  parent uses write end [1], child uses read end [0]
  // For output streams (stdout/stderr): parent uses read end [0], child uses write end [1]
  int idx_parent = is_istream ? 1 : 0;
  int idx_child  = is_istream ? 0 : 1;

  // Close the child's end
  ereturn_if(close(pipe[idx_child]) == -1, std::format("pipe[{}]: {}", idx_child, strerror(errno)));

  // If using standard stream, close parent end and return (no thread needed)
  if (is_standard_stream(stream))
  {
    close(pipe[idx_parent]);
    return;
  }

  // Create appropriate thread (use if constexpr to avoid compiling invalid branch)
  if constexpr (std::is_same_v<Stream, std::istream>)
  {
    // Input stream: read from Stream and write to child's stdin pipe
    std::thread(write_pipe, child_pid, pipe[idx_parent], std::ref(stream)).detach();
  }
  else // std::ostream
  {
    // Output stream: read from child's stdout/stderr pipe and write to Stream
    std::thread(read_pipe, pipe[idx_parent], std::ref(stream), path_file_log).detach();
  }
}

/**
 * @brief Handle pipe setup for both parent and child processes
 *
 * This function manages the pipe setup after fork():
 * - For parent (pid > 0): Creates detached threads for reading/writing pipes
 * - For child (pid == 0): Redirects stdin/stdout/stderr to pipes via dup2()
 *
 * Standard streams (std::cin, std::cout, std::cerr) are not redirected,
 * allowing terminal access.
 *
 * @param pid Process ID from fork() (0 for child, >0 for parent)
 * @param pipestdin Stdin pipe array [read_end, write_end]
 * @param pipestdout Stdout pipe array [read_end, write_end]
 * @param pipestderr Stderr pipe array [read_end, write_end]
 * @param stdin Input stream to read from (for child's stdin)
 * @param stdout Output stream to write to (for child's stdout)
 * @param stderr Error stream to write to (for child's stderr)
 * @param path_file_log Log file path for pipe reader threads
 */
inline void setup(
  pid_t pid,
  int pipestdin[2],
  int pipestdout[2],
  int pipestderr[2],
  std::istream& stdin,
  std::ostream& stdout,
  std::ostream& stderr,
  std::filesystem::path const& path_file_log)
{
  // Parent: setup writer/reader threads (or skip if std::cin/cout/cerr)
  if (pid > 0)
  {
    pipes_parent(pid, true, pipestdin, stdin, path_file_log);
    pipes_parent(pid, false, pipestdout, stdout, path_file_log);
    pipes_parent(pid, false, pipestderr, stderr, path_file_log);
  }

  // Child: redirect stdin/stdout/stderr (or skip if std::cin/cout/cerr)
  if (pid == 0)
  {
    pipes_child(true, pipestdin, stdin, STDIN_FILENO);
    pipes_child(false, pipestdout, stdout, STDOUT_FILENO);
    pipes_child(false, pipestderr, stderr, STDERR_FILENO);
  }
}

} // namespace ns_pipe

} // namespace ns_subprocess

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
