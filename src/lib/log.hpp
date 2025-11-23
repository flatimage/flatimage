/**
 * @file log.hpp
 * @author Ruan Formigoni
 * @brief A library for file logging
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 *
 * ## Thread-Local Logger with Fork Safety
 *
 * This logger uses `thread_local` storage combined with `pthread_atfork()` to handle
 * both multi-threading and process forking correctly.
 *
 * ### Thread-Local Storage (`thread_local`)
 *
 * The `thread_local Logger logger` ensures that each thread in a process
 * gets its own independent Logger instance:
 *
 * - **Single-threaded process**: One logger instance
 * - **Multi-threaded process**: One logger instance per thread (isolated, no races)
 * - **Shared memory**: NOT shared between threads (unlike static/global variables)
 *
 * Benefits:
 * - No mutex/locking needed (each thread has private instance)
 * - No race conditions on logger state (m_sink, m_level)
 * - Each thread can log to different files if needed
 *
 * ### Fork Behavior
 *
 * When `fork()` is called, the child process inherits a copy of the parent's memory:
 *
 * **Without pthread_atfork (problematic):**
 * ```
 * Parent opens /tmp/parent.log  // FD 5 points to file
 * fork()
 * -> Parent: FD 5 still open
 * -> Child:  FD 5 still open (SAME file, SAME position, CORRUPTION!)
 * ```
 *
 * **With pthread_atfork (this implementation):**
 * ```
 * Parent opens /tmp/parent.log  // FD 5 points to file
 * fork()
 * -> pthread_atfork child handler runs automatically
 * -> fork_handler_child() calls logger.set_sink_file("/dev/null")
 * -> Closes FD 5 in child, opens /dev/null (new FD)
 * -> Parent: FD 5 still valid (reference count decremented but not closed)
 * -> Child:  FD 5 closed, new FD for /dev/null (clean slate)
 * ```
 *
 * ### pthread_atfork() Registration
 *
 * In the Logger constructor:
 * ```cpp
 * static thread_local bool registered = false;
 * if (!registered) {
 *   pthread_atfork(nullptr, nullptr, fork_handler_child);
 *   registered = true;
 * }
 * ```
 *
 * - Registered once per thread (static thread_local guard)
 * - `fork_handler_child` runs in child process after fork()
 * - Resets logger file to (/dev/null)
 * - Child can then set its own log file independently
 *
 * ### File Descriptor Semantics
 *
 * File descriptors are NOT shared memory - they're kernel objects with reference counts:
 *
 * - **Before fork**: Parent has FD → File (refcount = 1)
 * - **After fork**: Parent FD → File (refcount = 2), Child FD → File (refcount = 2)
 * - **Child closes FD**: Parent FD → File (refcount = 1), Child FD closed
 * - **File closed when**: Refcount reaches 0 (all processes close their FDs)
 *
 * Therefore, closing the file in the child does NOT affect the parent's ability to log.
 *
 * ### Non-Copyable, Non-Movable Design
 *
 * The Logger class is completely non-copyable and non-movable:
 * ```cpp
 * Logger(Logger const&) = delete;          // Deleted copy constructor
 * Logger& operator=(Logger const&) = delete; // Deleted copy assignment
 * Logger(Logger&&) = delete;               // Deleted move constructor
 * Logger& operator=(Logger&&) = delete;    // Deleted move assignment
 * ```
 *
 * - Each thread_local Logger instance is tied to its thread
 * - After fork(), `fork_handler_child()` calls `set_sink_file("/dev/null")`
 * - This closes the old file descriptor and opens /dev/null in the child
 * - No move/copy operations needed - just reopen the sink file
 *
 * ### Usage Example
 *
 * ```cpp
 * // Parent process
 * ns_log::set_sink_file("/tmp/parent.log");
 * ns_log::info()("Parent starting");
 *
 * pid_t pid = fork();
 *
 * if (pid == 0) {
 *   // Child: logger automatically reset to /dev/null
 *   ns_log::set_sink_file("/tmp/child.log");  // Safe! New file
 *   ns_log::info()("Child process");           // Logs to child.log
 * } else {
 *   // Parent: still logging to parent.log
 *   ns_log::info()("Parent forked child");    // Logs to parent.log
 * }
 * ```
 *
 * ### Thread Safety Summary
 *
 * | Scenario | Behavior | Thread-Safe? |
 * |----------|----------|--------------|
 * | Single thread | One logger instance | ✅ Yes |
 * | Multiple threads | Separate logger per thread | ✅ Yes (isolated) |
 * | Fork (parent) | Keeps existing logger | ✅ Yes |
 * | Fork (child) | Fresh logger (/dev/null) | ✅ Yes (auto-reset) |
 * | Fork then thread | Child's threads get new loggers | ✅ Yes |
 */

#pragma once

#include <pthread.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <print>
#include <ranges>
#include <unistd.h>

#include "../std/concept.hpp"
#include "../std/string.hpp"

/**
 * @namespace ns_log
 * @brief Multi-level logging system with file and stdout sinks
 *
 * Thread-local logging infrastructure supporting DEBUG, INFO, WARNING, ERROR, and CRITICAL
 * levels with automatic source location tracking. Provides dual output to log files and
 * stdout/stderr with level-based prefixes (D::, I::, W::, E::, C::) for convenient filtering
 * and categorization.
 */
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
    std::ofstream m_sink;
    Level m_level;
    pid_t m_pid;  ///< PID at logger initialization (used to detect forked children)
  public:
    Logger();
    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger const&) = delete;
    Logger& operator=(Logger&&) = delete;
    void set_level(Level level);
    [[nodiscard]] Level get_level() const;
    [[nodiscard]] pid_t get_pid() const;
    void set_sink_file(fs::path const& path_file_sink);
    void set_as_fork();
    void flush();
    [[nodiscard]] std::ofstream& get_sink_file();
};

/**
 * @brief Thread-local logger instance
 *
 * Each thread in the process gets its own independent Logger instance.
 * Automatically reset to default state after fork() via pthread_atfork().
 */
thread_local Logger logger;


/**
 * @brief Fork preparation handler that flushes all output buffers
 *
 * This function is registered as the prepare handler via pthread_atfork() and
 * automatically executes in the parent process BEFORE fork() happens. It ensures
 * all buffered output is written.
 *
 * The fflush(nullptr) call flushes ALL streams
 */
static void fork_handler_parent()
{
  std::cout.flush();
  std::cerr.flush();
  ::fflush(nullptr);
}

/**
 * @brief Fork handler that resets the logger in child processes
 *
 * This function is registered via pthread_atfork() and automatically executes
 * in child processes after fork(). It resets the logger to a fresh state with
 * /dev/null sink, preventing file descriptor sharing issues.
 *
 * NOTE: m_pid is intentionally NOT updated here. This allows child processes
 * to detect they are forks (getpid() != m_pid) and log their PID instead of
 * file location.
 */
void fork_handler_child()
{
  logger.set_sink_file("/dev/null");
}

/**
 * @brief Construct a new Logger:: Logger object
 *
 */
inline Logger::Logger()
  : m_sink("/dev/null")
  , m_level(Level::CRITICAL)
  , m_pid(getpid())
{
  static thread_local bool registered = false;
  if (!registered)
  {
    pthread_atfork(fork_handler_parent, nullptr, fork_handler_child);
    registered = true;
  }
}

/**
 * @brief Sets the sink file of the logger
 *
 * @param path_file_sink The path to the logger sink file
 */
inline void Logger::set_sink_file(fs::path const& path_file_sink)
{
  // File to save logs into
  if ( const char* var = std::getenv("FIM_DEBUG"); var && std::string_view{var} == "1" )
  {
    std::println("D::Logger file: {}", path_file_sink.string());
  }
  // File output stream
  m_sink = std::ofstream(path_file_sink, std::ios::out | std::ios::trunc);
  // Check if file was opened successfully
  if(not m_sink.is_open())
  {
    std::println("E::Could not open file '{}'", path_file_sink.string());
  }
}

/**
 * @brief Gets the sink file of the logger
 *
 * @return std::ofstream& The reference to the sink file
 */
inline std::ofstream& Logger::get_sink_file()
{
  return m_sink;
}

/**
 * @brief Flushes the buffer content to the log file if it is defined
 */
inline void Logger::flush()
{
  if (auto& file = this->m_sink)
  {
    file.flush();
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

/**
 * @brief Get the PID that was active when this logger was initialized
 *
 * This returns the PID from when the Logger constructor ran, which may differ
 * from the current PID if this is a forked child process. This enables fork
 * detection: if getpid() != get_pid(), we're in a child process.
 *
 * @return pid_t The original process ID (parent's PID in forked children)
 */
inline pid_t Logger::get_pid() const
{
  return m_pid;
}

/**
 * @brief Marks the logger as being in a forked child process
 *
 * Sets m_pid to an invalid value (0) so that getpid() will always differ
 * from it. This ensures log messages always display the current PID instead
 * of file location information, making it clear these logs come from a
 * separate child process.
 */
inline void Logger::set_as_fork()
{
  m_pid = 0;
}

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
inline void set_sink_file(fs::path const& path_file_sink)
{
  logger.set_sink_file(path_file_sink);
}

/**
 * @brief Marks the logger as being in a forked child process
 *
 * For processes that need to explicitly indicate they are children (e.g., pipe
 * readers forked from the parent), this function sets the logger's tracked PID
 * to an invalid value (0) so that subsequent log messages always display the
 * current process ID instead of file location information.
 *
 * **Usage Example:**
 * @code
 * pid_t pid = fork();
 * if (pid == 0) {
 *   // Child process
 *   ns_log::set_as_fork();  // Ensure logs show this process's PID
 *   ns_log::set_sink_file(log_path);
 *   logger("I::Starting in child process");
 * }
 * @endcode
 *
 * **Effect on Log Output:**
 * - **Before**: `I::boot.cpp::50::Message` (if m_pid matched current PID)
 * - **After**: `I::12345::Message` (always shows current PID since m_pid=0)
 *
 * **Note:** This is typically automatic after fork() + pthread_atfork handlers,
 * but can be called explicitly for clarity or in special circumstances.
 */
inline void set_as_fork()
{
  logger.set_as_fork();
}

/**
 * @brief Source code location information for log messages
 *
 * Automatically captures file name and line number at compile time using
 * __builtin_FILE() and __builtin_LINE(). The file path is trimmed to show
 * only the filename (no directory path).
 */
struct Location
{
  std::string_view m_str_file; ///< Source file name (basename only)
  uint32_t m_line;             ///< Source line number

  /**
   * @brief Constructs a Location with automatic file/line capture
   *
   * @param str_file Source file path (default: current file via __builtin_FILE())
   * @param str_line Source line number (default: current line via __builtin_LINE())
   */
  consteval Location(const char* str_file = __builtin_FILE() , uint32_t str_line = __builtin_LINE())
    : m_str_file(str_file)
    , m_line(str_line)
  {
    m_str_file = m_str_file.substr(m_str_file.find_last_of("/")+1);
  }

  /**
   * @brief Formats location as "filename::line"
   *
   * @return std::string Formatted location string
   */
  constexpr auto get() const
  {
    return std::format("{}::{}", m_str_file, m_line);
  }
};

/**
 * @brief Workaround make_format_args only taking references
 *
 * @tparam Ts Types of format arguments (variadic)
 * @param fmt Format of the string
 * @param ts Arguments to the input format
 * @return std::string The formatted string
 */
template<typename... Ts>
std::string vformat(std::string_view fmt, Ts&&... ts)
{
  return std::vformat(fmt, std::make_format_args(ts...));
}

/**
 * @brief Base class for level-specific log writers (debug, info, warn, error, critical)
 *
 * Handles the actual formatting and output of log messages to both the sink file
 * (if set) and the console (if level is enabled). Thread-safe due to thread_local logger.
 */
class Writer
{
  private:
    Location m_loc;                           ///< Source location of the log call
    Level m_level;                            ///< Minimum level required for console output
    std::string m_prefix;                     ///< Log level prefix (D, I, W, E, C)
    std::reference_wrapper<std::ostream> ostream; ///< Output stream (stdout or stderr)

  public:
    /**
     * @brief Constructs a Writer with location and level information
     *
     * @param location Source code location
     * @param level Log level for this writer
     * @param prefix Single-character prefix for log messages
     * @param ostream Output stream reference (std::cout or std::cerr)
     */
    Writer(Location const& location, Level const& level, std::string prefix, std::ostream& ostream)
      : m_loc(location)
      , m_level(level)
      , m_prefix(prefix)
      , ostream(ostream)
    {}
    /**
     * @brief Writes a formatted log message
     *
     * @tparam T Type of the format string (string representable)
     * @tparam Args Types of format arguments (variadic, string representable or iterable)
     * @param format The format string
     * @param args The format arguments
     */
    template<ns_concept::StringRepresentable T, typename... Args>
    requires ( ( ns_concept::StringRepresentable<Args> or ns_concept::Iterable<Args> ) and ... )
    void operator()(T&& format, Args&&... args)
    {
      // Get current PID to detect if this is a forked child process
      pid_t pid = getpid();
      // Create string
      std::string line;
      // Parent: show "PREFIX::file::line::", Child: show "PREFIX::child_pid::"
      line += (pid == logger.get_pid())? std::format("{}::{}::", m_prefix, m_loc.get())
        : std::format("{}::{}::", m_prefix, pid);
      // Append format and args
      line += vformat(ns_string::to_string(format), ns_string::to_string(args)...)
        // Remove new lines to avoid printing without a prefix
        | std::views::filter([](char c){ return c != '\n'; })
        // Make it into a string
        | std::ranges::to<std::string>();
      // Append a single newline
      line += '\n';
      // Push to file
      if(std::ofstream& sink = logger.get_sink_file(); sink.is_open())
      {
        sink << line;
      }
      // Push to stream
      if(logger.get_level() >= m_level)
      {
        ostream.get() << line;
      }
      logger.flush();
    }
};

/**
 * @brief Debug-level logger (lowest priority)
 *
 * Only outputs to console when logger level is set to Level::DEBUG.
 * Always writes to sink file if configured. Uses stdout.
 */
class debug final : public Writer
{
  private:
    Location m_loc;
  public:
    /**
     * @brief Constructs a debug logger
     * @param location Source location (automatically captured if not specified)
     */
    debug(Location location = Location())
      : Writer(location, Level::DEBUG, "D", std::cout)
      , m_loc(location)
    {}
};

/**
 * @brief Info-level logger (informational messages)
 *
 * Outputs to console when logger level is INFO or higher.
 * Always writes to sink file if configured. Uses stdout.
 */
class info : public Writer
{
  private:
    Location m_loc;
  public:
    /**
     * @brief Constructs an info logger
     * @param location Source location (automatically captured if not specified)
     */
    info(Location location = Location())
      : Writer(location, Level::INFO, "I", std::cout)
      , m_loc(location)
    {}
};

/**
 * @brief Warning-level logger (potential issues)
 *
 * Outputs to console when logger level is WARN or higher.
 * Always writes to sink file if configured. Uses stderr.
 */
class warn : public Writer
{
  private:
    Location m_loc;
  public:
    /**
     * @brief Constructs a warning logger
     * @param location Source location (automatically captured if not specified)
     */
    warn(Location location = Location())
      : Writer(location, Level::WARN, "W", std::cerr)
      , m_loc(location)
    {}
};

/**
 * @brief Error-level logger (recoverable errors)
 *
 * Outputs to console when logger level is ERROR or higher.
 * Always writes to sink file if configured. Uses stderr.
 */
class error : public Writer
{
  private:
    Location m_loc;
  public:
    /**
     * @brief Constructs an error logger
     * @param location Source location (automatically captured if not specified)
     */
    error(Location location = Location())
      : Writer(location, Level::ERROR, "E", std::cerr)
      , m_loc(location)
    {}
};

/**
 * @brief Critical-level logger (highest priority, always shown)
 *
 * Always outputs to console regardless of logger level.
 * Always writes to sink file if configured. Uses stderr.
 */
class critical : public Writer
{
  private:
    Location m_loc;
  public:
    /**
     * @brief Constructs a critical logger
     * @param location Source location (automatically captured if not specified)
     */
    critical(Location location = Location())
      : Writer(location, Level::CRITICAL, "C", std::cerr)
      , m_loc(location)
    {}
};

/**
 * @def logger(fmt, ...)
 * @brief Compile-time log level dispatch macro with automatic location capture
 *
 * This macro provides a convenient interface for logging with compile-time level selection
 * based on message prefix. The source file and line number are automatically captured.
 *
 * **Log Level Prefixes:**
 * - `D::` - Debug (lowest priority, only shown when level = DEBUG)
 * - `I::` - Info (informational messages)
 * - `W::` - Warning (potential issues, uses stderr)
 * - `E::` - Error (recoverable errors, uses stderr)
 * - `C::` - Critical (highest priority, always shown, uses stderr)
 * - `Q::` - Quiet (message discarded, useful for conditional compilation)
 *
 * **Format String:**
 * Uses std::format syntax with positional arguments: `{0}`, `{1}`, `{}`
 *
 * **Examples:**
 * @code
 * // Basic usage with different log levels
 * logger("I::Application started");
 * logger("D::Debug value: {}", 42);
 * logger("W::Connection timeout after {} seconds", 30);
 * logger("E::Failed to open file: {}", filename);
 * logger("C::Critical error: out of memory");
 *
 * // Multiple arguments
 * int x = 10, y = 20;
 * logger("I::Coordinates: ({}, {})", x, y);
 *
 * // Complex formatting
 * logger("D::Processing item {0} of {1} ({2:.2f}%)", current, total, percent);
 *
 * // Quiet logging (discarded)
 * logger("Q::This message will never appear");
 *
 * // With containers (requires Iterable concept)
 * std::vector<int> vec = {1, 2, 3};
 * logger("I::Vector contents: {}", vec);
 * @endcode
 *
 * **Output Format:**
 * ```
 * PREFIX::filename.cpp::LINE::Message
 * ```
 *
 * Example output:
 * ```
 * I::main.cpp::42::Application started
 * D::utils.cpp::128::Debug value: 42
 * W::network.cpp::256::Connection timeout after 30 seconds
 * ```
 *
 * @param fmt Format string with log level prefix (e.g., "I::Message: {}")
 * @param ... Variadic arguments for format string placeholders
 *
 * @note This macro requires C++23 for static_string template parameter
 * @see logger_loc for custom location specification
 * @see ns_log::set_level to control console output verbosity
 * @see ns_log::set_sink_file to enable file logging
 */
#define logger(fmt, ...) ::ns_log::impl_log<fmt>(::ns_log::Location{})(__VA_ARGS__)

/**
 * @def logger_loc(loc, fmt, ...)
 * @brief Compile-time log level dispatch macro with manual location specification
 *
 * Similar to logger() but allows specifying a custom source location. Useful for
 * logging from helper functions while preserving the original call site location.
 *
 * **Use Cases:**
 * - Wrapper functions that want to preserve caller location
 * - Logging from template instantiations
 * - Custom error handling macros
 *
 * **Examples:**
 * @code
 * // Capture location at call site, pass to helper function
 * void log_helper(ns_log::Location loc, std::string_view msg) {
 *     logger_loc(loc, "I::{}", msg);
 * }
 *
 * void user_function() {
 *     // Location captured here, not inside log_helper
 *     log_helper(ns_log::Location{}, "User function called");
 * }
 * // Output: I::main.cpp::15::User function called
 * //         (line 15 is user_function, not log_helper)
 *
 * // Custom location for testing/debugging
 * ns_log::Location custom_loc{"custom.cpp", 999};
 * logger_loc(custom_loc, "E::Simulated error from custom location");
 * // Output: E::custom.cpp::999::Simulated error from custom location
 *
 * // Macro that preserves location
 * #define MY_LOG(msg) logger_loc(ns_log::Location{}, "I::{}", msg)
 * MY_LOG("This shows the caller's location");
 * @endcode
 *
 * @param loc ns_log::Location instance with file/line information
 * @param fmt Format string with log level prefix (e.g., "I::Message: {}")
 * @param ... Variadic arguments for format string placeholders
 *
 * @note Location is consteval, so it must be constructed at compile time
 * @see logger for automatic location capture
 */
#define logger_loc(loc, fmt, ...) ::ns_log::impl_log<fmt>(loc)(__VA_ARGS__)

/**
 * @brief Implementation function for compile-time log level dispatch
 *
 * @tparam str Static string containing the log level prefix (D::, I::, W::, E::, C::, Q::)
 * @param loc Source location of the log call
 * @return Lambda that accepts variadic arguments and dispatches to the appropriate logger
 */
template<ns_string::static_string str>
constexpr decltype(auto) impl_log(Location loc)
{
  return [=]<typename... Ts>(Ts&&... ts)
  {
    constexpr std::string_view sv = str.data;
    if constexpr (sv.starts_with("I::"))
    {
      info{loc}(sv.substr(3), std::forward<Ts>(ts)...);
    }
    else if constexpr (sv.starts_with("W::"))
    {
      warn{loc}(sv.substr(3), std::forward<Ts>(ts)...);
    }
    else if constexpr (sv.starts_with("E::"))
    {
      error{loc}(sv.substr(3), std::forward<Ts>(ts)...);
    }
    else if constexpr (sv.starts_with("C::"))
    {
      critical{loc}(sv.substr(3), std::forward<Ts>(ts)...);
    }
    else if constexpr (sv.starts_with("Q::"))
    {
      // Discard
    }
    else
    {
      // It is necessary to repeat all conditions here due to how the compile-time language works
      static_assert(sv.starts_with("D::")
        or sv.starts_with("I::")
        or sv.starts_with("W::")
        or sv.starts_with("E::")
        or sv.starts_with("C::")
        or sv.starts_with("Q::")
      );
      ns_log::debug{loc}(sv.substr(3), std::forward<Ts>(ts)...);
    }
  };
}

} // namespace ns_log

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/