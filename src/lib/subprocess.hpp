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
    std::optional<std::function<void(std::string)>> m_fstdout;
    std::optional<std::function<void(std::string)>> m_fstderr;
    Stream m_stream_mode;
    std::optional<pid_t> m_die_on_pid;
    std::optional<std::filesystem::path> m_log_file;

    void die_on_pid(pid_t pid);
    void to_dev_null();
    std::pair<pid_t, pid_t> setup_pipes(pid_t child_pid, int pipestdout[2], int pipestderr[2]);
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

    template<ns_string::static_string S = "D">
    [[maybe_unused]] [[nodiscard]] Subprocess& with_log_stdio();

    [[maybe_unused]] [[nodiscard]] Subprocess& with_log_file(std::filesystem::path const& path);

    template<typename F>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_stdout_handle(F&& f);

    template<typename F>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_stderr_handle(F&& f);

    [[maybe_unused]] [[nodiscard]] std::unique_ptr<Child> spawn();

    [[maybe_unused]] [[nodiscard]] Value<int> wait();
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
  , m_fstdout(std::nullopt)
  , m_fstderr(std::nullopt)
  , m_stream_mode(Stream::Inherit)
  , m_die_on_pid(std::nullopt)
  , m_log_file(std::nullopt)
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
  rm_var(k);
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
    qreturn_if(vec.empty(), false);
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
    if constexpr ( ns_concept::SameAs<std::remove_cvref_t<T>, std::string> )
    {
      this->m_args.push_back(std::forward<T>(arg));
    }
    else if constexpr ( ns_concept::IterableConst<T> )
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
      econtinue_if(parts.size() < 2, std::format("Entry '{}' is not valid", entry));
      std::string key = parts.front();
      std::ignore = this->rm_var(key);
    }
  };

  // Helper lambda to add a single env entry or container
  auto add_env = [this, &f_erase_existing]<typename T>(T&& arg) -> void
  {
    if constexpr ( ns_concept::SameAs<std::remove_cvref_t<T>, std::string> )
    {
      f_erase_existing(std::vector<std::string>{arg});
      this->m_env.push_back(std::forward<T>(arg));
    }
    else if constexpr ( ns_concept::IterableConst<T> )
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
 * - Stream::Pipe: Redirect to pipes with callbacks (use with_stdout_handle/with_stderr_handle)
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
 * std::string output;
 * Subprocess capture("/bin/ls");
 * capture.with_stdio(Stream::Pipe)
 *        .with_stdout_handle([&](std::string line) {
 *            output += line;
 *        })
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
 * @brief Sets up pipe handlers for logging stdout/stderr to the logger
 *
 * This is a convenience method that configures Stream::Pipe mode and sets up
 * stdout/stderr handlers that log output with the format "D::{program}:{line}".
 *
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Log all output from a subprocess
 * Subprocess("/usr/bin/make")
 *     .with_args("all")
 *     .with_log_stdio()
 *     .spawn()
 *     .wait();
 * @endcode
 */
template<ns_string::static_string S>
inline Subprocess& Subprocess::with_log_stdio()
{
  // Build fmt
  static constexpr ns_string::static_string suffix{"::{}::{}"};
  constexpr auto fmt = S.template join<suffix>();
  // Get program name
  std::string program_name = m_program.filename().string();
  // Send to logger
  return this->with_stdio(Stream::Pipe)
      .with_stdout_handle([fmt,program_name](std::string const& s){ logger(fmt, program_name, s); })
      .with_stderr_handle([fmt,program_name](std::string const& s){ logger(fmt, program_name, s); });
}

/**
 * @brief Sets the log file path for the child process logger
 *
 * Configures where the child process's ns_log output will be written.
 * This is set up via set_sink_file() in the child process.
 *
 * @param path Path to the log file (e.g., "/dev/null", "/tmp/child.log")
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Discard child's internal logging
 * Subprocess("/usr/bin/app")
 *     .with_log_file("/dev/null")
 *     .spawn();
 *
 * // Save child's logging to file
 * Subprocess("/usr/bin/app")
 *     .with_log_file("/tmp/child_debug.log")
 *     .spawn();
 * @endcode
 */
inline Subprocess& Subprocess::with_log_file(std::filesystem::path const& path)
{
  m_log_file = path;
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
  ereturn_if(prctl(PR_SET_PDEATHSIG, SIGKILL) < 0, strerror(errno));
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
  ereturn_if(fd < 0, std::format("Failed to open /dev/null: {}", strerror(errno)));
  ereturn_if(dup2(fd, STDOUT_FILENO) < 0, std::format("Failed to redirect stdout: {}", strerror(errno)));
  ereturn_if(dup2(fd, STDERR_FILENO) < 0, std::format("Failed to redirect stderr: {}", strerror(errno)));
  close(fd);
}

/**
 * @brief Sets up pipes for stdout/stderr redirection
 *
 * @param child_pid The PID of the child process
 * @param pipestdout Stdout pipe (must be created before fork)
 * @param pipestderr Stderr pipe (must be created before fork)
 * @return std::pair<pid_t, pid_t> PIDs of stdout and stderr reader processes
 */
inline std::pair<pid_t, pid_t> Subprocess::setup_pipes(pid_t child_pid, int pipestdout[2], int pipestderr[2])
{
  // Setup pipes for parent or child
  auto [pid_stdout, pid_stderr] = ns_pipe::setup(child_pid
    , pipestdout
    , pipestderr
    , m_fstdout.value_or([](std::string const&){})
    , m_fstderr.value_or([](std::string const&){})
  );

  // Return the pipe reader PIDs for the parent to manage
  return {pid_stdout, pid_stderr};
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
 * @brief Sets a callback function to handle the child process's stdout
 *
 * The callback is invoked for each line of output from the child's stdout.
 * Requires Stream::Pipe mode via with_stdio(Stream::Pipe).
 * The callback runs in a separate forked process to avoid blocking the parent.
 *
 * @tparam F Function type compatible with std::function<void(std::string)>
 * @param f Callback function that receives each stdout line
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Collect all stdout lines
 * std::vector<std::string> lines;
 * Subprocess proc("/bin/ls");
 * proc.with_stdio(Stream::Pipe)
 *     .with_stdout_handle([&](std::string line) {
 *         lines.push_back(line);
 *     })
 *     .with_args("-la", "/usr")
 *     .spawn()
 *     .wait();
 *
 * // Log each line in real-time
 * Subprocess build("/usr/bin/make");
 * build.with_stdio(Stream::Pipe)
 *      .with_stdout_handle([](std::string line) {
 *          std::cout << "[BUILD] " << line << "\n";
 *      })
 *      .with_stderr_handle([](std::string line) {
 *          std::cerr << "[ERROR] " << line << "\n";
 *      })
 *      .spawn();
 *
 * // Parse structured output
 * Subprocess json_proc("/bin/get_json");
 * json_proc.with_stdio(Stream::Pipe)
 *          .with_stdout_handle([](std::string json_line) {
 *              auto obj = nlohmann::json::parse(json_line);
 *              // Process JSON object
 *          })
 *          .spawn();
 * @endcode
 */
template<typename F>
Subprocess& Subprocess::with_stdout_handle(F&& f)
{
  this->m_fstdout = f;
  return *this;
}

/**
 * @brief Sets a callback function to handle the child process's stderr
 *
 * The callback is invoked for each line of output from the child's stderr.
 * Requires Stream::Pipe mode via with_stdio(Stream::Pipe).
 * The callback runs in a separate forked process to avoid blocking the parent.
 *
 * @tparam F Function type compatible with std::function<void(std::string)>
 * @param f Callback function that receives each stderr line
 * @return Subprocess& A reference to *this for method chaining
 *
 * @code
 * // Capture errors separately from output
 * std::string errors;
 * Subprocess proc("/usr/bin/gcc");
 * proc.with_stdio(Stream::Pipe)
 *     .with_stderr_handle([&](std::string line) {
 *         errors += line + "\n";
 *     })
 *     .with_args("file.c", "-o", "file")
 *     .spawn()
 *     .wait();
 * if (!errors.empty()) {
 *     std::cerr << "Compilation errors:\n" << errors;
 * }
 *
 * // Real-time error monitoring
 * Subprocess server("/usr/bin/server");
 * server.with_stdio(Stream::Pipe)
 *       .with_stderr_handle([](std::string line) {
 *           if (line.find("FATAL") != std::string::npos) {
 *               alert_admin(line);
 *           }
 *       })
 *       .spawn();
 *
 * // Separate handling for stdout and stderr
 * Subprocess cmd("/bin/complex_app");
 * cmd.with_stdio(Stream::Pipe)
 *    .with_stdout_handle([](std::string line) {
 *        process_output(line);
 *    })
 *    .with_stderr_handle([](std::string line) {
 *        log_error(line);
 *    })
 *    .spawn();
 * @endcode
 */
template<typename F>
Subprocess& Subprocess::with_stderr_handle(F&& f)
{
  this->m_fstderr = f;
  return *this;
}

/**
 * @brief Spawns (forks) the child process and begins execution
 *
 * Creates a child process via fork() and executes the configured program.
 * The parent process returns a Child handle immediately while the
 * child runs asynchronously. Call wait() on the returned handle
 * to synchronize and retrieve the exit code.
 *
 * Use with_log_file() before spawn() to redirect child's log output (not stdout/stderr).
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
 * // Spawn with custom logger output
 * auto proc = Subprocess("/bin/app")
 *     .with_log_file("/tmp/child_debug.log")
 *     .spawn();
 * // Child's info() etc. goes to /tmp/child_debug.log
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
 * std::vector<std::string> output;
 * auto result = Subprocess("/bin/ps")
 *     .with_stdio(Stream::Pipe)
 *     .with_stdout_handle([&](std::string line) { output.push_back(line); })
 *     .with_args("aux")
 *     .spawn()
 *     ->wait();
 * @endcode
 */
inline std::unique_ptr<Child> Subprocess::spawn()
{
  // Ignore on empty vec_argv
  ereturn_if(m_args.empty(), "No arguments to spawn subprocess", Child::create(-1, {-1, -1}, m_program));
  logger("D::Spawn command: {}", m_args);

  // Create pipes BEFORE fork (if needed for Stream::Pipe)
  int pipestdout[2];
  int pipestderr[2];
  std::pair<pid_t, pid_t> stdio_pids = {-1, -1};

  if ( m_stream_mode == Stream::Pipe )
  {
    ereturn_if(pipe(pipestdout), strerror(errno), Child::create(-1, {-1, -1}, m_program));
    ereturn_if(pipe(pipestderr), strerror(errno), Child::create(-1, {-1, -1}, m_program));
  }

  // Create child
  pid_t child_pid = fork();

  // Failed to fork
  ereturn_if(child_pid < 0, "Failed to fork", Child::create(-1, {-1, -1}, m_program));

  // Setup pipes for parent or child (only if Stream::Pipe)
  if ( m_stream_mode == Stream::Pipe )
  {
    stdio_pids = this->setup_pipes(child_pid, pipestdout, pipestderr);
  }

  // Parent returns here, child continues to execve
  if ( child_pid > 0 )
  {
    // Return Child handle with process and pipe PIDs
    return Child::create(child_pid, stdio_pids, m_program);
  }

  // Child process continues here

  // Setup logger file if provided
  if ( m_log_file )
  {
    ns_log::set_sink_file(m_log_file.value());
  }

  // Handle stdio redirection based on mode
  if ( m_stream_mode == Stream::Null )
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

  // Execute child process (never returns)
  this->exec_child();
}

/**
 * @brief Spawns the process and waits for completion with concise error handling
 *
 * This is a convenience method that:
 * - Spawns the process
 * - Waits for the process to complete
 * - Returns the exit code directly with logging
 *
 * @return Value<int> The exit code on success, error message on failure
 *
 * @code
 * // Simple usage
 * int code = Try(Subprocess("/usr/bin/make")
 *     .with_args("all")
 *     .with_log_stdio()
 *     .wait());
 *
 * if (code == 0) {
 *     std::cout << "Build succeeded\n";
 * }
 *
 * // With log file for child's internal logging
 * auto code = Subprocess("/usr/bin/app")
 *     .with_log_file("/dev/null")
 *     .with_log_stdio()
 *     .wait();
 * @endcode
 */
inline Value<int> Subprocess::wait()
{
  // Get code
  int code = Pop(spawn()->wait(), "E::{} exited abnormally", m_program);
  // Debug
  logger("D::{} exited with code {}", m_program, code);
  // Inform about error code
  return code;
}

} // namespace ns_subprocess

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
