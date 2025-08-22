/**
 * @file layers.hpp
 * @author Ruan Formigoni
 * @brief Manages filesystem layers in FlatImage
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <unistd.h>

#include "../../lib/subprocess.hpp"
#include "../../lib/env.hpp"

namespace
{

namespace fs = std::filesystem;

}

namespace ns_layers
{

/**
 * @brief Creates a layer (filesystem) from a source directory
 * 
 * @param path_dir_src Path to the source directory
 * @param path_file_dst Path to the output filesystem file
 * @param path_file_list Path to a temporary file to store the list of files to compress
 * @param compression_level The compression level to create the filesystem
 * @return Expected<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> create(fs::path const& path_dir_src
  , fs::path const& path_file_dst
  , fs::path const& path_file_list
  , uint64_t compression_level)
{
  // Find mkdwarfs binary
  auto path_file_mkdwarfs = Expect(ns_env::search_path("mkdwarfs"));
  // Compression level must be at least 1 and less or equal to 10
  compression_level = std::clamp(compression_level, uint64_t{0}, uint64_t{9});
  // Search for all viable files to compress
  ns_log::info()("Gathering files to compress...");
  std::ofstream file_list(path_file_list, std::ios::out | std::ios::trunc);
  qreturn_if(not file_list.is_open()
    , Unexpected("Could not open list of files '{}' to compress"_fmt(path_file_list))
  );
  // Check if source directory exists and is a directory
  qreturn_if(not fs::exists(path_dir_src), Unexpected("Source directory '{}' does not exist"_fmt(path_dir_src)));
  qreturn_if(not fs::is_directory(path_dir_src), Unexpected("Source '{}' is not a directory"_fmt(path_dir_src)));
  // Gather files to compress
  for(auto&& entry = fs::recursive_directory_iterator(path_dir_src)
    ; entry != fs::recursive_directory_iterator()
    ; ++entry)
  {
    // Get full file path to entry
    fs::path path_entry = entry->path();
    // Is a directory
    if(entry->is_directory())
    {
      // Ignore directory as it is not traverseable
      if(::access(path_entry.c_str(), R_OK | X_OK) != 0)
      {
        ns_log::info()("Insufficient permissions to enter directory '{}'", path_entry);
        entry.disable_recursion_pending();
        continue;
      }
      // Add empty directory to the list
      if(fs::is_empty(path_entry))
      {
        file_list
          << path_entry.lexically_relative(path_dir_src).string()
          << '\n';      
      }
    }
    // Regular file or symlink
    else if(entry->is_regular_file() or entry->is_symlink())
    {
      file_list
        << path_entry.lexically_relative(path_dir_src).string()
        << '\n';      
    }
    // Unsupported for compression
    else
    {
      ns_log::info()("Ignoring file '{}'", path_entry);
    }
  }
  file_list.close();

  // Compress filesystem
  ns_log::info()("Compression level: '{}'", compression_level);
  ns_log::info()("Compress filesystem to '{}'", path_file_dst);
  auto ret = ns_subprocess::Subprocess(path_file_mkdwarfs)
    .with_args("-f")
    .with_args("-i", path_dir_src, "-o", path_file_dst)
    .with_args("-l", compression_level)
    .with_args("--input-list", path_file_list)
    .spawn()
    .wait();
  qreturn_if(not ret, Unexpected("mkdwarfs process exited abnormally"));
  qreturn_if(ret.value() != 0, Unexpected("mkdwarfs process exited with error code '{}'", ret.value()));
  return {};
}

/**
 * @brief Includes a filesystem in the target FlatImage
 * 
 * @param path_file_binary Path to the target FlatImage in which to include the filesystem
 * @param path_file_layer Path to the filesystem to include in the FlatImage
 * @return Expected<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> add(fs::path const& path_file_binary, fs::path const& path_file_layer)
{
  // Open binary file for writing
  std::ofstream file_binary(path_file_binary, std::ios::app | std::ios::binary);
  qreturn_if(not file_binary.is_open(), Unexpected("Failed to open output file '{}'"_fmt(path_file_binary)));
  std::ifstream file_layer(path_file_layer, std::ios::in | std::ios::binary);
  qreturn_if(not file_layer.is_open(), Unexpected("Failed to open input file '{}'"_fmt(path_file_layer)));
  // Get byte size
  uint64_t file_size = fs::file_size(path_file_layer);
  // Write byte size
  file_binary.write(reinterpret_cast<char*>(&file_size), sizeof(file_size));
  char buff[8192];
  while( file_layer.read(buff, sizeof(buff)) or file_layer.gcount() > 0 )
  {
    file_binary.write(buff, file_layer.gcount());
    qreturn_if(not file_binary, Unexpected("Error writing data to file"));
  }
  ns_log::info()("Included novel layer from file '{}'", path_file_layer);
  return {};
}

} // namespace ns_layers

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
