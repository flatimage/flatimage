/**
 * @file pipe.hpp
 * @author Ruan Formigoni
 * @brief Pipe handling utilities for subprocess
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstring>
#include <functional>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <csignal>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <ranges>
#include <format>

#include "../../macro.hpp"
#include "../../lib/log.hpp"

namespace ns_subprocess
{

namespace ns_pipe
{

/**
 * @brief Setup pipes for the parent process
 *
 * @param pipestdout Stdout pipe
 * @param pipestderr Stderr pipe
 * @param fstdout Stdout handler function
 * @param fstderr Stderr handler function
 * @return std::pair<pid_t, pid_t> Pair of PIDs (stdout reader, stderr reader)
 */
inline std::pair<pid_t, pid_t> pipes_parent(
  pid_t child_pid,
  int pipestdout[2],
  int pipestderr[2],
  std::function<void(std::string)> const& fstdout,
  std::function<void(std::string)> const& fstderr
)
{
  pid_t pid_stdout = -1;
  pid_t pid_stderr = -1;

  // Close write end
  ereturn_if(close(pipestdout[1]) == -1, std::format("pipestdout[1]: {}", strerror(errno)), std::make_pair(pid_stdout, pid_stderr));
  ereturn_if(close(pipestderr[1]) == -1, std::format("pipestderr[1]: {}", strerror(errno)), std::make_pair(pid_stdout, pid_stderr));

  auto f_read_pipe = [child_pid](int id_pipe, auto const& handler) -> pid_t
  {
    // Fork a reader process
    pid_t pid = fork();
    qreturn_if(pid < 0, -1);
    // Parent ends here
    if (pid > 0)
    {
      return pid;
    } // if
    // Reader process: Exits when child closes pipe (read() returns 0 at EOF)
    // Backup cleanup: Dies if parent dies via PR_SET_PDEATHSIG(SIGKILL)
    e_exitif(prctl(PR_SET_PDEATHSIG, SIGKILL) < 0, strerror(errno), 1);
    e_exitif(::kill(child_pid, 0) < 0, std::format("Child died, prctl will not have effect: {}", strerror(errno)), 1);
    // Apply handler to incoming data from pipe
    char buffer[1024];
    ssize_t count;
    while ((count = read(id_pipe, buffer, sizeof(buffer))) != 0)
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
          | std::views::filter([](auto const& s){
              return not s.empty() && not std::ranges::all_of(s, ::isspace);
            })
        , [&](auto const& line){ handler(line); }
      );
    } // while
    close(id_pipe);
    // Exit normally
    exit(0);
  };
  // Create pipes from fifo to ostream
  pid_stdout = f_read_pipe(pipestdout[0], fstdout);
  pid_stderr = f_read_pipe(pipestderr[0], fstderr);

  return std::make_pair(pid_stdout, pid_stderr);
}

/**
 * @brief Setup pipes for the child process
 *
 * @param pipestdout Stdout pipe
 * @param pipestderr Stderr pipe
 */
inline void pipes_child(int pipestdout[2], int pipestderr[2])
{
  // Close read end
  ereturn_if(close(pipestdout[0]) == -1, std::format("pipestdout[0]: {}", strerror(errno)));
  ereturn_if(close(pipestderr[0]) == -1, std::format("pipestderr[0]: {}", strerror(errno)));

  // Redirect stdout and stderr to the opened pipes
  ereturn_if(dup2(pipestdout[1], STDOUT_FILENO) == -1, std::format("dup2(pipestdout[1]): {}", strerror(errno)));
  ereturn_if(dup2(pipestderr[1], STDERR_FILENO) == -1, std::format("dup2(pipestderr[1]): {}", strerror(errno)));

  // Close original write end after duplication
  ereturn_if(close(pipestdout[1]) == -1, std::format("pipestdout[1]: {}", strerror(errno)));
  ereturn_if(close(pipestderr[1]) == -1, std::format("pipestderr[1]: {}", strerror(errno)));
}

/**
 * @brief Handle pipe setup for both parent and child processes
 *
 * This function manages the pipe setup after fork():
 * - For parent (pid > 0): Sets up pipe readers and returns their PIDs
 * - For child (pid == 0): Redirects stdout/stderr to pipes
 *
 * @param pid Process ID from fork()
 * @param pipestdout Stdout pipe
 * @param pipestderr Stderr pipe
 * @param fstdout Stdout handler function
 * @param fstderr Stderr handler function
 * @return std::pair<pid_t, pid_t> Pair of PIDs for pipe readers (stdout, stderr), or (-1, -1) if child
 */
inline std::pair<pid_t, pid_t> setup(
  pid_t pid,
  int pipestdout[2],
  int pipestderr[2],
  std::function<void(std::string)> const& fstdout,
  std::function<void(std::string)> const& fstderr
)
{
  // On parent, setup pipe readers
  if ( pid > 0 )
  {
    return pipes_parent(pid, pipestdout, pipestderr, fstdout, fstderr);
  }

  // On child, redirect stdout/stderr to pipes
  if ( pid == 0 )
  {
    pipes_child(pipestdout, pipestderr);
  }

  return std::make_pair(-1, -1);
}

} // namespace ns_pipe

} // namespace ns_subprocess

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
