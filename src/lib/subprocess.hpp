/**
 * @file subprocess.hpp
 * @author Ruan Formigoni
 * @brief A library to spawn sub-processes in linux
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstring>
#include <functional>
#include <sys/wait.h>
#include <csignal>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ranges>
#include <filesystem>
#include <utility>
#include <memory>

#include "log.hpp"
#include "../macro.hpp"
#include "../std/vector.hpp"
#include "subprocess/pipe.hpp"
#include "subprocess/child.hpp"

/**
 * @namespace ns_subprocess
 * @brief Child process management and execution
 *
 * Provides comprehensive subprocess spawning and management with configurable stdio redirection
 * (Pipe/Null/Inherit), environment variable injection, death signal configuration (PR_SET_PDEATHSIG),
 * daemon mode, and log file integration. Supports both synchronous and asynchronous process
 * execution with proper cleanup.
 */
namespace ns_subprocess
{

/**
 * @brief Stream redirection modes for child process stdio
 */
enum class Stream
{
  Inherit,  // Child inherits parent's stdin/stdout/stderr
  Pipe,     // Redirect to pipes with callbacks
  Null,     // Redirect to /dev/null (silent)
};

/**
 * @brief Arguments passed to child callback
 */
struct ArgsCallbackChild
{
  pid_t parent_pid;  // Parent process PID
  int stdin_fd;      // Standard input file descriptor (usually 0)
  int stdout_fd;     // Standard output file descriptor (usually 1)
  int stderr_fd;     // Standard error file descriptor (usually 2)
};

/**
 * @brief Arguments passed to parent callback
 */
struct ArgsCallbackParent
{
  pid_t child_pid;   // Child process PID
};

/**
 * @brief Custom stream redirection for child process stdio
 */
namespace stream
{

/**
* @brief Redirects to /dev/null (silent)
*
* @return std::fstream& Reference to the null stream
*/
inline std::fstream& null()
{
  static std::fstream null("/dev/null", std::ios::in | std::ios::out);
  return null;
}

}

/**
 * @brief Converts a vector of strings to a null-terminated C-style array for execve
 *
 * @param vec Vector of strings to convert
 * @return std::unique_ptr<const char*[]> Null-terminated array of C-strings
 */
inline std::unique_ptr<const char*[]> to_carray(std::vector<std::string> const& vec)
{
  auto arr = std::make_unique<const char*[]>(vec.size() + 1);
  std::ranges::transform(vec, arr.get(), [](auto const& e) { return e.c_str(); });
  arr[vec.size()] = nullptr;
  return arr;
}

class Subprocess
{
  private:
    std::filesystem::path m_program;
    std::vector<std::string> m_args;
    std::vector<std::string> m_env;
    std::reference_wrapper<std::istream> m_stdin;
    std::reference_wrapper<std::ostream> m_stdout;
    std::reference_wrapper<std::ostream> m_stderr;
    Stream m_stream_mode;
    std::optional<pid_t> m_die_on_pid;
    std::filesystem::path m_path_file_log;
    ns_log::Level m_log_level;
    std::optional<std::function<void(ArgsCallbackChild)>> m_callback_child;
    std::optional<std::function<void(ArgsCallbackParent)>> m_callback_parent;
    bool m_daemon_mode;

    void die_on_pid(pid_t pid);
    void to_dev_null();
    void setup_pipes(pid_t child_pid
      , int pipestdin[2]
      , int pipestdout[2]
      , int pipestderr[2]
      , std::filesystem::path const& path_file_log);
    [[noreturn]] void exec_child();

  public:
    template<ns_concept::StringRepresentable T>
    [[nodiscard]] Subprocess(T&& t);
    ~Subprocess();

    Subprocess(Subprocess const&) = delete;
    Subprocess& operator=(Subprocess const&) = delete;

    Subprocess(Subprocess&&) = delete;
    Subprocess& operator=(Subprocess&&) = delete;


    [[maybe_unused]] [[nodiscard]] Subprocess& env_clear();

    template<ns_concept::StringRepresentable K, ns_concept::StringRepresentable V>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_var(K&& k, V&& v);

    template<ns_concept::StringRepresentable K>
    [[maybe_unused]] [[nodiscard]] Subprocess& rm_var(K&& k);

    template<typename... Args>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_args(Args&&... args);

    template<typename... Args>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_env(Args&&... args);

    [[maybe_unused]] [[nodiscard]] Subprocess& with_die_on_pid(pid_t pid);

    [[maybe_unused]] [[nodiscard]] Subprocess& with_stdio(Stream mode);

    [[maybe_unused]] [[nodiscard]] Subprocess& with_log_file(std::filesystem::path const& path);

    [[maybe_unused]] [[nodiscard]] Subprocess& with_log_level(ns_log::Level const& level);

    [[maybe_unused]] [[nodiscard]] Subprocess& with_streams(std::istream& stdin_stream, std::ostream& stdout_stream, std::ostream& stderr_stream);

    template<typename F>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_callback_child(F&& f);

    template<typename F>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_callback_parent(F&& f);

    [[maybe_unused]] [[nodiscard]] Subprocess& with_daemon();

    [[maybe_unused]] [[nodiscard]] std::unique_ptr<Child> spawn();
};

/**
 * @brief Construct a new Subprocess object with a program path
 *
 * Creates a subprocess object configured to run the specified program.
 * The environment is inherited from the parent process by default.
 *
 * @tparam T Type that is string representable
 * @param t The program path or name to execute
 *
 * @code
 * // Using string literal
 * Subprocess proc1("/bin/ls");
 *
 * // Using std::string
 * std::string program = "/usr/bin/gcc";
 * Subprocess proc2(program);
 *
 * // Using filesystem::path
 * std::filesystem::path exe = "/usr/bin/python3";
 * Subprocess proc3(exe);
 * @endcode
 */
template<ns_concept::StringRepresentable T>
Subprocess::Subprocess(T&& t)
  : m_program(ns_string::to_string(t))
  , m_args()
  , m_env()
  , m_stdin(stream::null())
  , m_stdout(stream::null())
  , m_stderr(stream::null())
  , m_stream_mode(Stream::Inherit)
  , m_die_on_pid(std::nullopt)
  , m_path_file_log("/dev/null")
  , m_log_level(ns_log::get_level())
  , m_callback_child(std::nullopt)
  , m_callback_parent(std::nullopt)
  , m_daemon_mode(false)
{
  // argv0 is program name
  m_args.push_back(m_program);
  // Copy environment
  for(char** i = environ; *i != nullptr; ++i)
  {
    m_env.push_back(*i);
  } // for
}

/**
 * @brief Destroy the Subprocess object
 *
 * Note: After spawn() is called, the Child handle takes ownership
 * of the child process. The Subprocess destructor does not wait for the child
 * since that responsibility has been transferred to Child.
 *
 * @code
 * {
 *     Subprocess proc("/bin/sleep");
 *     auto spawned = proc.with_args("5").spawn();
 *     // Child destructor will wait
 * } // spawned's destructor waits here, not proc's
 * @endcode
 */
inline Subprocess::~Subprocess()
{
}

/**
 * @brief Clears all environment variables before starting the process
 *
 * Removes all inherited environment variables, creating a clean slate.
 * Useful for when you need precise control over the environment.
 *
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Start process with only specific environment variables
 * Subprocess proc("/usr/bin/env");
 * proc.env_clear()                    // Remove all inherited vars
 *     .with_env("PATH=/usr/bin")      // Add only what we need
 *     .with_env("HOME=/tmp")
 *     .spawn();
 *
 * // Secure execution with minimal environment
 * Subprocess secure("/bin/untrusted_app");
 * secure.env_clear()
 *       .with_env("SAFE_VAR=value")
 *       .spawn();
 * @endcode
 */
inline Subprocess& Subprocess::env_clear()
{
  m_env.clear();
  return *this;
}

/**
 * @brief Adds or replaces a single environment variable
 *
 * Sets an environment variable by key-value pair. If the key already exists,
 * it is replaced with the new value. More convenient than with_env() when
 * you have separate key and value.
 *
 * @tparam K Type that is string representable for the key
 * @tparam V Type that is string representable for the value
 * @param k The name of the environment variable
 * @param v The value of the environment variable
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Add environment variables by key-value pairs
 * Subprocess proc("/usr/bin/python3");
 * proc.with_var("PYTHONPATH", "/usr/lib/python")
 *     .with_var("DEBUG", "1")
 *     .with_var("HOME", "/home/user")
 *     .spawn();
 *
 * // Replace existing variable
 * proc.with_var("PATH", "/usr/bin");        // First value
 * proc.with_var("PATH", "/usr/local/bin");  // Replaces previous
 *
 * // Using different types
 * std::string key = "PORT";
 * int value = 8080;
 * proc.with_var(key, value);  // PORT=8080
 * @endcode
 */
template<ns_concept::StringRepresentable K, ns_concept::StringRepresentable V>
Subprocess& Subprocess::with_var(K&& k, V&& v)
{
  std::ignore = rm_var(k);
  m_env.push_back(std::format("{}={}", k, v));
  return *this;
}

/**
 * @brief Removes an environment variable from the process environment
 *
 * Deletes the specified environment variable if it exists. If the variable
 * doesn't exist, this is a no-op (safe to call multiple times).
 *
 * @tparam K Type that is string representable
 * @param k The name of the variable to remove
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Remove potentially dangerous environment variables
 * Subprocess proc("/bin/app");
 * proc.rm_var("LD_PRELOAD")
 *     .rm_var("LD_LIBRARY_PATH")
 *     .rm_var("DYLD_INSERT_LIBRARIES")
 *     .spawn();
 *
 * // Remove inherited variable before setting custom value
 * proc.rm_var("PATH")
 *     .with_var("PATH", "/usr/local/bin:/usr/bin");
 *
 * // Safe to remove non-existent variables
 * proc.rm_var("DOES_NOT_EXIST");  // No error
 * @endcode
 */
template<ns_concept::StringRepresentable K>
Subprocess& Subprocess::rm_var(K&& k)
{
  // Find variable
  auto it = std::ranges::find_if(m_env, [&](std::string const& e)
  {
    auto vec = ns_vector::from_string(e, '=');
    return_if(vec.empty(), false);
    return vec.front() == k;
  });

  // Erase if found
  if ( it != std::ranges::end(m_env) )
  {
    logger("D::Erased var entry: {}", *it);
    m_env.erase(it);
  } // if

  return *this;
}

/**
 * @brief Arguments forwarded as the process' arguments
 *
 * Accepts any number of arguments (including zero). Each argument can be:
 * - A single string/string-like value (added as one argument)
 * - An iterable container (each element added as separate argument)
 *
 * @tparam Args Types of arguments (variadic, can be string, iterable, or string representable)
 * @param args Arguments to forward to the process
 * @return Subprocess& A reference to *this
 *
 * @code
 * // Single argument
 * .with_args("--verbose")
 *
 * // Multiple arguments
 * .with_args("--input", "file.txt", "--output", "out.txt")
 *
 * // Container of arguments
 * std::vector<std::string> opts = {"--flag1", "--flag2"};
 * .with_args(opts)
 *
 * // Mixed
 * .with_args("gcc", opts, "-o", "output")
 * @endcode
 */
template<typename... Args>
Subprocess& Subprocess::with_args(Args&&... args)
{
  // Helper lambda to add a single argument
  auto add_arg = [this]<typename T>(T&& arg) -> void
  {
    if constexpr ( ns_concept::Uniform<T, std::string> )
    {
      this->m_args.push_back(std::forward<T>(arg));
    }
    else if constexpr ( ns_concept::Container<T> )
    {
      std::copy(arg.begin(), arg.end(), std::back_inserter(m_args));
    }
    else if constexpr ( ns_concept::StringRepresentable<T> )
    {
      this->m_args.push_back(ns_string::to_string(std::forward<T>(arg)));
    }
    else
    {
      static_assert(false, "Could not determine argument type");
    }
  };

  // Process each argument
  (add_arg(std::forward<Args>(args)), ...);

  return *this;
}

/**
 * @brief Includes environment variables with the format 'NAME=VALUE' in the environment
 *
 * Accepts any number of environment variable entries (including zero). Each entry can be:
 * - A single "KEY=VALUE" string (replaces existing KEY if present)
 * - An iterable container of "KEY=VALUE" strings (each element processed)
 *
 * Automatically removes duplicate keys - if the same KEY appears multiple times,
 * only the last value is kept.
 *
 * @tparam Args Types of arguments (variadic, can be string, iterable, or string representable)
 * @param args Environment variables to include (format: "KEY=VALUE")
 * @return Subprocess& A reference to *this
 *
 * @code
 * // Single variable
 * .with_env("PATH=/usr/bin")
 *
 * // Multiple variables
 * .with_env("HOME=/home/user", "LANG=en_US.UTF-8", "EDITOR=vim")
 *
 * // Container of variables
 * std::vector<std::string> env_vars = {"VAR1=value1", "VAR2=value2"};
 * .with_env(env_vars)
 *
 * // Mixed
 * .with_env("PATH=/usr/bin", env_vars, "DEBUG=1")
 * @endcode
 */
template<typename... Args>
Subprocess& Subprocess::with_env(Args&&... args)
{
  // Helper to erase existing entries by key
  auto f_erase_existing = [this](auto&& entries)
  {
    for (auto&& entry : entries)
    {
      auto parts = ns_vector::from_string(entry, '=');
      continue_if(parts.size() < 2, "E::Entry '{}' is not valid", entry);
      std::string key = parts.front();
      std::ignore = this->rm_var(key);
    }
  };

  // Helper lambda to add a single env entry or container
  auto add_env = [this, &f_erase_existing]<typename T>(T&& arg) -> void
  {
    if constexpr ( ns_concept::Uniform<std::remove_cvref_t<T>, std::string> )
    {
      f_erase_existing(std::vector<std::string>{arg});
      this->m_env.push_back(std::forward<T>(arg));
    }
    else if constexpr ( ns_concept::Iterable<T> )
    {
      f_erase_existing(arg);
      std::ranges::copy(arg, std::back_inserter(m_env));
    }
    else if constexpr ( ns_concept::StringRepresentable<T> )
    {
      auto entry = ns_string::to_string(std::forward<T>(arg));
      f_erase_existing(std::vector<std::string>{entry});
      this->m_env.push_back(entry);
    }
    else
    {
      static_assert(false, "Could not determine argument type");
    }
  };

  // Process each argument
  (add_env(std::forward<Args>(args)), ...);

  return *this;
}

/**
 * @brief Configures the child process to die when the specified PID dies
 *
 * Uses prctl(PR_SET_PDEATHSIG) to ensure the child process receives SIGKILL
 * when the parent process with the given PID terminates. This prevents orphaned
 * processes and ensures cleanup even if the parent crashes.
 *
 * @param pid The PID that the child should monitor (typically parent's PID)
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Child dies when current process dies
 * pid_t my_pid = getpid();
 * Subprocess daemon("/usr/bin/daemon");
 * daemon.with_die_on_pid(my_pid)
 *       .spawn();
 * // If this process exits/crashes, daemon is automatically killed
 * @endcode
 */
inline Subprocess& Subprocess::with_die_on_pid(pid_t pid)
{
  m_die_on_pid = pid;
  return *this;
}

/**
 * @brief Sets the stdio redirection mode for the child process
 *
 * Controls how the child's stdin, stdout, and stderr are handled:
 * - Stream::Inherit: Child inherits parent's stdio (default)
 * - Stream::Pipe: Redirect to pipes with stream handlers (use with_streams())
 * - Stream::Null: Redirect to /dev/null (silent execution)
 *
 * @param mode Stream redirection mode
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Silent execution (no output)
 * Subprocess quiet("/usr/bin/noisy_app");
 * quiet.with_stdio(Stream::Null)
 *      .spawn();
 *
 * // Capture output with pipes
 * std::ostringstream output;
 * Subprocess capture("/bin/ls");
 * capture.with_stdio(Stream::Pipe)
 *        .with_streams(std::cin, output, std::cerr)
 *        .with_args("-la")
 *        .spawn()
 *        .wait();
 *
 * // Inherit (show on parent's terminal)
 * Subprocess interactive("/usr/bin/vim");
 * interactive.with_stdio(Stream::Inherit)
 *            .spawn();
 * @endcode
 */
inline Subprocess& Subprocess::with_stdio(Stream mode)
{
  m_stream_mode = mode;
  return *this;
}

/**
 * @brief Configures logging output for child process stdout/stderr
 *
 * Automatically sets up pipe redirection (Stream::Pipe mode) and configures
 * the log sink for the pipe reader threads. All child process output
 * (stdout and stderr) is captured and logged to the specified file.
 *
 * The child's stdout/stderr are redirected to pipes, and those pipes are
 * read by detached threads that write directly to log files. This works
 * even if the child process calls execve(), as the pipe readers remain in
 * the parent.
 *
 * The log file path is used directly by ns_log::set_sink_file() in the
 * pipe reader threads.
 *
 * If stdout/stderr stream handlers are registered via with_streams(),
 * they will receive the child's output in addition to the log file.
 *
 * @param path Path for log file (e.g., "/tmp/app.log")
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Capture child's stdout/stderr to log file
 * Subprocess("/usr/bin/app")
 *     .with_log_file("/tmp/app.log")
 *     .spawn();
 * @endcode
 */
inline Subprocess& Subprocess::with_log_file(std::filesystem::path const& path)
{
  m_path_file_log = path;
  // Set up pipe mode - pipe readers will configure logger sink to the specified path
  return this->with_stdio(Stream::Pipe);
}

/**
 * @brief Sets the logging level for the pipe reader processes
 *
 * Configures the verbosity level for the pipe reader processes that capture
 * the child's stdout/stderr. The log level determines which messages are
 * written to the log files:
 * - Level::CRITICAL: Only critical messages (always shown)
 * - Level::ERROR: Critical and error messages
 * - Level::WARN: Critical, error, and warning messages
 * - Level::INFO: Critical, error, warning, and info messages
 * - Level::DEBUG: All messages including debug (most verbose)
 *
 * This is applied in the forked pipe reader processes (not the main child process),
 * so it affects logging from the subprocess library's pipe handling mechanism.
 *
 * @param level The logging level to set (ns_log::Level enum)
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Enable debug logging for pipe readers
 * Subprocess proc("/usr/bin/app");
 * proc.with_log_level(ns_log::Level::DEBUG)
 *     .with_log_file("/tmp/app.log")
 *     .spawn();
 *
 * // Minimal logging (only critical messages)
 * Subprocess quiet("/usr/bin/background_task");
 * quiet.with_log_level(ns_log::Level::CRITICAL)
 *      .with_stdio(Stream::Null)
 *      .spawn();
 *
 * // Standard info level logging
 * Subprocess normal("/usr/bin/service");
 * normal.with_log_level(ns_log::Level::INFO)
 *       .spawn();
 * @endcode
 */
inline Subprocess& Subprocess::with_log_level(ns_log::Level const& level)
{
  m_log_level = level;
  return *this;
}

/**
 * @brief Configures the child process to die when the specified parent PID dies
 *
 * Sets up a death signal (SIGKILL) via prctl(PR_SET_PDEATHSIG) so the child process
 * terminates if the parent dies. This prevents orphaned processes.
 *
 * @param pid The parent process ID to monitor (typically from getpid() in parent)
 */
inline void Subprocess::die_on_pid(pid_t pid)
{
  // Set death signal when pid dies
  return_if(prctl(PR_SET_PDEATHSIG, SIGKILL) < 0,,"E::Failed to set PR_SET_PDEATHSIG: {}", strerror(errno));
  // Abort if pid is not running
  if (::kill(pid, 0) < 0)
  {
    logger("E::Parent died, prctl will not have effect: {}", strerror(errno));
    _exit(1);
  } // if
  // Log pid and current pid
  logger("D::{} dies with {}", getpid(), pid);
}

/**
 * @brief Redirects stdout and stderr to /dev/null
 */
inline void Subprocess::to_dev_null()
{
  int fd = open("/dev/null", O_WRONLY);
  return_if(fd < 0,,"E::Failed to open /dev/null: {}", strerror(errno));
  return_if(dup2(fd, STDIN_FILENO) < 0,,"E::Failed to redirect stdin: {}", strerror(errno));
  return_if(dup2(fd, STDOUT_FILENO) < 0,,"E::Failed to redirect stdout: {}", strerror(errno));
  return_if(dup2(fd, STDERR_FILENO) < 0,,"E::Failed to redirect stderr: {}", strerror(errno));
  close(fd);
}

/**
 * @brief Sets up pipes for stdin/stdout/stderr redirection
 *
 * Delegates to ns_pipe::setup() to configure pipe redirection for the child process.
 * Creates detached threads for reading/writing pipes.
 *
 * @param child_pid The PID of the child process (0 for child, >0 for parent)
 * @param pipestdin Stdin pipe array [read_end, write_end] (must be created before fork)
 * @param pipestdout Stdout pipe array [read_end, write_end] (must be created before fork)
 * @param pipestderr Stderr pipe array [read_end, write_end] (must be created before fork)
 * @param log Log file path for the pipe reader threads
 */
inline void Subprocess::setup_pipes(pid_t child_pid
  , int pipestdin[2]
  , int pipestdout[2]
  , int pipestderr[2]
  , std::filesystem::path const& log)
{
  // Setup pipes for parent or child
  ns_pipe::setup(child_pid
    , pipestdin
    , pipestdout
    , pipestderr
    , m_stdin.get()
    , m_stdout.get()
    , m_stderr.get()
    , log
  );
}

/**
 * @brief Executes the child process via execve (never returns)
 */
[[noreturn]] inline void Subprocess::exec_child()
{
  // Create null-terminated arrays for execve
  auto argv_custom = to_carray(m_args);
  auto envp_custom = to_carray(m_env);

  // Perform execve
  execve(m_program.c_str(), const_cast<char**>(argv_custom.get()), const_cast<char**>(envp_custom.get()));

  // Log error (will go to log_file if redirected)
  logger("E::execve() failed: {}", strerror(errno));

  // Child should stop here
  _exit(1);
}

/**
 * @brief Configures stream handlers for stdin, stdout, and stderr of the child process
 *
 * Sets up the stream redirection for all three standard I/O streams at once.
 * The streams are used when Stream::Pipe mode is active via with_stdio(Stream::Pipe).
 *
 * - stdin_stream: Input stream to read from and write to child's stdin
 * - stdout_stream: Output stream to write child's stdout to
 * - stderr_stream: Output stream to write child's stderr to
 *
 * @param stdin_stream Input stream for the child process's stdin
 * @param stdout_stream Output stream for the child process's stdout
 * @param stderr_stream Output stream for the child process's stderr
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Redirect all streams to custom handlers
 * std::istringstream input("echo hello");
 * std::ostringstream output;
 * std::ostringstream errors;
 *
 * Subprocess proc("/bin/bash");
 * proc.with_stdio(Stream::Pipe)
 *     .with_streams(input, output, errors)
 *     .spawn()
 *     .wait();
 *
 * // Use with std::cout and std::cerr
 * Subprocess build("/usr/bin/make");
 * build.with_stdio(Stream::Pipe)
 *      .with_streams(std::cin, std::cout, std::cerr)
 *      .spawn();
 *
 * // Capture output to files via fstream
 * std::ifstream stdin_file("/dev/null");
 * std::ofstream stdout_file("/tmp/output.log");
 * std::ofstream stderr_file("/tmp/errors.log");
 * Subprocess cmd("/bin/app");
 * cmd.with_stdio(Stream::Pipe)
 *    .with_streams(stdin_file, stdout_file, stderr_file)
 *    .spawn();
 * @endcode
 */
inline Subprocess& Subprocess::with_streams(std::istream& stdin_stream, std::ostream& stdout_stream, std::ostream& stderr_stream)
{
  this->m_stdin = stdin_stream;
  this->m_stdout = stdout_stream;
  this->m_stderr = stderr_stream;
  return *this;
}

/**
 * @brief Sets a callback to run in the child process after fork() but before execve()
 *
 * The callback runs in the child process context, allowing you to for example:
 * - Manipulate file descriptors beyond stdin/stdout/stderr
 * - Change working directory
 *
 * IMPORTANT: The callback runs AFTER stdio redirection and die_on_pid setup,
 * but BEFORE execve(). Any errors should use _exit() not exit() to avoid
 * flushing parent's buffers.
 *
 * @tparam F Function type compatible with std::function<void(ArgsCallbackChild)>
 * @param f Callback function that receives ArgsCallbackChild
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Close extra file descriptors
 * Subprocess proc("/usr/bin/app");
 * proc.with_callback_child([](ArgsCallbackChild args) {
 *     // Close all FDs except 0, 1, 2
 *     for (int fd = 3; fd < 1024; fd++) {
 *         close(fd);
 *     }
 * }).spawn();
 *
 * // Change working directory
 * proc.with_callback_child([](ArgsCallbackChild args) {
 *     if (chdir("/tmp") < 0) {
 *         _exit(1);  // Use _exit, not exit
 *     }
 * }).spawn();
 *
 * // Duplicate FD for inheritance
 * int special_fd = open("/path/to/file", O_RDONLY);
 * proc.with_callback_child([special_fd](ArgsCallbackChild args) {
 *     dup2(special_fd, 10);  // Child will have file at FD 10
 *     close(special_fd);
 *     // Can also access args.parent_pid, args.stdin_fd, etc.
 * }).spawn();
 * @endcode
 */
template<typename F>
Subprocess& Subprocess::with_callback_child(F&& f)
{
  this->m_callback_child = std::forward<F>(f);
  return *this;
}

/**
 * @brief Sets a callback to run in the parent process after fork()
 *
 * The callback runs in the parent process context after the child has been forked,
 * allowing you to:
 * - Record the child PID for monitoring
 * - Close file descriptors that only the child needs
 * - Set up additional IPC mechanisms
 * - Perform cleanup or wait operations in the parent
 *
 * The callback receives ArgsCallbackParent containing the child's PID.
 *
 * IMPORTANT: The callback runs AFTER the child has been forked and BEFORE
 * the Child handle is returned. Any exceptions or errors will prevent the
 * Child handle from being created.
 *
 * @tparam F Function type compatible with std::function<void(ArgsCallbackParent)>
 * @param f Callback function that receives ArgsCallbackParent
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Record child PID
 * pid_t child_pid;
 * Subprocess proc("/usr/bin/app");
 * proc.with_callback_parent([&child_pid](ArgsCallbackParent args) {
 *     child_pid = args.child_pid;
 *     std::cout << "Spawned child: " << args.child_pid << "\n";
 * }).spawn();
 *
 * // Close inherited FD in parent (child keeps it)
 * int shared_fd = open("/path/to/file", O_RDONLY);
 * proc.with_callback_parent([shared_fd](ArgsCallbackParent args) {
 *     close(shared_fd);  // Parent doesn't need it anymore
 * }).spawn();
 *
 * // Set up process monitoring
 * proc.with_callback_parent([](ArgsCallbackParent args) {
 *     register_process_monitor(args.child_pid);
 * }).spawn();
 * @endcode
 */
template<typename F>
inline Subprocess& Subprocess::with_callback_parent(F&& f)
{
  m_callback_parent = std::forward<F>(f);
  return *this;
}

/**
 * @brief Enable daemon mode using double fork pattern
 *
 * When enabled, the spawn() method will perform a double fork:
 * 1. First fork creates intermediate child
 * 2. Intermediate child forks to create grandchild (daemon)
 * 3. Intermediate child exits immediately
 * 4. Parent waits for intermediate child
 * 5. Grandchild becomes orphaned, adopted by init
 * 6. Grandchild calls setsid() to become session leader
 * 7. Grandchild continues with execve
 *
 * This detaches the process from the terminal and parent process.
 *
 * @return Reference to this Subprocess for method chaining
 *
 * @code
 * Subprocess proc("/usr/bin/my-daemon");
 * auto child = proc.with_args("--config", "/etc/config")
 *                  .with_daemon()
 *                  .spawn();
 * // Process is now daemonized and detached
 * @endcode
 */
inline Subprocess& Subprocess::with_daemon()
{
  m_daemon_mode = true;
  return *this;
}

/**
 * @brief Spawns (forks) the child process and begins execution
 *
 * Creates a child process via fork() and executes the configured program.
 * When daemon mode is enabled via with_daemon(), performs a double fork pattern
 * to fully detach the process from the terminal and parent.
 *
 * The parent process returns a Child handle immediately while the
 * child runs asynchronously. Call wait() on the returned handle
 * to synchronize and retrieve the exit code.
 *
 * Use with_log_file() before spawn() to capture child's stdout/stderr to log files.
 *
 * @return std::unique_ptr<Child> Unique pointer to the spawned process with wait() methods
 *
 * @code
 * // Basic spawn and wait
 * auto result = Subprocess("/bin/ls")
 *     .with_args("-la")
 *     .spawn()
 *     ->wait();
 *
 * // Spawn with stdout/stderr capture to files
 * auto proc = Subprocess("/bin/app")
 *     .with_log_file("/tmp/child.log")
 *     .spawn();
 *
 * // Background process (destructor waits automatically)
 * {
 *     auto daemon = Subprocess("/usr/bin/daemon")
 *         .with_stdio(Stream::Null)
 *         .spawn();
 *     // daemon runs, destructor waits when going out of scope
 * }
 *
 * // Multiple subprocesses in parallel
 * auto worker1 = Subprocess("/bin/worker").with_args("task1").spawn();
 * auto worker2 = Subprocess("/bin/worker").with_args("task2").spawn();
 * // Both run in parallel
 * worker1->wait();
 * worker2->wait();
 *
 * // Spawn with output capture
 * std::ostringstream output;
 * auto result = Subprocess("/bin/ps")
 *     .with_stdio(Stream::Pipe)
 *     .with_streams(std::cin, output, std::cerr)
 *     .with_args("aux")
 *     .spawn()
 *     ->wait();
 * @endcode
 */
inline std::unique_ptr<Child> Subprocess::spawn()
{
  // Ignore on empty vec_argv
  return_if(m_args.empty(), Child::create(-1, m_program), "E::No arguments to spawn subprocess");
  logger("D::Spawn command: {}", m_args);

  // Create pipes BEFORE fork (if needed for Stream::Pipe)
  int pipestdin[2];
  int pipestdout[2];
  int pipestderr[2];

  if ( m_stream_mode == Stream::Pipe )
  {
    return_if(pipe(pipestdin), Child::create(-1, m_program), "E::{}", strerror(errno));
    return_if(pipe(pipestdout), Child::create(-1, m_program), "E::{}", strerror(errno));
    return_if(pipe(pipestderr), Child::create(-1, m_program), "E::{}", strerror(errno));
  }

  // Create child
  pid_t pid = fork();

  // Failed to fork
  return_if(pid < 0, Child::create(-1, m_program), "E::Failed to fork");

  // Parent returns here, child continues to execve
  if ( pid > 0 )
  {
    // If daemon mode, wait for intermediate child to exit
    if ( m_daemon_mode )
    {
      int status;
      log_if(waitpid(pid, &status, 0) < 0, "E::Waitpid failed: {}", strerror(errno));
      logger("D::Daemon mode: intermediate process exited");
      // Return a Child handle with -1 to indicate daemon (no process to track)
      return Child::create(-1, m_program);
    }
    // Setup pipes for parent
    else if ( m_stream_mode == Stream::Pipe)
    {
      this->setup_pipes(pid, pipestdin, pipestdout, pipestderr, m_path_file_log);
      logger("D::Parent pipes configured");
    }

    // Execute parent callback if provided
    if (m_callback_parent)
    {
      ArgsCallbackParent args{.child_pid = pid};
      m_callback_parent.value()(args);
    }

    // Return Child handle with process and pipe PIDs (stdin writer, stdout reader, stderr reader)
    return Child::create(pid, m_program);
  }

  // Child process continues here (intermediate child in daemon mode, final child otherwise)

  // Daemon mode: perform second fork to create grandchild
  if ( m_daemon_mode )
  {
    // Create session leader to detach from controlling terminal
    if ( setsid() < 0 )
    {
      logger("E::setsid() failed: {}", strerror(errno));
      _exit(1);
    }

    // Second fork - creates grandchild
    pid = fork();

    if ( pid < 0 )
    {
      logger("E::Second fork failed: {}", strerror(errno));
      _exit(1);
    }

    if ( pid > 0 )
    {
      // Intermediate child exits immediately
      // This causes grandchild to be orphaned and adopted by init
      _exit(0);
    }

    // Grandchild continues here - now a daemon
    // Change working directory to root to avoid keeping directories busy
    if ( chdir("/") < 0 )
    {
      logger("W::chdir() to / failed: {}", strerror(errno));
    }

    // Reset file mode creation mask
    umask(0);
  }
  // Setup pipes for child
  else if ( m_stream_mode == Stream::Pipe )
  {
    this->setup_pipes(pid, pipestdin, pipestdout, pipestderr, m_path_file_log);
    logger("D::child pipes configured", m_daemon_mode);
  }

  ns_log::set_level(m_log_level);

  // Handle stdio redirection based on mode
  // Daemon should not inherit the parent's IO
  if (m_stream_mode == Stream::Null or m_daemon_mode)
  {
    this->to_dev_null();
  }
  // Stream::Pipe: child's stdout/stderr redirected by pipes_child() via setup_pipes()
  // Stream::Inherit: do nothing, child keeps parent's stdio

  // Die with pid
  if(m_die_on_pid)
  {
    this->die_on_pid(m_die_on_pid.value());
  }

  // Execute child callback if provided
  if (m_callback_child)
  {
    ArgsCallbackChild args
    {
      .parent_pid = getppid(),
      .stdin_fd = STDIN_FILENO,
      .stdout_fd = STDOUT_FILENO,
      .stderr_fd = STDERR_FILENO
    };
    m_callback_child.value()(args);
  }

  // Execute child process (never returns)
  this->exec_child();
}

} // namespace ns_subprocess

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
