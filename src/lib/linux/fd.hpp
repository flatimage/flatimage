/**
 * @file fd.hpp
 * @author Ruan Formigoni
 * @brief File descriptor redirection helpers
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <sys/types.h>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <span>

#include "../linux.hpp"

namespace ns_linux::ns_fd
{

constexpr uint32_t const SECONDS_TIMEOUT = 5;
constexpr uint32_t const SIZE_BUFFER_READ = 16384;

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
      if(errno == EWOULDBLOCK or errno == EAGAIN or errno == ETIMEDOUT)
      {
        return true;  // Timeout or would block - keep trying
      }
      logger("E::Failed to read from file descriptor '{}' with error '{}'", fd_src, strerror(errno));
      return false;
    }
    log_if(::write(fd_dst, buf, n) < 0, "D::Could not write to file descriptor '{}' with error '{}'", fd_dst, strerror(errno));
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
 * @brief Redirects the output of a file to a file descriptor
 *
 * @param ppid Keep trying to read and write while this pid is alive
 * @param path_file_file Path to the file to read from
 * @param fd_dst Path to the file descriptor to write to
 * @return std::jthread Returns the spawned redirector thread
 */
[[nodiscard]] inline std::jthread redirect_file_to_fd(pid_t ppid, fs::path const& path_file, int fd_dst)
{
  return std::jthread([=]
  {
    // Try to open file within a timeout
    int fd_src = ns_linux::open_with_timeout(path_file.c_str()
      , std::chrono::seconds(SECONDS_TIMEOUT)
      , O_RDONLY
    );
    if (fd_src == -1)
    {
      logger("E::Failed to open file '{}' with error '{}'", path_file, strerror(errno));
      return;
    }
    // Redirect file to stdout
    redirect_fd_to_fd(ppid, fd_src, fd_dst);
    // Close file
    close(fd_src);
  });
}


/**
 * @brief Redirects the output of a file descriptor to a file
 *
 * @param ppid Keep trying to read and write while this pid is alive
 * @param fd_src Path to the file descriptor to read from
 * @param path_file_file Path to the file to write to
 * @return std::jthread Returns the spawned redirector thread
 */
[[nodiscard]] inline std::jthread redirect_fd_to_file(pid_t ppid
  , int fd_src
  , fs::path const& path_file)
{
  return std::jthread([=]
  {
    // Try to open file within a timeout
    int fd_dst = ns_linux::open_with_timeout(path_file.string().c_str()
      , std::chrono::seconds(SECONDS_TIMEOUT)
      , O_WRONLY
    );
    if (fd_dst == -1)
    {
      logger("E::Failed to open file '{}' with error '{}'", path_file.string(), strerror(errno));
      return;
    }
    // Redirect file to stdout
    redirect_fd_to_fd(ppid, fd_src, fd_dst);
    // Close file
    close(fd_dst);
  });
}

} // namespace ns_linux::ns_fd