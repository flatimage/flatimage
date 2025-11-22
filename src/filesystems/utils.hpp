/**
 * @file utils.hpp
 * @brief Filesystem utilities for managing instances and layers.
 *
 * This file provides utility functions and data structures for interacting with
 * FlatImage filesystem components.
 */

#pragma once

#include <string>
#include <filesystem>
#include <ranges>

#include "../std/expected.hpp"
#include "../macro.hpp"

/**
 * @namespace ns_filesystems::ns_utils
 * @brief Filesystem utility functions
 *
 * Provides utility functions for filesystem management including busy state detection by
 * scanning /proc/[pid]/mountinfo, layer enumeration, and instance cleanup operations.
 * Supports filesystem state queries and validation for safe mount/unmount operations.
 */
namespace ns_filesystems::ns_utils
{

namespace
{

namespace fs = std::filesystem;

}

/** @brief Checks if a filesystem path is currently in use by any active process.
 *
 * Scans all processes in /proc/[pid]/mountinfo to determine if the given path
 * is mounted or actively used. Iterates through each process's mount
 * information and returns true if the path is found in any mountinfo entry,
 * indicating the path is busy/in-use.
 *
 * @param path_dir The filesystem path to check for busy status.
 * @return true if the path is found in any process's mount information
 * (indicating busy state), false if no processes have mounted or are using the
 * path.
 */
inline bool is_busy(fs::path const& path_dir)
{
  [[maybe_unused]] auto __expected_fn = [](auto&&){ return std::nullopt; };
  // Check all processes in /proc
  std::vector<std::string> const pids = std::filesystem::directory_iterator("/proc")
    // Get directories only
    | std::views::filter([](auto&& e){ return fs::is_directory(e); })
    // Get string file names
    | std::views::transform([](auto&& e){ return e.path().filename().string(); })
    // Drop empty strings
    | std::views::filter([](auto&& e){ return not e.empty(); })
    // Filter & transform to pid
    | std::views::filter([](auto&& e){ return Catch(std::stoi(e)).has_value(); })
    | std::ranges::to<std::vector<std::string>>();

  for (std::string const& pid : pids)
  {
    // Open mountinfo file
    std::ifstream file_info("/proc/" + pid + "/mountinfo");
    continue_if(not file_info.is_open());
    // Iterate through file lines
    auto lines = std::ranges::istream_view<std::string>(file_info);
    auto it = std::ranges::find_if(lines, [&](auto const& line)
    {
      return line.find(path_dir) != std::string::npos;
    });
    return_if(it != lines.end(), true, "D::Busy '{}' due to '{}'", path_dir, *it);
  }
  return false;
}

/**
 * @brief Represents an instance
 */
struct Instance
{
  pid_t pid;  ///< PID of the running instance
  fs::path path; ///< Path to FIM_DIR_INSTANCE
};

/**
 * @brief Get the running FlatImage instances
 *
 * Enumerates all active FlatImage instances by scanning the instance directory.
 * Each subdirectory in path_dir_instances is expected to be named after a process
 * ID (PID).
 *
 * @param path_dir_instances Path to FIM_DIR_INSTANCE of a FlatImage application
 * @return std::vector<Instance> A vector of Instance{pid,path} structures for all
 *         active instances, sorted by PID
 */
[[nodiscard]] inline std::vector<Instance> get_instances(fs::path const& path_dir_instances)
{
  // List instances
  auto f_pid = [&](auto&& e) { return Catch(std::stoi(e.path().filename().string())).value_or(0); };
  // Get instances
  auto instances = fs::directory_iterator(path_dir_instances)
    | std::views::transform([&](auto&& e){ return Instance(f_pid(e), e.path()); })
    | std::views::filter([&](auto&& e){ return e.pid > 0; })
    | std::views::filter([&](auto&& e){ return fs::exists(fs::path{"/proc"} / std::to_string(e.pid)); })
    | std::views::filter([&](auto&& e){ return e.pid != getpid(); })
    | std::ranges::to<std::vector<Instance>>();
  // Sort by pid (filename is a directory named as the pid of that instance)
  std::ranges::sort(instances, {}, [](auto&& e){ return e.pid; });
  return instances;
}

/**
 * @brief Get a path for each layer directory
 *
 * Each entry in path_dir_layers is expected to be a directory representing a
 * filesystem layer.  The function filters and collects only valid directories,
 * excluding any files or invalid entries.
 *
 * @param path_dir_layers Path to the layer directory
 * @return std::vector<fs::path> A vector of filesystem paths for all mounted
 * layers, sorted in ascending order
 */
[[nodiscard]] inline std::vector<fs::path> get_mounted_layers(fs::path const& path_dir_layers)
{
  std::vector<fs::path> vec_path_dir_layer = fs::directory_iterator(path_dir_layers)
    | std::views::filter([](auto&& e){ return fs::is_directory(e.path()); })
    | std::views::transform([](auto&& e){ return e.path(); })
    | std::ranges::to<std::vector<fs::path>>();
  std::ranges::sort(vec_path_dir_layer);
  return vec_path_dir_layer;
}



} // namespace ns_filesystems::ns_utils