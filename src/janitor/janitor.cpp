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

#include "../lib/log.hpp"
#include "../lib/env.hpp"
#include "../lib/fuse.hpp"
#include "../macro.hpp"

std::atomic_bool G_CONTINUE(true);

void cleanup(int)
{
  G_CONTINUE = false;
}


/**
 * @brief Boots the main janitor program
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return Expected<void> Nothing on success or the respective error
 */
[[nodiscard]] Expected<void> boot(int argc, char** argv)
{
  // Register signal handler
  signal(SIGTERM, cleanup);
  // Initialize logger
  ns_log::set_sink_file(Expect(ns_env::get_expected("FIM_DIR_MOUNT")) + ".janitor.log");
  // Check argc
  qreturn_if(argc < 2, std::unexpected("Incorrect usage"));
  // Get pid to wait for
  pid_t pid_parent = std::stoi(Expect(ns_env::get_expected("PID_PARENT")));
  // Create a novel session for the child process
  pid_t pid_session = setsid();
  qreturn_if(pid_session < 0, std::unexpected("Failed to create a novel session for janitor"));
  ns_log::info()("Session id is '{}'", pid_session);
  // Wait for parent process to exit
  while ( kill(pid_parent, 0) == 0 and G_CONTINUE )
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  ns_log::info()("Parent process with pid '{}' finished", pid_parent);
  // Cleanup of mountpoints
  for (auto&& path_dir_mountpoint : std::vector<std::filesystem::path>(argv+1, argv+argc))
  {
    ns_log::info()("Un-mount '{}'", path_dir_mountpoint);
    ns_fuse::unmount(path_dir_mountpoint);
  }
  return {};
}

int main(int argc, char** argv)
{
  auto result = boot(argc, argv);
  if(not result)
  {
    ns_log::error()("Failure to start janitor: {}", result.error());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
} // main

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
