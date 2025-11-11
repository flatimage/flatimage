/**
 * @file janitor.cpp
 * @author Ruan Formigoni
 * @brief Cleans mountpoint after the PID passed as an argument exits,
 * it works as a fallback in case the main process fails to cleanup
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */


#include <chrono>
#include <cstdlib>
#include <thread>
#include <filesystem>
#include <csignal>

#include "../std/expected.hpp"
#include "../lib/log.hpp"
#include "../lib/fuse.hpp"
#include "../macro.hpp"

std::atomic_bool G_PARENT_OK(false);

void cleanup(int)
{
  G_PARENT_OK = true;
}


/**
 * @brief Boots the main janitor program
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] Value<void> boot(int argc, char** argv)
{
  // Register signal handlers
  signal(SIGTERM, cleanup);
  // Ignore SIGPIPE - when parent dies and pipe readers close, we can still cleanup
  signal(SIGPIPE, SIG_IGN);
  // Check argc
  return_if(argc < 3, Error("E::Incorrect usage: fim_janitor <parent_pid> <log_path>"));
  // Get pid to wait for
  pid_t pid_parent = Try(std::stoi(argv[1]));
  // Get log path from parent
  std::string path_log_file = argv[2];
  // Create a novel session for the child process
  pid_t pid_session = setsid();
  return_if(pid_session < 0, Error("E::Failed to create a novel session for janitor"));
  // Enable logger to write INFO messages to stdout (for pipe capture)
  ns_log::set_level(ns_log::Level::DEBUG);
  ns_log::set_as_fork();
  // Configure logger sink to write directly to file (fallback if pipe readers die)
  ns_log::set_sink_file(path_log_file);
  logger("I::Session id is '{}'", pid_session);
  // Wait for parent process to exit
  while (not G_PARENT_OK and kill(pid_parent, 0) == 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  // Check if should skip clean
  if(G_PARENT_OK)
  {
    logger("I::Parent process with pid '{}' finished", pid_parent);
    return {};
  }
  // Log that parent exited abnormally
  logger("E::Parent process with pid '{}' failed to send skip signal", pid_parent);
  // Cleanup of mountpoints
  for (auto&& path_dir_mountpoint : std::vector<std::filesystem::path>(argv+2, argv+argc))
  {
    logger("I::Un-mount '{}'", path_dir_mountpoint);
    ns_fuse::unmount(path_dir_mountpoint).discard("E::Could not un-mount '{}'", path_dir_mountpoint);
  }
  return {};
}

int main(int argc, char** argv)
{
  auto __expected_fn = [](auto&&){ return 1; };
  Pop(boot(argc, argv), "C::Failure to start janitor");
  return EXIT_SUCCESS;
} // main

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
