/**
 * @file linux.hpp
 * @author Ruan Formigoni
 * @brief A library with helpers for linux operations
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <string>
#include <filesystem>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <cassert>
#include <thread>

#include "../macro.hpp"
#include "../std/expected.hpp"

/**
 * @namespace ns_linux
 * @brief Linux-specific system operations
 *
 * Provides Linux kernel-specific functionality including kernel module availability checking,
 * system capability detection, and OS-level feature queries. Used to verify FUSE module
 * availability and other kernel features required by FlatImage.
 */
namespace ns_linux
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Waits for a file descriptor to be ready for I/O with a timeout
 *
 * @param fd The file descriptor to poll
 * @param events Poll events (POLLIN for read, POLLOUT for write)
 * @param timeout The timeout in std::chrono::milliseconds
 * @return bool True if ready, false on timeout or error (errno set appropriately)
 */
[[nodiscard]] inline bool poll_with_timeout(int fd, short events, std::chrono::milliseconds const& timeout)
{
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;

  int poll_result = ::poll(&pfd, 1, timeout.count());

  if (poll_result < 0)
  {
    // poll() error - errno already set
    return false;
  }
  else if (poll_result == 0)
  {
    // Timeout
    errno = ETIMEDOUT;
    return false;
  }

  // Ready for I/O
  return true;
}

/**
 * @brief Reads from the file descriptor with a timeout
 *
 * @tparam Data Type of data elements in the buffer
 * @param fd The file descriptor
 * @param timeout The timeout in std::chrono::milliseconds
 * @param buf The buffer in which to store the read data
 * @return ssize_t The number of read bytes or -1 on error. On error, errno is set appropriately (ETIMEDOUT if timeout expires, or standard read() error codes).
 *
 * @note If timeout expires, returns -1 with errno = ETIMEDOUT. Thread-safe using poll().
 */
template<typename Data>
[[nodiscard]] inline ssize_t read_with_timeout(int fd
  , std::chrono::milliseconds const& timeout
  , std::span<Data> buf)
{
  using Element = typename std::decay_t<typename decltype(buf)::value_type>;

  if (!poll_with_timeout(fd, POLLIN, timeout))
  {
    return -1;
  }

  return ::read(fd, buf.data(), buf.size() * sizeof(Element));
}

/**
 * @brief Writes to the file descriptor with a timeout
 *
 * @tparam Data Type of data elements in the buffer
 * @param fd The file descriptor
 * @param timeout The timeout in std::chrono::milliseconds
 * @param buf The buffer with the data to write
 * @return ssize_t The number of written bytes or -1 on error. On error, errno is set appropriately (ETIMEDOUT if timeout expires, or standard write() error codes).
 *
 * @note If timeout expires, returns -1 with errno = ETIMEDOUT. Thread-safe using poll().
 */
template<typename Data>
[[nodiscard]] inline ssize_t write_with_timeout(int fd
  , std::chrono::milliseconds const& timeout
  , std::span<Data> buf)
{
  using Element = typename std::decay_t<typename decltype(buf)::value_type>;

  if (!poll_with_timeout(fd, POLLOUT, timeout))
  {
    return -1;
  }

  return ::write(fd, buf.data(), buf.size() * sizeof(Element));
}

/**
 * @brief Opens a given file with a timeout
 *
 * @param path_file_src Path for the file to open
 * @param timeout The timeout in std::chrono::milliseconds
 * @param oflag The open flags O_*
 * @return int The file descriptor or -1 on error. On error, errno is set appropriately (ETIMEDOUT on timeout).
 *
 * @note Thread-safe implementation using O_NONBLOCK and retry logic.
 *       For FIFOs: Opens with O_NONBLOCK. For writes (O_WRONLY), retries until reader connects or timeout.
 *       For reads (O_RDONLY), succeeds immediately but may not have a writer yet.
 *       This approach is simpler than pthread_cancel and avoids blocking syscalls entirely.
 *       For regular files: opens normally without timeout.
 */
[[nodiscard]] inline int open_with_timeout(
  fs::path const&            path_file_src,
  std::chrono::milliseconds  timeout,
  int                         oflag)
{
  if(struct stat st; ::stat(path_file_src.c_str(), &st) != 0)
  {
    return -1;
  }
  // Check if this is a FIFO first
  else if (not S_ISFIFO(st.st_mode))
  {
    // For regular files, open normally
    return ::open(path_file_src.c_str(), oflag);
  }
  // For FIFOs: use O_NONBLOCK to avoid blocking
  // For O_WRONLY: open() fails with ENXIO if no reader, so we retry
  // For O_RDONLY: open() succeeds immediately
  for(auto start = std::chrono::steady_clock::now();;)
  {
    if (int fd = ::open(path_file_src.c_str(), oflag | O_NONBLOCK); fd >= 0)
    {
      // Successfully opened - remove O_NONBLOCK if not requested
      if (!(oflag & O_NONBLOCK))
      {
        int flags = ::fcntl(fd, F_GETFL);
        if (flags >= 0)
        {
          ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
        }
      }
      return fd;
    }
    // Check if this is a retry-able error (ENXIO for O_WRONLY means no reader yet)
    if (errno == ENXIO and (oflag & O_WRONLY))
    {
      // Check timeout
      auto elapsed = std::chrono::steady_clock::now() - start;
      if (elapsed >= timeout)
      {
        errno = ETIMEDOUT;
        return -1;
      }
      // Retry after a short delay
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    // Other errors - return immediately
    return -1;
  }
}

/**
 * @brief Opens and reads from the given input file
 *
 * @tparam Data Type of data elements in the buffer
 * @param path_file_src Path to the file to open and read
 * @param timeout The timeout in std::chrono::milliseconds
 * @param buf The buffer in which to store the read data
 * @return ssize_t The number of bytes read or -1 on error. On error, errno is set appropriately (ETIMEDOUT if timeout expires, or standard open()/read() error codes).
 */
template<typename Data>
[[nodiscard]] inline ssize_t open_read_with_timeout(fs::path const& path_file_src
  , std::chrono::milliseconds const& timeout
  , std::span<Data> buf)
{
  int fd = open_with_timeout(path_file_src, timeout, O_RDONLY);
  return_if(fd < 0, fd);
  ssize_t bytes_read = read_with_timeout(fd, timeout, buf);
  close(fd);
  return bytes_read;
}

/**
 * @brief Opens and writes to the given input file
 *
 * @tparam Data Type of data elements in the buffer
 * @param path_file_src Path to the file to open and write
 * @param timeout The timeout in std::chrono::milliseconds
 * @param buf The buffer with the data to write
 * @return ssize_t The number of bytes written or -1 on error. On error, errno is set appropriately (ETIMEDOUT if timeout expires, or standard open()/write() error codes).
 */
template<typename Data>
[[nodiscard]] inline ssize_t open_write_with_timeout(fs::path const& path_file_src
  , std::chrono::milliseconds const& timeout
  , std::span<Data> buf)
{
  int fd = open_with_timeout(path_file_src, timeout, O_WRONLY);
  return_if(fd < 0, fd);
  ssize_t bytes_written = write_with_timeout(fd, timeout, buf);
  close(fd);
  return bytes_written;
}

/**
 * @brief Checks if the linux kernel has a module loaded that matches the input name
 *
 * @param str_name Name of the module to check for
 * @return Value<bool> The boolean result, or the respective internal error
 */
[[nodiscard]] inline Value<bool> module_check(std::string_view str_name)
{
  std::ifstream file_modules("/proc/modules");
  return_if(not file_modules.is_open(), Error("E::Could not open modules file"));

  std::string line;
  while ( std::getline(file_modules, line) )
  {
    return_if(line.contains(str_name), true);
  }

  return false;
}

} // namespace ns_linux

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
