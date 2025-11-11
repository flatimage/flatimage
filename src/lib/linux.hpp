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
#include <string>
#include <filesystem>
#include <optional>
#include <sys/time.h>
#include <unistd.h>
#include <cassert>

#include "../macro.hpp"
#include "../std/expected.hpp"
#include "log.hpp"

namespace ns_linux
{

extern "C" inline void alarm_handler(int) { /* no-op */ }
namespace
{

namespace fs = std::filesystem;

/**
 * @brief RAII wrapper for POSIX interval timers with SIGALRM
 *
 * @warning NOT THREAD-SAFE: Uses process-wide resources (SIGALRM, ITIMER_REAL).
 *          Multiple instances or concurrent use will cause race conditions.
 *          Only use in single-threaded contexts or with external synchronization.
 *
 * @note Signal behavior:
 *       - Replaces existing SIGALRM handler (preserved and restored in destructor)
 *       - Sets sa_flags = 0 (no SA_RESTART, syscalls will be interrupted)
 *       - Uses empty signal mask (sigemptyset)
 *       - Old handler and timer are restored in destructor
 */
class InterruptTimer
{
  private:
    std::optional<struct sigaction>  m_old_sa;
    std::optional<struct itimerval>  m_old_timer;

  public:
    InterruptTimer()
      : m_old_sa()
      , m_old_timer()
    {}

    ~InterruptTimer()
    {
      clean();
    }

    Value<void> start(std::chrono::milliseconds const& timeout)
    {
      // Validate timeout range
      if (timeout.count() <= 0)
      {
        return Error("E::Timeout must be positive (zero timeout disarms the timer)");
      }
      // Check for overflow on 32-bit systems (time_t might be 32-bit)
      // Convert to seconds first to avoid overflow in multiplication
      auto const timeout_seconds = timeout.count() / 1000;
      if (timeout_seconds > std::numeric_limits<time_t>::max())
      {
        return Error("E::Timeout too large: {} ms (max: {} seconds)"
          , timeout.count()
          , std::numeric_limits<time_t>::max()
        );
      }

      struct sigaction  old_sa;
      struct itimerval  old_timer;
      // save old handler
      if (sigaction(SIGALRM, nullptr, &old_sa) < 0)
      {
        return Error("E::Failed to save action: {}", strerror(errno));
      }
      else
      {
        m_old_sa = old_sa;
      }
      // save old timer
      if (getitimer(ITIMER_REAL, &old_timer) < 0)
      {
        return Error("E::Failed to save timer: {}", strerror(errno));
      }
      else
      {
        m_old_timer = old_timer;
      }

      // Set mask for novel action
      struct sigaction sa{};
      sa.sa_handler = alarm_handler;
      if (sigemptyset(&sa.sa_mask) < 0)
      {
        return Error("E::Failed to set mask: {}", strerror(errno));
      }
      // Install novel action
      sa.sa_flags = 0;
      if (sigaction(SIGALRM, &sa, nullptr) < 0)
      {
        return Error("E::Failed to set action: {}", strerror(errno));
      }
      // Arm new timer
      struct itimerval tm{};
      tm.it_value.tv_sec  = timeout.count()/1000; // Convert ms to seconds (e.g., 1500ms -> 1s)
      tm.it_value.tv_usec = (timeout.count()%1000)*1000; // Remainder as μs (e.g., 1500ms -> 500ms -> 500000μs)
      tm.it_interval.tv_sec = 0;  // One-shot timer: no repeat interval
      tm.it_interval.tv_usec = 0;
      if (setitimer(ITIMER_REAL, &tm, nullptr) < 0)
      {
        return Error("E::Could not arm timer: {}", strerror(errno));
      }
      return {};
    }

    void clean()
    {
      // restore old handler
      if(m_old_sa)
      {
        if (sigaction(SIGALRM, &m_old_sa.value(), nullptr) < 0)
        {
          logger("E::Failed to restore SIGALRM handler: {}", strerror(errno));
        }
        m_old_sa.reset();
      }
      // restore old timer
      if(m_old_timer)
      {
        if (setitimer(ITIMER_REAL, &m_old_timer.value(), nullptr) < 0)
        {
          logger("E::Failed to restore timer: {}", strerror(errno));
        }
        m_old_timer.reset();
      }
    }
};

}

/**
 * @brief Reads from the file descriptor with a timeout
 *
 * @tparam Data Type of data elements in the buffer
 * @param fd The file descriptor
 * @param timeout The timeout in std::chrono::milliseconds
 * @param buf The buffer in which to store the read data
 * @return ssize_t The number of read bytes or -1 and errno is set
 *
 * @note If timeout expires, read() is interrupted and returns -1 with errno = EINTR
 * @todo Make this return Value due to timer
 */
template<typename Data>
[[nodiscard]] inline ssize_t read_with_timeout(int fd
  , std::chrono::milliseconds const& timeout
  , std::span<Data> buf)
{
  using Element = typename std::decay_t<typename decltype(buf)::value_type>;
  InterruptTimer interrupt;
  if(auto ret = interrupt.start(timeout); not ret)
  {
    logger("E::{}", ret.error());
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
 * @return ssize_t The number of written bytes or -1 and errno is set
 *
 * @note If timeout expires, write() is interrupted and returns -1 with errno = EINTR
 * @todo Make this return Value due to timer
 */
template<typename Data>
[[nodiscard]] inline ssize_t write_with_timeout(int fd
  , std::chrono::milliseconds const& timeout
  , std::span<Data> buf)
{
  using Element = typename std::decay_t<typename decltype(buf)::value_type>;
  InterruptTimer interrupt;
  if(auto ret = interrupt.start(timeout); not ret)
  {
    logger("E::{}", ret.error());
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
 * @return int The file descriptor or -1 on error and errno is set
 *
 * @note If timeout expires, open() is interrupted and returns -1 with errno = EINTR
 * @todo Make this return Value due to timer
 */
[[nodiscard]] inline int open_with_timeout(
  fs::path const&            path_file_src,
  std::chrono::milliseconds  timeout,
  int                         oflag)
{
  InterruptTimer interrupt;
  if(auto ret = interrupt.start(timeout); not ret)
  {
    logger("E::{}", ret.error());
    return -1;
  }
  return ::open(path_file_src.c_str(), oflag);
}

/**
 * @brief Opens and reads from the given input file
 *
 * @tparam Data Type of data elements in the buffer
 * @param path_file_src Path to the file to open and read
 * @param timeout The timeout in std::chrono::milliseconds
 * @param buf The buffer in which to store the read data
 * @return ssize_t The number of bytes read or -1 on error
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
 * @return ssize_t The number of bytes written or -1 on error
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
