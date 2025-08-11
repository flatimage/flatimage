///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : linux
///

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

#include "../common.hpp"
#include "../macro.hpp"

namespace ns_linux
{

namespace
{

namespace fs = std::filesystem;

struct InterruptTimer
{
  struct sigaction  old_sa{};
  struct itimerval  old_timer{};

  InterruptTimer(std::chrono::milliseconds const& timeout)
  {
    // save old handler
    if (sigaction(SIGALRM, nullptr, &old_sa) < 0)
    {
      throw std::runtime_error(std::format("Failed to save action: {}", strerror(errno)));
    }
    // save old timer
    if (setitimer(ITIMER_REAL, nullptr, &old_timer) < 0)
    {
      throw std::runtime_error(std::format("Failed to save timer: {}", strerror(errno)));
    }
    
    try
    {
      // Set mask for novel action
      struct sigaction sa{};
      sa.sa_handler = [](int){};
      if (sigemptyset(&sa.sa_mask) < 0)
      {
        throw std::runtime_error(std::format("Failed to set mask: {}", strerror(errno)));
      }
      // Install novel action
      sa.sa_flags = 0;
      if (sigaction(SIGALRM, &sa, nullptr) < 0)
      {
        throw std::runtime_error(std::format("Failed to set action: {}", strerror(errno)));
      }
      // Arm new timer
      struct itimerval tm{};
      tm.it_value.tv_sec  = timeout.count()/1000; // integer seconds -> 1
      tm.it_value.tv_usec = (timeout.count()%1000)*1000; // remaining ms -> 500 ms, convert to μs -> 500 000 μs
      tm.it_interval.tv_sec = 0;
      tm.it_interval.tv_usec = 0;
      if (setitimer(ITIMER_REAL, &tm, nullptr) < 0)
      {
        throw std::runtime_error(std::format("Could not arm timer: {}", strerror(errno)));
      }
    }
    catch (...)
    {
      clean();
    }
  }

  ~InterruptTimer()
  {
    clean();
  }
  
  void clean()
  {
    
    // restore old handler
    sigaction(SIGALRM, &old_sa, nullptr);
    // restore old timer
    setitimer(ITIMER_REAL, &old_timer, nullptr);
  }
};

}

// read_with_timeout() {{{
template<typename Data>
[[nodiscard]] inline ssize_t read_with_timeout(int fd
  , std::chrono::milliseconds const& timeout
  , std::span<Data> buf)
{
  using Element = typename std::decay_t<typename decltype(buf)::value_type>;
  InterruptTimer interrupt(timeout);
  return ::read(fd, buf.data(), buf.size()*sizeof(Element));
} // function: read_with_timeout }}}

// open_with_timeout() {{{
[[nodiscard]] inline int open_with_timeout(
  fs::path const&            path_file_src,
  std::chrono::milliseconds  timeout,
  int                         oflag)
{
  InterruptTimer interrupt(timeout);
  return ::open(path_file_src.c_str(), oflag);
}
// open_with_timeout() }}}

// open_read_with_timeout() {{{
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
} // function: open_read_with_timeout }}}

// open_write_with_timeout() {{{
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
} // function: open_write_with_timeout }}}

// mkdtemp() {{{
// Creates a temporary directory in path_dir_parent with the template provided by 'dir_template'
[[nodiscard]] inline fs::path mkdtemp(fs::path const& path_dir_parent, std::string dir_template = "XXXXXX")
{
  std::error_code ec;
  fs::create_directories(path_dir_parent, ec);
  ethrow_if(ec, "Failed to create temporary dir {}: '{}'"_fmt(path_dir_parent, ec.message()));
  fs::path path_dir_template = path_dir_parent / dir_template;
  const char* cstr_path_dir_temp = ::mkdtemp(path_dir_template.string().data()); 
  ethrow_if(cstr_path_dir_temp == nullptr, "Failed to create temporary dir {}"_fmt(path_dir_template));
  return cstr_path_dir_temp;
} // function: mkdtemp }}}

// mkstemp() {{{
[[nodiscard]] inline Expected<fs::path> mkstemps(fs::path const& path_dir_parent
  , std::string file_template = "XXXXXX"
  , int suffixlen = 0
)
{
  std::string str_template = path_dir_parent / file_template;
  int fd = ::mkstemps(str_template.data(), suffixlen);
  qreturn_if(fd < 0, Unexpected(strerror(errno)));
  close(fd);
  return fs::path{str_template};
}
// mkstemp() }}}

// module_check() {{{
inline Expected<bool> module_check(std::string_view str_name)
{
  std::ifstream file_modules("/proc/modules");
  qreturn_if(not file_modules.is_open(), Unexpected("Could not open modules file"));

  std::string line;
  while ( std::getline(file_modules, line) )
  {
    qreturn_if(line.contains(str_name), true);
  } // while

  return false;
} // function: module_check() }}}

// search_path() {{{
inline Expected<fs::path> search_path(fs::path const& query)
{
  char const * env_path = std::getenv("PATH");
  qreturn_if( env_path == nullptr, Unexpected("PATH environment variable not found"));

  char const * env_dir_global_bin = std::getenv("FIM_DIR_GLOBAL_BIN");
  char const * env_dir_static = std::getenv("FIM_DIR_STATIC");

  if ( query.is_absolute() )
  {
    return_if_else(fs::exists(query), query, Unexpected("File not found through absolute path"));
  } // if

  std::string path(env_path);
  std::istringstream istream_path(path);
  std::string str_getline;

  while (std::getline(istream_path, str_getline, ':'))
  {
    fs::path path_parent = str_getline;
    qcontinue_if(env_dir_static and path_parent == fs::path(env_dir_static));
    qcontinue_if(env_dir_global_bin and path_parent == fs::path(env_dir_global_bin));
    fs::path path_full = path_parent / query;
    qreturn_if(fs::exists(path_full), path_full);
  } // while

  return Unexpected("File not found in PATH");
} // search_path() }}}

} // namespace ns_linux

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
