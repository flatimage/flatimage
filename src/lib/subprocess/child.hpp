/**
 * @file child.hpp
 * @author Ruan Formigoni
 * @brief Handle for a spawned child process
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <csignal>
#include <string>
#include <optional>
#include <utility>
#include <memory>

#include "../../macro.hpp"
#include "../../std/expected.hpp"

namespace ns_subprocess
{

// Forward declaration
class Subprocess;

/**
 * @brief Handle to a spawned child process
 *
 * This class represents a running child process and provides methods to wait for it
 * and retrieve its exit code. Returned by Subprocess::spawn().
 *
 * The Child class uses RAII to ensure proper cleanup - the destructor automatically
 * waits for the child process if it hasn't been waited on explicitly. This prevents
 * zombie processes.
 *
 * @note This class is non-copyable and non-moveable. It automatically waits for
 *       the process in its destructor.
 *
 * @code
 * // Basic usage - direct chaining
 * auto result = Subprocess("/bin/ls")
 *     .with_args("-la")
 *     .spawn()
 *     ->wait();
 *
 * @endcode
 */
class Child
{
  private:
    pid_t m_pid;
    std::string m_description;

    friend class Subprocess;

    /**
     * @brief Private constructor - only Subprocess can create Child instances
     *
     * @param pid The process ID of the child
     * @param description Description of the process (typically the program name)
     */
    Child(pid_t pid, std::string description)
      : m_pid(pid)
      , m_description(std::move(description))
    {}

    /**
     * @brief Factory method for creating Child instances
     *
     * @param pid The process ID of the child
     * @param description Description of the process (typically the program name)
     * @return std::unique_ptr<Child> Unique pointer to the created Child instance
     */
    static std::unique_ptr<Child> create(pid_t pid, std::string const& description)
    {
      return std::unique_ptr<Child>(new Child(pid, description));
    }

  public:
    /**
     * @brief Destructor automatically waits for the child process
     *
     * Ensures child processes are properly cleaned up even if wait() is not called.
     * This prevents zombie processes and resource leaks.
     *
     * @code
     * {
     *     auto child = Subprocess("/bin/sleep").with_args("5").spawn();
     *     // Destructor automatically waits for sleep to finish
     * } // wait() called here
     * @endcode
     */
    ~Child()
    {
      if (m_pid > 0)
      {
        std::ignore = wait();
      }
    }

    // Delete copy and move operations - Child is non-copyable and non-moveable
    Child(Child const&) = delete;
    Child& operator=(Child const&) = delete;
    Child(Child&&) = delete;
    Child& operator=(Child&&) = delete;

    /**
     * @brief Waits for the child process to finish and returns exit code
     *
     * Blocks until the child process terminates, then returns its exit code.
     * Also cleans up any pipe reader processes created by Stream::Pipe mode.
     * After wait() returns, the PID is invalidated (get_pid() returns nullopt).
     *
     * @return Value<int> The exit code (typically 0-255) on success, or error message on failure.
     *                    Exit code range depends on the child process (WEXITSTATUS returns low 8 bits).
     *
     * @code
     * // Check exit code
     * auto child = Subprocess("/bin/false").spawn();
     * auto result = child->wait();
     * if (result) {
     *     std::cout << "Exit code: " << *result << "\n";  // Exit code: 1
     * } else {
     *     std::cerr << "Error: " << result.error() << "\n";
     * }
     *
     * // Success/failure handling
     * auto child = Subprocess("/usr/bin/make").with_args("all").spawn();
     * if (auto code = child->wait(); code && *code == 0) {
     *     std::cout << "Build succeeded\n";
     * } else {
     *     std::cerr << "Build failed\n";
     * }
     *
     * // Multiple waits (safe but second returns error)
     * auto child = Subprocess("/bin/true").spawn();
     * auto result1 = child->wait();  // Returns exit code
     * auto result2 = child->wait();  // Returns error (already waited)
     * @endcode
     */
    [[nodiscard]] Value<int> wait()
    {
      // Check if pid is valid
      return_if(m_pid <= 0, Error("E::Invalid pid to wait for in {}", m_description));

      // Wait for current process
      int status;
      pid_t result = waitpid(m_pid, &status, 0);
      return_if(result < 0, Error("E::waitpid failed on {}: {}", m_description, strerror(errno)));

      // Mark pid as invalid to prevent double-wait
      m_pid = -1;

      return WIFEXITED(status)? Value<int>(WEXITSTATUS(status))
        : WIFSIGNALED(status)? Error("E::The process {} was terminated by a signal", m_description)
        : WIFSTOPPED(status)? Error("E::The process {} was stopped by a signal", m_description)
        : Error("E::The process {} exited abnormally", m_description);
    }

    /**
     * @brief Gets the PID of the child process
     *
     * Returns the process ID if the child is still valid (hasn't been waited on),
     * or std::nullopt if wait() has already been called.
     *
     * @return std::optional<pid_t> The PID if still running, nullopt otherwise
     *
     * @code
     * auto child = Subprocess("/bin/sleep").with_args("1").spawn();
     *
     * if (auto pid = child->get_pid()) {
     *     std::cout << "Child PID: " << *pid << "\n";
     * }
     *
     * child->wait();
     *
     * if (auto pid = child->get_pid()) {
     *     // Won't execute - PID is invalid after wait
     * } else {
     *     std::cout << "Process has finished\n";
     * }
     * @endcode
     */
    [[nodiscard]] std::optional<pid_t> get_pid() const
    {
      return (m_pid > 0) ? std::optional<pid_t>(m_pid) : std::nullopt;
    }

    /**
     * @brief Sends a signal to the child process
     *
     * Sends the specified signal to the child process if it's still valid.
     * Does nothing if the process has already been waited on.
     *
     * @param signal The signal number to send (e.g., SIGTERM, SIGKILL)
     *
     * @code
     * auto child = Subprocess("/bin/sleep").with_args("100").spawn();
     *
     * // Let it run for a bit
     * std::this_thread::sleep_for(std::chrono::milliseconds(100));
     *
     * // Terminate it early
     * child->kill(SIGTERM);
     *
     * // Wait for cleanup
     * child->wait();
     * @endcode
     */
    void kill(int signal)
    {
      if (m_pid > 0)
      {
        ::kill(m_pid, signal);
      }
    }
};

} // namespace ns_subprocess
