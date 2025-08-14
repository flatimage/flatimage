///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : log
///

#pragma once

#include <filesystem>
#include <fstream>

#include "../common.hpp"
#include "../std/concept.hpp"

namespace ns_log
{

enum class Level : int
{
  CRITICAL,
  ERROR,
  INFO,
  DEBUG,
};

namespace
{

namespace fs = std::filesystem;

// class Logger {{{
class Logger
{
  private:
    std::optional<std::ofstream> m_opt_os;
    Level m_level;
  public:
    Logger();
    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger const&) = delete;
    Logger& operator=(Logger&&) = delete;
    void set_level(Level level);
    Level get_level() const;
    void set_sink_file(fs::path path_file_sink);
    void flush();
    std::optional<std::ofstream>& get_sink_file();
}; // class Logger }}}

// fn: Logger::Logger {{{
inline Logger::Logger()
  : m_opt_os(std::nullopt)
  , m_level(Level::CRITICAL)
{
} // fn: Logger::Logger }}}

// fn: Logger::set_sink_file {{{
inline void Logger::set_sink_file(fs::path path_file_sink)
{
  // File to save logs into
  if ( const char* var = std::getenv("FIM_DEBUG"); var && std::string_view{var} == "1" )
  {
    "Logger file: {}\n"_print(path_file_sink);
  } // if

  // File output stream
  m_opt_os = std::ofstream{path_file_sink};

  if( not m_opt_os->is_open() ) { throw std::runtime_error("Could not open file '{}'"_fmt(path_file_sink)); };
} // fn: Logger::set_sink_file }}}

// fn: Logger::get_sink_file {{{
inline std::optional<std::ofstream>& Logger::get_sink_file()
{
  return m_opt_os;
} // fn: Logger::get_sink_file }}}

// fn: Logger::flush {{{
inline void Logger::flush()
{
  if (auto& file = this->m_opt_os)
  {
    file->flush();
  }
} // fn: Logger::flush }}}

// fn: Logger::set_level {{{
inline void Logger::set_level(Level level)
{
  m_level = level;
} // fn: Logger::set_level }}}

// fn: Logger::get_level {{{
inline Level Logger::get_level() const
{
  return m_level;
} // fn: Logger::get_level }}}

thread_local Logger logger;

} // namespace

// fn: set_level {{{
inline void set_level(Level level)
{
  logger.set_level(level);
} // fn: set_level }}}

// fn: get_level {{{
inline Level get_level()
{
  return logger.get_level();
} // fn: get_level }}}

// fn: set_sink_file {{{
inline void set_sink_file(fs::path path_file_sink)
{
  logger.set_sink_file(path_file_sink);
} // fn: set_sink_file }}}

// class: Location {{{
class Location
{
  private:
    fs::path m_str_file;
    uint32_t m_line;

  public:
    Location(
        char const* str_file = __builtin_FILE()
      , uint32_t line = __builtin_LINE()
    )
      : m_str_file(fs::path(str_file).filename())
      , m_line(line)
    {}

    auto get() const
    {
      return "{}::{}"_fmt(m_str_file, m_line);
    } // get
}; // class: Location }}}

// class debug {{{
class debug
{
  private:
    Location m_loc;

  public:
    debug(Location location = {}) : m_loc(location) {}
    template<ns_concept::StringRepresentable T, typename... Args>
    requires ( ( ns_concept::StringRepresentable<Args> or ns_concept::IterableConst<Args> ) and ... )
    void operator()(T&& format, Args&&... args)
    {
      auto& opt_ostream_sink = logger.get_sink_file();
      print_if(opt_ostream_sink, *opt_ostream_sink, "D::{}::{}\n"_fmt(m_loc.get(), format), args...);
      print_if((logger.get_level() >= Level::DEBUG), std::cerr, "D::{}::{}\n"_fmt(m_loc.get(), format), std::forward<Args>(args)...);
      logger.flush();
    } // debug
}; // class debug }}}


// class info {{{
class info
{
  private:
    Location m_loc;

  public:
    info(Location location = {}) : m_loc(location) {}
    template<ns_concept::StringRepresentable T, typename... Args >
    requires ( ( ns_concept::StringRepresentable<Args> or ns_concept::IterableConst<Args> ) and ... )
    void operator()(T&& format, Args&&... args)
    {
      auto& opt_ostream_sink = logger.get_sink_file();
      print_if(opt_ostream_sink, *opt_ostream_sink, "I::{}::{}\n"_fmt(m_loc.get(), format), args...);
      print_if((logger.get_level() >= Level::INFO), std::cout, "I::{}::{}\n"_fmt(m_loc.get(), format), std::forward<Args>(args)...);
      logger.flush();
    } // info
}; // class info }}}

// class error {{{
class error
{
  private:
    Location m_loc;

  public:
    error(Location location = {}) : m_loc(location) {}
    template<ns_concept::StringRepresentable T, typename... Args>
    requires ( ( ns_concept::StringRepresentable<Args> or ns_concept::IterableConst<Args> ) and ... )
    void operator()(T&& format, Args&&... args)
    {
      auto& opt_ostream_sink = logger.get_sink_file();
      print_if(opt_ostream_sink, *opt_ostream_sink, "E::{}::{}\n"_fmt(m_loc.get(), format), args...);
      print_if((logger.get_level() >= Level::ERROR), std::cerr, "E::{}::{}\n"_fmt(m_loc.get(), format), std::forward<Args>(args)...);
      logger.flush();
    } // error
}; // class error }}}

// class critical {{{
class critical
{
  private:
    Location m_loc;

  public:
    critical(Location location = {}) : m_loc(location) {}
    template<ns_concept::StringRepresentable T, typename... Args>
    requires ( ( ns_concept::StringRepresentable<Args> or ns_concept::IterableConst<Args> ) and ... )
    void operator()(T&& format, Args&&... args)
    {
      auto& opt_ostream_sink = logger.get_sink_file();
      print_if(opt_ostream_sink, *opt_ostream_sink, "C::{}::{}\n"_fmt(m_loc.get(), format), args...);
      print_if((logger.get_level() >= Level::CRITICAL), std::cerr, "C::{}::{}\n"_fmt(m_loc.get(), format), std::forward<Args>(args)...);
      logger.flush();
    } // critical
}; // class critical }}}

// fn: ec {{{
template<typename F, typename... Args>
inline auto ec(F&& fn, Args&&... args) -> std::invoke_result_t<F, Args...>
{
  std::error_code ec;
  if constexpr ( std::same_as<void,std::invoke_result_t<F, Args...>> )
  {
    fn(std::forward<Args>(args)..., ec);
    if ( ec ) { ns_log::error()(ec.message()); } // if
    return;
  }
  else
  {
    auto ret = fn(std::forward<Args>(args)..., ec);
    if ( ec ) { ns_log::error()(ec.message()); } // if
    return ret;
  } // else
} // }}}

} // namespace ns_log

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
