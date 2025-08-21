/**
 * @file log.hpp
 * @author Ruan Formigoni
 * @brief A library for file logging
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <fstream>

#include "../common.hpp"
#include "../std/concept.hpp"
#include "../std/string.hpp"

namespace ns_log
{

enum class Level : int
{
  CRITICAL,
  ERROR,
  WARN,
  INFO,
  DEBUG,
};

namespace
{

namespace fs = std::filesystem;

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
    [[nodiscard]] Level get_level() const;
    [[nodiscard]] inline std::expected<void,std::string> set_sink_file(fs::path path_file_sink);
    void flush();
    [[nodiscard]] std::optional<std::ofstream>& get_sink_file();
};

/**
 * @brief Construct a new Logger:: Logger object
 * 
 */
inline Logger::Logger()
  : m_opt_os(std::nullopt)
  , m_level(Level::CRITICAL)
{
}

/**
 * @brief Sets the sink file of the logger
 * 
 * @param path_file_sink The path to the logger sink file 
 */
[[nodiscard]] inline std::expected<void,std::string> Logger::set_sink_file(fs::path path_file_sink)
{
  // File to save logs into
  if ( const char* var = std::getenv("FIM_DEBUG"); var && std::string_view{var} == "1" )
  {
    "Logger file: {}\n"_print(path_file_sink);
  }
  // File output stream
  m_opt_os = std::ofstream(path_file_sink, std::ios::out | std::ios::trunc);
  // Check if file was opened successfully  
  if(not m_opt_os->is_open())
  {
    return std::unexpected("Could not open file '{}'"_fmt(path_file_sink));
  }
  return {};
}

/**
 * @brief Gets the sink file of the logger
 * 
 * @return std::optional<std::ofstream>& The reference to the sink file if it is defined
 */
inline std::optional<std::ofstream>& Logger::get_sink_file()
{
  return m_opt_os;
}

/**
 * @brief Flushes the buffer content to the log file if it is defined
 */
inline void Logger::flush()
{
  if (auto& file = this->m_opt_os)
  {
    file->flush();
  }
}

/**
 * @brief Sets the logging verbosity (CRITICAL,ERROR,INFO,DEBUG)
 * 
 * @param level Enumeration of the verbosity level
 */
inline void Logger::set_level(Level level)
{
  m_level = level;
}

/**
 * @brief Get current verbosity level of the logger
 * 
 * @return Level 
 */
inline Level Logger::get_level() const
{
  return m_level;
}

thread_local Logger logger;

}

/**
 * @brief Sets the logging verbosity (CRITICAL,ERROR,INFO,DEBUG)
 * 
 * @param level Enumeration of the verbosity level
 */
inline void set_level(Level level)
{
  logger.set_level(level);
}

/**
 * @brief Get current verbosity level of the logger
 * 
 * @return Level 
 */
inline Level get_level()
{
  return logger.get_level();
}

/**
 * @brief Sets the sink file of the logger
 * 
 * @param path_file_sink The path to the logger sink file 
 */
inline std::expected<void,std::string> set_sink_file(fs::path path_file_sink)
{
  return logger.set_sink_file(path_file_sink);
}

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
    }
};

/**
 * @brief Workaround make_format_args only taking references
 * 
 * @param fmt Format of the string
 * @param ts Arguments to the input format
 * @return std::string The formatted string
 */
template<typename... Ts>
std::string vformat(std::string fmt, Ts&&... ts)
{
  return std::vformat(fmt, std::make_format_args(ts...));
}

// Writer
class Writer
{
  private:
    Location m_loc;
    Level m_level;
    std::string m_prefix;

  public:
    Writer(Location const& location, Level const& level, std::string prefix)
      : m_loc(location)
      , m_level(level)
      , m_prefix(prefix)
    {}
    template<ns_concept::StringRepresentable T, typename... Args>
    requires ( ( ns_concept::StringRepresentable<Args> or ns_concept::IterableConst<Args> ) and ... )
    void operator()(T&& format, Args&&... args)
    {
      auto& opt_ostream_sink = logger.get_sink_file();
      if(opt_ostream_sink)
      {
        opt_ostream_sink.value()
          << "{}::{}::"_fmt(m_prefix, m_loc.get())
          << vformat(ns_string::to_string(format), ns_string::to_string(args)...)
          << '\n';
      }
      if(logger.get_level() >= m_level)
      {
        std::cerr
          << "{}::{}::"_fmt(m_prefix, m_loc.get())
          << vformat(ns_string::to_string(format), ns_string::to_string(args)...)
          << '\n';
      }
      logger.flush();
    }
};

class debug final : public Writer
{
  private:
    Location m_loc;
  public:
    debug(Location location = {})
      : Writer(location, Level::DEBUG, "D")
      , m_loc(location)
    {}
};

class info : public Writer
{
  private:
    Location m_loc;
  public:
    info(Location location = {})
      : Writer(location, Level::INFO, "I")
      , m_loc(location)
    {}
};

class warn : public Writer
{
  private:
    Location m_loc;
  public:
    warn(Location location = {})
      : Writer(location, Level::WARN, "W")
      , m_loc(location)
    {}
};

class error : public Writer
{
  private:
    Location m_loc;
  public:
    error(Location location = {})
      : Writer(location, Level::ERROR, "E")
      , m_loc(location)
    {}
};

class critical : public Writer
{
  private:
    Location m_loc;
  public:
    critical(Location location = {})
      : Writer(location, Level::CRITICAL, "C")
      , m_loc(location)
    {}
};

} // namespace ns_log

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
