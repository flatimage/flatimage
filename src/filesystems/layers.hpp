/**
 * @file layers.hpp
 * @author Ruan Formigoni
 * @brief Layer management for DwarFS filesystems
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>
#include <ranges>
#include <algorithm>

#include "../std/expected.hpp"
#include "../std/filesystem.hpp"
#include "../lib/env.hpp"
#include "../macro.hpp"
#include "dwarfs.hpp"

namespace ns_filesystems::ns_layers
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @class Layers
 * @brief Manages external DwarFS layer files and directories for the filesystem controller
 *
 * Provides a unified interface for collecting layer files from both individual file paths
 * and directories. Supports the FIM_LAYERS environment variable which accepts a colon-separated
 * list of paths that can be files or directories.
 *
 * **Directory Scanning:**
 * - Non-recursive (only scans direct children)
 * - Alphabetical order within each directory
 * - Validates files as DwarFS filesystems
 *
 * **File Processing:**
 * - Direct file paths are validated and added immediately
 * - Invalid files are skipped with a warning
 *
 * @example
 * @code
 * Layers layers;
 * layers.push_from_var("FIM_LAYERS");  // Process FIM_LAYERS env var
 * layers.push("/path/to/layers");       // Add directory or file
 * auto const& paths = layers.get_layers();  // Retrieve all layer paths
 * @endcode
 */
class Layers
{
  private:
    struct Layer
    {
      fs::path const path;
      uint64_t offset;
      uint64_t size;
    };
    std::vector<Layer> layers;  ///< Collection of validated layer file paths with offsets

    /**
     * @brief Validates and appends a single layer file
     *
     * Checks if the file is a valid DwarFS filesystem by examining magic bytes.
     * Invalid files are skipped with a warning.
     *
     * @param path Path to the layer file to validate and add
     */
    [[nodiscard]] Value<void> append_file(fs::path const& path)
    {
      return_if(not ns_dwarfs::is_dwarfs(path), Error("W::Skipping invalid dwarfs filesystem '{}'", path));
      layers.push_back({path, 0, std::filesystem::file_size(path)});
      return {};
    }

    /**
     * @brief Scans a directory for layer files and appends them
     *
     * Performs non-recursive directory scanning to collect regular files.
     * Sorts files lexicographically
     * Each file is validated via append_file() before being added.
     *
     * @param path Path to directory to scan
     */
    [[nodiscard]] Value<void> append_directory(fs::path const& path)
    {
      // Get all regular files from the directory
      auto result = Pop(ns_fs::regular_files(path));
      // Sort layers files
      std::ranges::sort(result);
      // Process each file
      for(auto&& file : result)
      {
        append_file(file).discard("W::Failed to append layer from directory");
      }
      return {};
    }

  public:
    /**
     * @brief Adds a layer from a file or directory path
     *
     * Automatically detects whether the path is a file or directory and processes accordingly:
     * - **File:** Validates and adds directly
     * - **Directory:** Scans for layer files and adds them alphabetically
     *
     * @param path Filesystem path (file or directory)
     * @return Value<void> Success or error
     */
    [[nodiscard]] Value<void> push(fs::path const& path)
    {
      if(Try(fs::is_regular_file(path)))
      {
        append_file(path).discard("W::Failed to append layer from regular file");
      }
      else if(Try(fs::is_directory(path)))
      {
        append_directory(path).discard("W::Failed to append layer from directory");
      }
      return {};
    }

    /**
     * @brief Loads layers from a colon-separated environment variable
     *
     * Processes environment variables like FIM_LAYERS which contain colon-separated
     * paths. Each path can be either a file or directory. Performs shell expansion
     * on variable values before processing.
     *
     * **Processing Steps:**
     * 1. Retrieves environment variable value
     * 2. Performs word expansion (variables, subshells)
     * 3. Splits on ':' delimiter
     * 4. Processes each path via push()
     *
     * @param var Name of environment variable to read (e.g., "FIM_LAYERS")
     * @return Value<void> Success or error
     */
    [[nodiscard]] Value<void> push_from_var(std::string_view var)
    {
      for(fs::path const& path : ns_env::get_expected<"Q">(var)
        // Perform word expansions (variables, run subshells...)
        .transform([](auto&& e){ return ns_env::expand(e).value_or(std::string{e}); })
        // Get value or default to empty
        .value_or(std::string{})
        // Split values variable on ':'
        | std::views::split(':')
        // Create a path
        | std::views::transform([](auto&& e){ return fs::path(e.begin(), e.end()); })
      )
      {
        Pop(push(path));
      }
      return {};
    }

    /**
     * @brief Retrieves the collected layer file paths with offsets
     *
     * Returns the final list of validated layer files with their offsets in the order they were added.
     * This is used by the filesystem controller to mount layers sequentially.
     *
     * @return const reference to vector of (path, offset) pairs
     */
    std::vector<Layer> const& get_layers() const
    {
      return layers;
    }

    /**
     * @brief Scans a binary file for embedded DwarFS filesystems
     *
     * Reads a binary file from the given offset, looking for concatenated DwarFS filesystems.
     * Each filesystem is prefixed with an 8-byte size field. Appends found filesystems to the
     * layers collection with their respective offsets.
     *
     * **Format:**
     * - 8 bytes: filesystem size (int64_t)
     * - N bytes: DwarFS filesystem data
     * - (repeats for additional filesystems)
     *
     * @param path_file_binary Path to the binary file to scan
     * @param offset Initial offset in bytes where scanning begins
     * @return Value<void> Success or error
     */
    void push_binary(fs::path const& path_file_binary, uint64_t offset)
    {
      // Open the binary file
      std::ifstream file_binary(path_file_binary, std::ios::binary);

      // Advance to starting offset
      file_binary.seekg(offset);

      // Scan for filesystems concatenated in the binary
      while (true)
      {
        // Read filesystem size
        uint64_t size_fs;
        break_if(not file_binary.read(reinterpret_cast<char*>(&size_fs), sizeof(size_fs))
          , "D::Stopped reading at offset {}", offset
        );
        logger("D::Filesystem size is '{}'", size_fs);
        // Validate filesystem size
        break_if(size_fs <= 0, "E::Invalid filesystem size '{}' at offset {}", size_fs, offset);
        // Skip size bytes to point to filesystem data
        offset += 8;
        // Check if filesystem is of type 'DWARFS'
        break_if(not ns_dwarfs::is_dwarfs(path_file_binary, offset)
          , "E::Invalid dwarfs filesystem appended on the image"
        );
        // Store the filesystem with its offset
        layers.push_back({path_file_binary, offset, size_fs});
        // Move to next filesystem position
        offset += size_fs;
        file_binary.seekg(offset);
      }

      file_binary.close();
    }
};

} // ns_filesystems::ns_layers