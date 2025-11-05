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
#include <ranges>

#include "log.hpp"
#include "../macro.hpp"
#include "../std/vector.hpp"
#include "../std/expected.hpp"

namespace ns_subprocess
{

class Subprocess
{
  private:
    std::string m_program;
    std::vector<std::string> m_args;
    std::vector<std::string> m_env;
    pid_t m_pid;
    std::vector<pid_t> m_vec_pids_pipe;
    std::optional<std::function<void(std::string)>> m_fstdout;
    std::optional<std::function<void(std::string)>> m_fstderr;
    bool m_with_piped_outputs;
    std::optional<pid_t> m_die_on_pid;

    [[nodiscard]] Subprocess& with_pipes_parent(int pipestdout[2], int pipestderr[2]);
    void with_pipes_child(int pipestdout[2], int pipestderr[2]);
    void die_on_pid(pid_t pid);
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

    [[maybe_unused]] [[nodiscard]] std::optional<pid_t> get_pid();

    void kill(int signal);

    template<typename Arg, typename... Args>
    requires (sizeof...(Args) > 0)
    [[maybe_unused]] [[nodiscard]] Subprocess& with_args(Arg&& arg, Args&&... args);

    template<typename T>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_args(T&& t);

    template<typename Arg, typename... Args>
    requires (sizeof...(Args) > 0)
    [[maybe_unused]] [[nodiscard]] Subprocess& with_env(Arg&& arg, Args&&... args);

    template<typename T>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_env(T&& t);

    [[maybe_unused]] [[nodiscard]] Subprocess& with_die_on_pid(pid_t pid);

    [[maybe_unused]] [[nodiscard]] Subprocess& with_piped_outputs();

    template<typename F>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_stdout_handle(F&& f);

    template<typename F>
    [[maybe_unused]] [[nodiscard]] Subprocess& with_stderr_handle(F&& f);

    [[maybe_unused]] [[nodiscard]] Subprocess& spawn();

    [[maybe_unused]] [[nodiscard]] Value<int> wait();
};

/**
 * @brief Construct a new Subprocess:: Subprocess object
 *
 * @tparam T Type that is string representable
 * @param t A string representable object
 */
template<ns_concept::StringRepresentable T>
Subprocess::Subprocess(T&& t)
  : m_program(ns_string::to_string(t))
  , m_args()
  , m_env()
  , m_pid(-1)
  , m_vec_pids_pipe()
  , m_fstdout(std::nullopt)
  , m_fstderr(std::nullopt)
  , m_with_piped_outputs(false)
  , m_die_on_pid(std::nullopt)
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
 * @brief Destroy the Subprocess:: Subprocess object
 */
inline Subprocess::~Subprocess()
{
  std::ignore = this->wait();
}

/**
 * @brief Clears the environment before starting the process
 * 
 * @return Subprocess& A reference to *this
 */
inline Subprocess& Subprocess::env_clear()
{
  m_env.clear();
  return *this;
}

/**
 * @brief Adds an environment variable in the process
 *
 * @tparam K Type that is string representable for the key
 * @tparam V Type that is string representable for the value
 * @param k The name of the environment variable
 * @param v The value of the environment variable
 * @return Subprocess& A reference to *this
 */
template<ns_concept::StringRepresentable K, ns_concept::StringRepresentable V>
Subprocess& Subprocess::with_var(K&& k, V&& v)
{
  rm_var(k);
  m_env.push_back("{}={}"_fmt(k,v));
  return *this;
}

/**
 * @brief Removes an environment variable from the process environment
 *
 * @tparam K Type that is string representable
 * @param k The name of the variable to remove
 * @return Subprocess& A reference to *this
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
    ns_log::debug()("Erased var entry: {}", *it);
    m_env.erase(it);
  } // if

  return *this;
}

/**
 * @brief Gets the PID of the current running process
 * 
 * @return std::optional<pid_t> The pid if the process is running
 */
inline std::optional<pid_t> Subprocess::get_pid()
{
  return this->m_pid;
}

/**
 * @brief Sends the signal to the process
 * 
 * @param signal The signal to send
 */
inline void Subprocess::kill(int signal)
{
  if ( auto opt_pid = this->get_pid(); opt_pid )
  {
    ::kill(*opt_pid, signal);
  } // if
}

/**
 * @brief Arguments forwarded as the process' arguments
 *
 * @tparam Arg Type of the first argument
 * @tparam Args Types of additional arguments (variadic)
 * @param arg The argument to forward
 * @param args More arguments to forward (optional)
 * @return Subprocess& A reference to *this
 */
template<typename Arg, typename... Args>
requires (sizeof...(Args) > 0)
Subprocess& Subprocess::with_args(Arg&& arg, Args&&... args)
{
  return with_args(std::forward<Arg>(arg)).with_args(std::forward<Args>(args)...);
}

/**
 * @brief Arguments forwarded as the process' arguments
 *
 * @tparam T Type of the argument (can be string, iterable, or string representable)
 * @param arg The argument to forward
 * @return Subprocess& A reference to *this
 */
template<typename T>
Subprocess& Subprocess::with_args(T&& arg)
{
  if constexpr ( ns_concept::SameAs<T, std::string> )
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

  return *this;
}

/**
 * @brief Includes environment variables with the format 'NAME=VALUE' in the environment
 *
 * @tparam Arg Type of the first argument
 * @tparam Args Types of additional arguments (variadic)
 * @param arg Variable to include
 * @param args Additional variables to include (optional)
 * @return Subprocess& A reference to *this
 */
template<typename Arg, typename... Args>
requires (sizeof...(Args) > 0)
Subprocess& Subprocess::with_env(Arg&& arg, Args&&... args)
{
  return with_env(std::forward<Arg>(arg)).with_env(std::forward<Args>(args)...);
}

/**
 * @brief Includes an environment variable with the format 'NAME=VALUE' in the environment
 *
 * @tparam T Type of the argument (can be string, iterable, or string representable)
 * @param arg Variable to include
 * @return Subprocess& A reference to *this
 */
template<typename T>
Subprocess& Subprocess::with_env(T&& arg)
{
  auto f_erase_existing = [this](auto&& entries)
  {
    for (auto&& entry : entries)
    {
      auto parts = ns_vector::from_string(entry, '=');
      econtinue_if(parts.size() < 2, "Entry '{}' is not valid"_fmt(entry));
      std::string key = parts.front();
      std::ignore = this->rm_var(key);
    } // for
  };

  if constexpr ( ns_concept::SameAs<T, std::string> )
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

  return *this;
}

/**
 * @brief Sets the process to die when 'pid' dies
 * 
 * @param pid The pid which causes the child to die
 * @return Subprocess& A reference to *this
 */
inline Subprocess& Subprocess::with_die_on_pid(pid_t pid)
{
  m_die_on_pid = pid;
  return *this;
}

/**
 * @brief Pipe the outputs of the spawned process
 * 
 * @return Subprocess& A reference to *this
 */
inline Subprocess& Subprocess::with_piped_outputs()
{
  m_with_piped_outputs = true;
  return *this;
}

/**
 * @brief Setup pipes for the parent process
 * 
 * @param pipestdout Stdout pipe
 * @param pipestderr Stderr pipe
 * @return Subprocess& A reference to *this
 */
inline Subprocess& Subprocess::with_pipes_parent(int pipestdout[2], int pipestderr[2])
{
  // Close write end
  ereturn_if(close(pipestdout[1]) == -1, "pipestdout[1]: {}"_fmt(strerror(errno)), *this);
  ereturn_if(close(pipestderr[1]) == -1, "pipestderr[1]: {}"_fmt(strerror(errno)), *this);

  auto f_read_pipe = [this](int id_pipe, std::string_view prefix, auto&& f)
  {
    // Fork
    pid_t ppid = getpid();
    pid_t pid = fork();
    ereturn_if(pid < 0, "Could not fork '{}'"_fmt(strerror(errno)));
    // Parent ends here
    if (pid > 0 )
    {
      m_vec_pids_pipe.push_back(pid);
      return;
    } // if
    // Die with parent
    e_exitif(prctl(PR_SET_PDEATHSIG, SIGKILL) < 0, strerror(errno), 1);
    e_exitif(::kill(ppid, 0) < 0, "Parent died, prctl will not have effect: {}"_fmt(strerror(errno)), 1);
    // Check if 'f' is defined
    if ( not f ) { f = [&](auto&& e) { ns_log::debug()("{}({}): {}", prefix, m_program, e); }; }
    // Apply f to incoming data from pipe
    char buffer[1024];
    ssize_t count;
    while ((count = read(id_pipe, buffer, sizeof(buffer))) != 0)
    {
      // Failed to read
      ebreak_if(count == -1, "broke parent read loop: {}"_fmt(strerror(errno)));
      // Split newlines and print each line with prefix
      std::ranges::for_each(std::string(buffer, count)
          | std::views::split('\n')
          | std::views::filter([&](auto&& e){ return not e.empty(); })
        , [&](auto&& e){ (*f)(std::string{e.begin(), e.end()});
      });
    } // while
    close(id_pipe);
    // Exit normally
    exit(0);
  };
  // Create pipes from fifo to ostream
  f_read_pipe(pipestdout[0], "stdout", this->m_fstdout);
  f_read_pipe(pipestderr[0], "stderr", this->m_fstderr);

  return *this;
}

/**
 * @brief Setup pipes for the child process
 * 
 * @param pipestdout Stdout pipe
 * @param pipestderr Stderr pipe
 */
inline void Subprocess::with_pipes_child(int pipestdout[2], int pipestderr[2])
{
  // Close read end
  ereturn_if(close(pipestdout[0]) == -1, "pipestdout[0]: {}"_fmt(strerror(errno)));
  ereturn_if(close(pipestderr[0]) == -1, "pipestderr[0]: {}"_fmt(strerror(errno)));

  // Make the opened pipe the replace stdout
  ereturn_if(dup2(pipestdout[1], STDOUT_FILENO) == -1, "dup2(pipestdout[1]): {}"_fmt(strerror(errno)));
  ereturn_if(dup2(pipestderr[1], STDERR_FILENO) == -1, "dup2(pipestderr[1]): {}"_fmt(strerror(errno)));

  // Close original write end after duplication
  ereturn_if(close(pipestdout[1]) == -1, "pipestdout[1]: {}"_fmt(strerror(errno)));
  ereturn_if(close(pipestderr[1]) == -1, "pipestderr[1]: {}"_fmt(strerror(errno)));
}

/**
 * @brief Sets the process to die when 'pid' dies
 * 
 * @param pid The pid which causes the child to die
 * @return Subprocess& A reference to *this
 */
inline void Subprocess::die_on_pid(pid_t pid)
{
  // Set death signal when pid dies
  ereturn_if(prctl(PR_SET_PDEATHSIG, SIGKILL) < 0, strerror(errno));
  // Abort if pid is not running
  if (::kill(pid, 0) < 0)
  {
    ns_log::error()("Parent died, prctl will not have effect: {}", strerror(errno));
    _exit(1);;
  } // if
  // Log pid and current pid
  ns_log::debug()("{} dies with {}", getpid(), pid);
}

/**
 * @brief Pipes the process STDOUT to f
 *
 * @tparam F Function type (std::function<void(std::string)>)
 * @param f A lambda std::function<void(std::string)>
 * @return Subprocess& A reference to *this
 */
template<typename F>
Subprocess& Subprocess::with_stdout_handle(F&& f)
{
  this->m_fstdout = f;
  return *this;
}

/**
 * @brief Pipes the process STDERR to f
 *
 * @tparam F Function type (std::function<void(std::string)>)
 * @param f A lambda std::function<void(std::string)>
 * @return Subprocess& A reference to *this
 */
template<typename F>
Subprocess& Subprocess::with_stderr_handle(F&& f)
{
  this->m_fstderr = f;
  return *this;
}

/**
 * @brief Waits for the spawned process to finish
 * 
 * @return Value<int> The process return code or the respective error
 */
inline Value<int> Subprocess::wait()
{
  // Check if pid is valid
  qreturn_if(m_pid <= 0, std::unexpected("Invalid pid to wait for"));

  // Wait for current process
  int status;
  waitpid(m_pid, &status, 0);

  // Send SIGTERM for reader forks
  std::ranges::for_each(m_vec_pids_pipe, [](pid_t pid){ ::kill(pid, SIGTERM); });

  // Wait for forks
  std::ranges::for_each(m_vec_pids_pipe, [](pid_t pid){ waitpid(pid, nullptr, 0); });

  return (WIFEXITED(status))?
      Value<int>(WEXITSTATUS(status))
    : std::unexpected("The process exited abnormally");
}

/**
 * @brief Spawns (forks) the child process
 * 
 * @return Subprocess& A reference to *this
 */
inline Subprocess& Subprocess::spawn()
{
  // Log
  ns_log::debug()("Spawn command: {}", m_args);

  int pipestdout[2];
  int pipestderr[2];

  // Create pipe
  ereturn_if(pipe(pipestdout), strerror(errno), *this);
  ereturn_if(pipe(pipestderr), strerror(errno), *this);

  // Ignore on empty vec_argv
  if ( m_args.empty() )
  {
    ns_log::error()("No arguments to spawn subprocess");
    return *this;
  } // if

  // Create child
  m_pid = fork();

  // Failed to fork
  ereturn_if(m_pid < 0, "Failed to fork", *this);

  // Setup pipe on child and parent
  // On parent, return exit code of child
  if ( m_pid > 0 )
  {
    return m_with_piped_outputs ? with_pipes_parent(pipestdout, pipestderr) : *this;
  } // if

  // On child, just setup the pipe
  if ( m_with_piped_outputs && m_pid == 0)
  {
    // this is non-blocking, setup pipes and perform execve afterwards
    with_pipes_child(pipestdout, pipestderr);
  } // else

  // Check if should die with pid
  if ( m_die_on_pid )
  {
    die_on_pid(*m_die_on_pid);
  } // if

  // Create arguments for execve
  auto argv_custom = std::make_unique<const char*[]>(m_args.size() + 1);

  // Copy arguments
  std::ranges::transform(m_args, argv_custom.get(), [](auto&& e) { return e.c_str(); });

  // Set last entry to nullptr
  argv_custom[m_args.size()] = nullptr;

  // Set environment variables
  for(auto&& entry : m_env)
  {
    auto pos_key = entry.find('=');
    econtinue_if(pos_key == std::string::npos or pos_key == 0
      , "Invalid environment variable '{}'"_fmt(entry)
    );
    std::string key = entry.substr(0, pos_key);
    std::string val = entry.substr(pos_key+1);
    elog_if(setenv(key.c_str(), val.c_str(), 1) < 0
      , "Could not set environment variable: {}"_fmt(strerror(errno))
    );
  }

  // Create environment for execve
  auto envp_custom = std::make_unique<const char*[]>(m_env.size() + 1);

  // Copy variables
  std::transform(m_env.begin(), m_env.end(), envp_custom.get(), [](auto&& e) { return e.c_str(); });

  // Set last entry to nullptr
  envp_custom[m_env.size()] = nullptr;

  // Perform execve
  execve(m_program.c_str(), const_cast<char**>(argv_custom.get()), const_cast<char**>(envp_custom.get()));

  // Log error
  ns_log::error()("execve() failed: ", strerror(errno));

  // Child should stop here
  _exit(1);
}

/**
 * @brief Spawns the process and waits for it to finish
 *
 * @tparam T Type of the process (string representable)
 * @tparam Args Types of additional arguments (variadic)
 * @param proc The process to start
 * @param args The arguments forwarded to the process
 * @return Value<int> The process return code or the respective error
 */
template<typename T, typename... Args>
[[nodiscard]] inline Value<int> wait(T&& proc, Args&&... args)
{
  return Subprocess(std::forward<T>(proc))
    .with_piped_outputs()
    .with_args(std::forward<Args>(args)...)
    .spawn()
    .wait();
}

/**
 * @brief Logs the subprocess execution with the current logger
 * 
 * @param ret The Subprocess::wait() return value
 * @param msg The message to display on errors
 */
[[maybe_unused]] inline void log(Value<int> const& ret, std::string_view msg)
{
  if ( not ret )
  {
    ns_log::error()("{} exited abnormally", msg);
  }
  else if ( ret.value() == 0 )
  {
    ns_log::debug()("{} exited with code 0", msg);
  }
  else
  {
    ns_log::error()("{} exited with non-zero exit code '{}'", msg, ret.value());
  }
}

} // namespace ns_subprocess

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
