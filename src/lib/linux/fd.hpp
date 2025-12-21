/**
 * @file fd.hpp
 * @author Ruan Formigoni
 * @brief File descriptor redirection helpers
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <fcntl.h>
#include <sys/types.h>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <span>

#include "../linux.hpp"

namespace ns_linux::ns_fd
{

constexpr uint32_t const SECONDS_TIMEOUT = 5;
constexpr uint32_t const SIZE_BUFFER_READ = 16384;
constexpr auto const TIMEOUT_RETRY = std::chrono::milliseconds(50);

/**
 * @brief Redirects the output of one file descriptor as input of another
 *
 * @param ppid Keep trying to read and write while this pid is alive
 * @param fd_src File descriptor to read from
 * @param fd_dst File descriptor to write to
 */
[[nodiscard]] inline Value<void> redirect_fd_to_fd(pid_t ppid, int fd_src, int fd_dst)
{
  // Validate file descriptors
  return_if (ppid < 0, Error("E::Invalid pid to wait for: {}", ppid));
  return_if (fd_src < 0, Error("E::Invalid source file descriptor: {}", fd_src));
  return_if (fd_dst < 0, Error("E::Invalid destination file descriptor: {}", fd_dst));
  // Read lambda
  auto f_rw = [](int fd_src, int fd_dst) -> Value<bool>
  {
    alignas(16) char buf[SIZE_BUFFER_READ];

    ssize_t n = ns_linux::read_with_timeout(fd_src
      , std::chrono::milliseconds(100)
      , std::span(buf, sizeof(buf))
    );
    // EOF
    if(n == 0)
    {
      return false;
    }
    // Possible non-recoverable error
    else if(n < 0)
    {
      // Timeout or would block - keep trying
      if((errno == EAGAIN) or (errno == ETIMEDOUT))
      {
        return true;
      }
      // Non-recoverable error
      return Error("E::Failed to read from file descriptor '{}' with error '{}'"
        , fd_src
        , strerror(errno)
      );
    }
    // n > 0, try to write read data
    return_if(::write(fd_dst, buf, n) < 0
      , Error("D::Could not write to file descriptor '{}' with error '{}'", fd_dst, strerror(errno))
    );
    return true;
  };
  // Try to read from the src fifo with 50ms delays
  while (::kill(ppid, 0) == 0 and Pop(f_rw(fd_src, fd_dst)))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  // After the process exited, check for any leftover output
  std::ignore = Pop(f_rw(fd_src, fd_dst));
  return {};
}

/**
 * @brief Redirects the output of a file to a file descriptor
 *
 * @param ppid Keep trying to read and write while this pid is alive
 * @param path_file_file Path to the file to read from
 * @param fd_dst Path to the file descriptor to write to
 * @return std::jthread Returns the spawned redirector thread
 */
[[nodiscard]] inline Value<void> redirect_file_to_fd(pid_t ppid, fs::path const& path_file, int fd_dst)
{
  // Try to open file within a timeout
  int fd_src = ns_linux::open_with_timeout(path_file.c_str()
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , O_RDONLY
  );
  if (fd_src == -1)
  {
    return Error("E::Failed to open file '{}' with error '{}'", path_file, strerror(errno));
  }
  // Redirect file to stdout
  Pop(redirect_fd_to_fd(ppid, fd_src, fd_dst));
  // Close file
  close(fd_src);
  return {};
}


/**
 * @brief Redirects the output of a file descriptor to a file
 *
 * @param ppid Keep trying to read and write while this pid is alive
 * @param fd_src Path to the file descriptor to read from
 * @param path_file_file Path to the file to write to
 * @return std::jthread Returns the spawned redirector thread
 */
[[nodiscard]] inline Value<void> redirect_fd_to_file(pid_t ppid
  , int fd_src
  , fs::path const& path_file)
{
    // Try to open file within a timeout
    int fd_dst = ns_linux::open_with_timeout(path_file.c_str()
      , std::chrono::seconds(SECONDS_TIMEOUT)
      , O_WRONLY
    );
    if (fd_dst < 0)
    {
      return Error("E::Failed to open file '{}' with error '{}'", path_file, strerror(errno));
    }
    // Redirect file to stdout
    Pop(redirect_fd_to_fd(ppid, fd_src, fd_dst));
    // Close file
    close(fd_dst);
    return {};
}

/**
 * @brief Redirects the output of a file descriptor to a stream
 *
 * Reads data from a file descriptor and writes it to an output stream.
 * Continues reading while the specified process is alive.
 *
 * @param ppid Keep trying to read and write while this pid is alive
 * @param fd_src File descriptor to read from
 * @param stream_dst Output stream to write to
 * @param transform Optionally transforms the read data before writing to the stream
 */
[[nodiscard]] inline Value<void> redirect_fd_to_stream(pid_t ppid
  , int fd_src
  , std::ostream& stream_dst
  , std::function<std::string(std::string const&)> transform = [](std::string e) -> std::string { return e; })
{
  // Validate file descriptors
  return_if(fd_src < 0, Error("E::Invalid src file descriptor"));
  // aligas is used because this gives out SIGILL when allocating the buffer
  // if the memory is un-aligned. Pain.
  for (alignas(16) char buf[SIZE_BUFFER_READ]; ::kill(ppid, 0) == 0;)
  {
    ssize_t n = ns_linux::read_with_timeout(fd_src, TIMEOUT_RETRY, std::span(buf, sizeof(buf)));
    if (n > 0)
    {
      // Split on both newlines and carriage returns, filter empty/whitespace-only lines
      std::string chunk(buf, n);
      // Replace all \r with \n to handle Windows-style line endings and progress updates
      std::ranges::replace(chunk, '\r', '\n');
      // Split by newline and process each line
      std::ranges::for_each(chunk
          | std::views::split('\n')
          | std::views::transform([](auto&& e){ return std::string{e.begin(), e.end()}; })
          | std::views::filter([](auto const& s){ return not s.empty(); })
          | std::views::filter([](auto const& s){ return not std::ranges::all_of(s, ::isspace); })
          | std::views::transform([&](auto&& e){ return transform(e); })
        , [&](auto line) { stream_dst << line << '\n'; }
      );
      stream_dst.flush();
    }
    // If the fd was given EOF (closed), then stop.
    if(n == 0)
    {
      break;
    }
    // Possible error when n < 0 that is not a timeout nor a retry
    return_if(n < 0 and (errno != EAGAIN) and (errno != ETIMEDOUT)
      , Error("E::Failed to read from file descriptor '{}' with error '{}'", fd_src, strerror(errno));
    );
    // Retry
    std::this_thread::sleep_for(TIMEOUT_RETRY);
  }
  return {};
}

/**
 * @brief Redirects the output of a stream to a file descriptor
 *
 * Reads lines from an input stream and writes them to a file descriptor.
 * Continues reading while the specified process is alive.
 *
 * @param ppid Keep trying to read and write while this pid is alive
 * @param stream_src Input stream to read from
 * @param fd_dst File descriptor to write to
 */
[[nodiscard]] inline Value<void> redirect_stream_to_fd(pid_t ppid, std::istream& stream_src, int fd_dst)
{
  using namespace std::chrono_literals;
  // Align to avoid SIGILL
  alignas(16) char buf[SIZE_BUFFER_READ];
  // Validate file descriptors
  return_if(fd_dst < 0, Error("E::Invalid src file descriptor"));
  // Query stream for data & forward to fd
  for(; ::kill(ppid, 0) == 0; std::this_thread::sleep_for(TIMEOUT_RETRY))
  {
    // Use a non-blocking approach, checking if there's data available to read.
    // in_avail: Returns the number of characters available for non-blocking
    // read (either the size of the get area or the number of characters ready
    // for reading from the associated character sequence), or -1 if no
    // characters are available in the associated sequence as far as showmanyc()
    // can tell.
    if (std::streamsize avail = stream_src.rdbuf()->in_avail(); avail > 0)
    {
      // Read up to available or buffer size
      std::streamsize to_read = std::min(avail, static_cast<std::streamsize>(sizeof(buf)));
      stream_src.read(buf, to_read);
      // How much was actually read
      std::streamsize n = stream_src.gcount();
      // The return of gcount is the number of read bytes by the last unformatted input operation
      // which includes basic_istream::read. Regarding negative values:
      // "Except in the constructors of std::strstreambuf, negative values of std::streamsize are never used."
      continue_if (n <= 0);
      // Write to file descriptor
      ssize_t bytes = ns_linux::write_with_timeout(fd_dst, 1s, std::span(buf, n));
      return_if (bytes < 0
        , Error("E::Could not write data to file descriptor '{}': {}", fd_dst, strerror(errno));
      );
    }
    // Custom stringstream streams with have eof on no data to read
    // instead of hanging like std::cin
    if (stream_src.eof())
    {
      stream_src.clear();
    }
  }
  return {};
}

} // namespace ns_linux::ns_fd