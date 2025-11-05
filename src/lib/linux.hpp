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
#include <sys/time.h>
#include <unistd.h>
#include <cassert>

#include "../macro.hpp"
#include "../std/expected.hpp"

namespace ns_linux
{

namespace
{

namespace fs = std::filesystem;

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

    Expected<void> start(std::chrono::milliseconds const& timeout)
    {
      struct sigaction  old_sa;
      struct itimerval  old_timer;
      // save old handler
      if (sigaction(SIGALRM, nullptr, &old_sa) < 0)
      {
        return std::unexpected(std::format("Failed to save action: {}", strerror(errno)));
      }
      else
      {
        m_old_sa = old_sa;
      }
      // save old timer
      if (setitimer(ITIMER_REAL, nullptr, &old_timer) < 0)
      {
        return std::unexpected(std::format("Failed to save timer: {}", strerror(errno)));
      }
      else
      {
        m_old_timer = old_timer;
      }

      // Set mask for novel action
      struct sigaction sa{};
      sa.sa_handler = [](int){};
      if (sigemptyset(&sa.sa_mask) < 0)
      {
        return std::unexpected(std::format("Failed to set mask: {}", strerror(errno)));
      }
      // Install novel action
      sa.sa_flags = 0;
      if (sigaction(SIGALRM, &sa, nullptr) < 0)
      {
        return std::unexpected(std::format("Failed to set action: {}", strerror(errno)));
      }
      // Arm new timer
      struct itimerval tm{};
      tm.it_value.tv_sec  = timeout.count()/1000; // integer seconds -> 1
      tm.it_value.tv_usec = (timeout.count()%1000)*1000; // remaining ms -> 500 ms, convert to μs -> 500 000 μs
      tm.it_interval.tv_sec = 0;
      tm.it_interval.tv_usec = 0;
      if (setitimer(ITIMER_REAL, &tm, nullptr) < 0)
      {
        return std::unexpected(std::format("Could not arm timer: {}", strerror(errno)));
      }
      return {};
    }

    void clean()
    {
      // restore old handler
      if(m_old_sa) { sigaction(SIGALRM, &m_old_sa.value(), nullptr); }
      // restore old timer
      if(m_old_timer) { setitimer(ITIMER_REAL, &m_old_timer.value(), nullptr); }
    }
};

}

/**
 * @brief Reads from the file descriptor or exits within a timeout
 *
 * @tparam Data Type of data elements in the buffer
 * @param fd The file descriptor
 * @param timeout The timeout in std::chrono::milliseconds
 * @param buf The buffer in which to store the read data
 * @return ssize_t The number of read bytes or -1 and errno is set
 *
 * @todo Make this return Expected due to timer
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
    ns_log::error()(ret.error());
    return -1;
  }
  return ::read(fd, buf.data(), buf.size()*sizeof(Element));
}

/**
 * @brief Opens a given file or exits within a timeout
 * 
 * @param path_file_src Path for the file to open
 * @param timeout The timeout in std::chrono::milliseconds
 * @param oflag The open flags O_*
 * @return int The file descriptor or -1 on error and errno is set
 *
 * @todo Make this return Expected due to timer
 */
[[nodiscard]] inline int open_with_timeout(
  fs::path const&            path_file_src,
  std::chrono::milliseconds  timeout,
  int                         oflag)
{
  InterruptTimer interrupt;
  if(auto ret = interrupt.start(timeout); not ret)
  {
    ns_log::error()(ret.error());
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
  qreturn_if(fd < 0, fd);
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
  using Element = typename std::decay_t<typename decltype(buf)::value_type>;
  int fd = open_with_timeout(path_file_src, timeout, O_WRONLY);
  qreturn_if(fd < 0, fd);
  ssize_t bytes_written = write(fd, buf.data(), buf.size()*sizeof(Element));
  close(fd);
  return bytes_written;
}

/**
 * @brief Checks if the linux kernel has a module loaded that matches the input name
 * 
 * @param str_name Name of the module to check for
 * @return Expected<bool> The boolean result, or the respective internal error
 */
[[nodiscard]] inline Expected<bool> module_check(std::string_view str_name)
{
  std::ifstream file_modules("/proc/modules");
  qreturn_if(not file_modules.is_open(), std::unexpected("Could not open modules file"));

  std::string line;
  while ( std::getline(file_modules, line) )
  {
    qreturn_if(line.contains(str_name), true);
  }

  return false;
}

} // namespace ns_linux

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
