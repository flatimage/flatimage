/**
 * @file test.hpp
 * @author Ruan Formigoni
 * @brief Test utilities for FlatImage unit tests
 *
 * Provides helper functions for creating and managing DwarFS filesystems
 * in test environments.
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>

#include "../../src/lib/env.hpp"
#include "../../src/lib/subprocess.hpp"
#include "../../src/std/expected.hpp"

namespace fs = std::filesystem;

/**
 * @namespace ns_test
 * @brief Test utilities for FlatImage testing
 */
namespace ns_test
{

  namespace
  {

    /// Path to the dwarfs_aio binary
    inline fs::path g_path_dwarfs_aio = "/tmp/dwarfs_aio";

    /**
     * @brief Ensures dwarfs_aio binary is downloaded and executable
     *
     * Downloads the binary from GitHub releases if it doesn't exist,
     * and makes it executable. Subsequent calls are fast if already set up.
     *
     * @return Value<void> Success or error message
     */
    inline Value<void> ensure_dwarfs_binary()
    {
      // Check if binary already exists and is executable
      if (fs::exists(g_path_dwarfs_aio))
      {
        auto perms = fs::status(g_path_dwarfs_aio).permissions();
        if ((perms & fs::perms::owner_exec) != fs::perms::none)
        {
          return {};
        }
      }

      // Download the binary
      const std::string url =
          "https://github.com/flatimage/tools/releases/latest/download/dwarfs_aio-x86_64";

      std::ostringstream output;
      fs::path path_bin_wget = Pop(ns_env::search_path("wget"));
      auto child = ns_subprocess::Subprocess(path_bin_wget)
                       .with_args(url, "-O", g_path_dwarfs_aio)
                       .with_stdio(ns_subprocess::Stream::Pipe)
                       .with_streams(std::cin, output, output)
                       .spawn();

      return_if(not child, Error("E::Failed to spawn wget process"));

      int exit_code = child->wait().value_or(-1);

      if (exit_code != 0)
      {
        std::string output_str = output.str();
        return Error("E::wget failed with exit code: {} for URL: {}\nOutput: {}",
                     exit_code,
                     url,
                     output_str);
      }

      // Make the binary executable
      using enum fs::perms;
      fs::permissions(g_path_dwarfs_aio,
                      owner_read | owner_write | owner_exec | group_read | group_exec | others_read
                          | others_exec,
                      fs::perm_options::replace);

      return_if(not fs::exists(g_path_dwarfs_aio), Error("E::Binary not found after download"));

      return {};
    }

  } // anonymous namespace

  /**
   * @brief Creates a DwarFS filesystem image from a directory
   *
   * Downloads dwarfs_aio if needed and creates a compressed DwarFS image
   * from the specified source directory.
   *
   * @param path_dir_source Source directory to compress
   * @param path_file_output Output .dwarfs file path
   * @return Value<void> Success or error message
   *
   * @code
   * fs::path source = "/tmp/my_files";
   * fs::path image = "/tmp/my_image.dwarfs";
   * fs::create_directories(source);
   * std::ofstream(source / "test.txt") << "Hello!";
   *
   * auto result = ns_test::create_dwarfs(source, image);
   * if (result.has_value()) {
   *     // Image created successfully
   * }
   * @endcode
   */
  inline Value<void> create_dwarfs(fs::path const& path_dir_source,
                                   fs::path const& path_file_output)
  {
    // Ensure dwarfs binary is available
    Pop(ensure_dwarfs_binary());

    // Validate source directory exists
    return_if(not fs::exists(path_dir_source),
              Error("E::Source directory does not exist: {}", path_dir_source.string()));
    return_if(not fs::is_directory(path_dir_source),
              Error("E::Source path is not a directory: {}", path_dir_source.string()));

    // Create the DwarFS image using mkdwarfs
    std::ostringstream output;
    auto child =
        ns_subprocess::Subprocess(g_path_dwarfs_aio)
            .with_args("--tool=mkdwarfs", "-i", path_dir_source, "-o", path_file_output, "-l", "0")
            .with_stdio(ns_subprocess::Stream::Pipe)
            .with_streams(std::cin, output, output)
            .spawn();

    return_if(not child, Error("E::Failed to spawn mkdwarfs process"));

    int exit_code = child->wait().value_or(-1);

    if (exit_code != 0)
    {
      std::string output_str = output.str();
      return Error("E::mkdwarfs failed with exit code: {} (source: {}, output: {})\nOutput: {}",
                   exit_code,
                   path_dir_source.string(),
                   path_file_output.string(),
                   output_str);
    }

    return_if(not fs::exists(path_file_output),
              Error("E::Output file not created: {}", path_file_output.string()));

    return {};
  }

  /**
   * @brief Mounts a DwarFS filesystem image to a temporary directory
   *
   * Mounts the specified DwarFS image file to a unique temporary directory
   * and returns the mount point path. The caller is responsible for unmounting
   * when done.
   *
   * @param path_file_image Path to the .dwarfs image file
   * @return Value<fs::path> Mount point path or error message
   *
   * @code
   * fs::path image = "/tmp/my_image.dwarfs";
   * auto mount_result = ns_test::mount_dwarfs(image);
   *
   * if (mount_result.has_value()) {
   *     fs::path mount_point = mount_result.value();
   *     // Use the mounted filesystem
   *     auto file = mount_point / "test.txt";
   *
   *     // Remember to unmount when done
   *     ns_fuse::unmount(mount_point);
   * }
   * @endcode
   */
  inline Value<fs::path> mount_dwarfs(fs::path const& path_file_image)
  {
    using namespace std::chrono_literals;

    // Ensure dwarfs binary is available
    Pop(ensure_dwarfs_binary());

    // Validate image file exists
    return_if(not fs::exists(path_file_image),
              Error("E::Image file does not exist: {}", path_file_image.string()));

    // Create unique temporary mount point
    auto unique_suffix =
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    fs::path path_dir_mount = fs::temp_directory_path() / ("dwarfs_mount_" + unique_suffix);
    fs::create_directories(path_dir_mount);

    // Mount the DwarFS image (runs in foreground, so we spawn it as a background process)
    std::ostringstream output;
    auto child = ns_subprocess::Subprocess(g_path_dwarfs_aio)
                     .with_args("--tool=dwarfs", path_file_image, path_dir_mount)
                     .with_stdio(ns_subprocess::Stream::Pipe)
                     .with_streams(std::cin, output, output)
                     .spawn();

    if (not child)
    {
      std::string output_str = output.str();
      return Error("E::Failed to spawn dwarfs mount process for image: {}\nOutput: {}",
                   path_file_image.string(),
                   output_str);
    }

    // Give the filesystem time to mount (dwarfs runs in foreground)
    // We need to wait a bit for the mount to complete
    std::this_thread::sleep_for(500ms);

    return path_dir_mount;
  }

} // namespace ns_test

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
