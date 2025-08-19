/**
 * @file fifo.hpp
 * @author Ruan Formigoni
 * @brief Operations on fifos (named pipes)
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <expected>
#include <thread>
#include <sys/stat.h> // mkfifo
#include <fcntl.h> // O_RDONLY
#include <sys/prctl.h> // PR_SET_PDEATHSIG

#include "../macro.hpp"
#include "../lib/linux.hpp"
#include "config.hpp"

namespace fs = std::filesystem;

/**
 * @brief Create a fifo object
 * 
 * @param path_file_fifo Where to saved the fifo to
 * @return Expected<fs::path> The path to the created fifo, or the respective error
 */
[[nodiscard]] inline Expected<fs::path> create_fifo(fs::path const& path_file_fifo)
{
  std::error_code ec;
  fs::path path_dir_parent = path_file_fifo.parent_path();
  // Create parent directory(ies)
  qreturn_if(not fs::exists(path_dir_parent, ec) and not fs::create_directories(path_dir_parent, ec)
    , Unexpected("Failed to create upper directories for fifo")
  );
  // Replace old fifo if exists
  if (fs::exists(path_file_fifo, ec))
  {
    fs::remove(path_file_fifo, ec);
  }
  // Create fifo
  qreturn_if(mkfifo(path_file_fifo.c_str(), 0666) < 0
    , Unexpected(strerror(errno))
  );
  return path_file_fifo;
}

/**
 * @brief Redirects the output of one file descriptor as input of another
 * 
 * @param ppid Keep trying to read and write while this pid is alive
 * @param fd_src File descriptor to read from
 * @param fd_dst File descriptor to write to
 */
inline void redirect_fd_to_fd(pid_t ppid, int fd_src, int fd_dst)
{
  auto f_read = [](int fd_src, int fd_dst) -> bool
  {
    char buf[SIZE_BUFFER_READ];

    ssize_t n = ns_linux::read_with_timeout(fd_src
      , std::chrono::milliseconds(100)
      , std::span(buf, sizeof(buf))
    );

    if(n == 0)
    {
      return false;
    }
    else if(n < 0)
    {
      if(errno != EWOULDBLOCK and errno != EAGAIN and errno != EINTR)
      {
        ns_log::error()("Failed read with error: {}", strerror(errno));
        return false;
      }
      return true;
    }
    dlog_if(::write(fd_dst, buf, n) < 0, "Could not perform write: {}"_fmt(strerror(errno)));
    return true;
  };
  // Try to read from the src fifo with 50ms delays
  while (::kill(ppid, 0) == 0 and f_read(fd_src, fd_dst))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  // After the process exited, check for any leftover output
  std::ignore = f_read(fd_src, fd_dst);
}

/**
 * @brief Redirects the output of a fifo to a file descriptor
 * 
 * @param ppid Keep trying to read and write while this pid is alive
 * @param path_file_fifo Path to the fifo to read from
 * @param fd_dst Path to the file descriptor to write to
 * @return pid_t The value of fork() on parent and calls _exit(code) on child
 */
[[nodiscard]] inline pid_t redirect_fifo_to_fd(pid_t ppid, fs::path const& path_file_fifo, int fd_dst)
{
  // Fork
  pid_t pid = fork();
  ereturn_if(pid < 0, "Could not fork '{}'"_fmt(strerror(errno)), -1);
  // Parent ends here
  if( pid > 0 ) { return pid; }
  // Try to open fifo within a timeout
  int fd_src = ns_linux::open_with_timeout(path_file_fifo.string().c_str()
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , O_RDONLY
  );
  e_exitif(fd_src == -1, strerror(errno), 1);
  // Redirect fifo to stdout
  redirect_fd_to_fd(ppid, fd_src, fd_dst);
  // Close fifo
  close(fd_src);
  // Exit without cleanup
  _exit(0);
}


/**
 * @brief Redirects the output of a file descriptor to a fifo
 * 
 * @param ppid Keep trying to read and write while this pid is alive
 * @param fd_src Path to the file descriptor to read from
 * @param path_file_fifo Path to the fifo to write to
 * @return pid_t The value of fork() on parent and calls _exit(code) on child
 */
[[nodiscard]] inline pid_t redirect_fd_to_fifo(pid_t ppid, int fd_src, fs::path const& path_file_fifo)
{
  // Fork
  pid_t pid = fork();
  ereturn_if(pid < 0, "Could not fork '{}'"_fmt(strerror(errno)), -1);
  // Parent ends here
  if( pid > 0 ) { return pid; }
  // Try to open fifo within a timeout
  int fd_dst = ns_linux::open_with_timeout(path_file_fifo.string().c_str()
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , O_WRONLY
  );
  e_exitif(fd_dst == -1, strerror(errno), 1);
  // Redirect fifo to stdout
  redirect_fd_to_fd(ppid, fd_src, fd_dst);
  // Close fifo
  close(fd_dst);
  // Exit without cleanup
  _exit(0);
}