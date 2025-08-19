/**
 * @file child.hpp
 * @author Ruan Formigoni
 * @brief Spawns a child process and connects its I/O to pipes
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <chrono>
#include <fcntl.h>
#include <string>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../lib/linux.hpp"
#include "../lib/env.hpp"
#include "../db/db.hpp"
#include "config.hpp"

namespace fs = std::filesystem;

namespace child
{

/**
 * @brief Write a PID to a fifo
 * 
 * @param pid The pid to write
 * @param db The database that writes the value of the "pid" key
 * @return Expected<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Expected<void> write_fifo_pid(pid_t const pid, ns_db::Db& db)
{
  // Get pid fifo
  auto path_fifo_pid = Expect(db("pid").template value<std::string>());
  // Write to fifo
  ssize_t bytes_written = ns_linux::open_write_with_timeout(path_fifo_pid
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<pid_t const>(&pid, 1)
  );
  // Check success with number of written bytes
  elog_if(bytes_written != sizeof(pid), std::format("Failed to write pid: {}", strerror(errno)));
  return {};
}

/**
 * @brief Writes an exit code to a fifo
 * 
 * @param code The exit code to write
 * @param db The database that writes the value of the "exit" key
 * @return Expected<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Expected<void> write_fifo_exit(int const code, ns_db::Db& db)
{
  // Get exit code fifo
  auto path_file_fifo = Expect(db("exit").template value<std::string>());
  // Write to fifo
  ssize_t bytes_written = ns_linux::open_write_with_timeout(path_file_fifo
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<int const>(&code, 1)
  );
  // Check success with number of written bytes
  elog_if(bytes_written != sizeof(code), std::format("Failed to write code: {}", strerror(errno)));
  return {};
}

/**
 * @brief Waits for a child pid to exit
 * 
 * @param pid The pid to wait for
 * @param db The database to write the pid into
 */
inline void parent_wait(pid_t const pid, ns_db::Db& db)
{
  // Write pid to fifo
  elog_expected("Failed to write pid to fifo: {}", write_fifo_pid(pid, db));
  // Wait for child to finish
  int status = -1;
  elog_if(::waitpid(pid, &status, 0) < 0
    , std::format("Could not wait for child: {}", strerror(errno))
  );
  // Get exit code
  int code = (not WIFEXITED(status))? 1 : WEXITSTATUS(status);
  ns_log::debug()("Exit code: {}", code);
  // Send exit code of child through a fifo
  elog_expected("Failed to write exit code to fifo: {}", write_fifo_exit(code, db));
}

/**
 * @brief Reads a file with environment variables
 * 
 * @param db The database that writes the value of the file pointed by the "environment" key
 * @return Expected<std::vector<std::string>> The read variables or the respective error
 */
[[nodiscard]] inline Expected<std::vector<std::string>> read_file_environment(ns_db::Db& db)
{
  // Fetch environment file path from db
  auto path_file_environment = Expect(db("environment").template value<std::string>());
  // Open environment file
  std::ifstream file_environment(path_file_environment);
  qreturn_if(not file_environment.is_open(), Unexpected("Could not open environment fifo file"));
  // Collect environment variables
  std::vector<std::string> out;
  for (std::string entry; std::getline(file_environment, entry);)
  {
    out.push_back(entry);
  }
  return out;
}

/**
 * @brief Forks a child
 * 
 * @param vec_argv Arguments to the child process
 * @param db Database with the process details, e.g. (stdin/stdout/stderr) pipes
 * @return Expected<void> 
 */
[[nodiscard]] inline Expected<void> child_execve(std::vector<std::string> const& vec_argv, ns_db::Db& db)
{
  // Create arguments for execve
  auto argv_custom = std::make_unique<const char*[]>(vec_argv.size()+1);
  argv_custom[vec_argv.size()] = nullptr;
  std::ranges::transform(vec_argv
     , argv_custom.get()
     , [](auto&& e){ return e.c_str(); }
  );
  // Create environment vector execve
  std::vector<std::string> vec_environment = Expect(read_file_environment(db));
  auto env_custom = std::make_unique<const char*[]>(vec_environment.size()+1);
  env_custom[vec_environment.size()] = nullptr;
  std::ranges::transform(vec_environment
     , env_custom.get()
     , [](auto&& e){ return e.c_str(); }
  );
  // Configure pipes
  auto f_open_fd = [](ns_db::Db& db, std::string const& name, int const fileno, int const oflag) -> Expected<void>
  {
    auto path_fifo_stdin = Expect(db(name).template value<fs::path>());
    int fd = ns_linux::open_with_timeout(path_fifo_stdin
      , std::chrono::seconds(SECONDS_TIMEOUT)
      , oflag
    );
    qreturn_if(fd < 0, Unexpected(std::format("open fd {} failed", name)));
    qreturn_if(dup2(fd, fileno) < 0, Unexpected(std::format("dup2 {} failed", name)));
    close(fd);
    return {};
  };
  Expect(f_open_fd(db, "stdin", STDIN_FILENO, O_RDONLY));
  Expect(f_open_fd(db, "stdout", STDOUT_FILENO, O_WRONLY));
  Expect(f_open_fd(db, "stderr", STDERR_FILENO, O_WRONLY));
  // Perform execve
  int ret_execve = execve(argv_custom[0], (char**) argv_custom.get(), (char**) env_custom.get());
  ns_log::error()("Could not perform execve({}): {}", ret_execve, strerror(errno));
  _exit(1);
}

/**
 * @brief Forks and execve a new child
 * 
 * @param path_dir_portal Path to the portal directory of the child process
 * @param msg Database received as a message to be parsed
 * @return Expected<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> spawn(fs::path const& path_dir_portal, std::string_view msg)
{
  if(auto ret = ns_log::set_sink_file(path_dir_portal / "spawned_parent_{}.log"_fmt(getpid())); not ret)
  {
    std::cerr << "Failed to set logger sink file: " << ret.error() << '\n';
  }
  ns_log::set_level(ns_log::Level::CRITICAL);
  auto db = Expect(ns_db::from_string(msg));
  // Get command
  auto vec_argv = Expect(db("command").template value<std::vector<std::string>>());
  // Search for command in PATH and replace vec_argv[0] with the full path to the binary
  vec_argv[0] = Expect(ns_env::search_path(vec_argv[0]));
  // Ignore on empty command
  if ( vec_argv.empty() ) { return Unexpected("Empty command"); }
  // Create child
  pid_t ppid = getpid();
  pid_t pid = fork();
  // Failed to fork
  if(pid < 0)
  {
    return Unexpected("Failed to fork");
  }
  // Is parent
  else if (pid > 0)
  {
    parent_wait(pid, db);
    _exit(0);
  }
  // Is child
  // Configure child logger
  if(auto ret = ns_log::set_sink_file(path_dir_portal / "spawned_child_{}.log"_fmt(getpid())); not ret)
  {
    std::cerr << "Failed to set logger sink file: " << ret.error() << '\n';
  }
  ns_log::set_level(ns_log::Level::CRITICAL);
  // Die with parent
  qreturn_if(::prctl(PR_SET_PDEATHSIG, SIGKILL) < 0, Unexpected("Could not set child to die with parent"));
  // Check if parent still exists
  qreturn_if(::kill(ppid, 0) < 0, Unexpected("Parent pid is already dead"));
  // Perform execve
  Expect(child_execve(vec_argv, db));
  _exit(1);
}

} // namespace child