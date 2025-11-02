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
#include "../../std/expected.hpp"

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
  // Compression level
  compression_level = std::clamp(compression_level, uint64_t{0}, uint64_t{9});
  // Search for all viable files to compress
  ns_log::info()("Gathering files to compress...");
  std::ofstream file_list(path_file_list, std::ios::out | std::ios::trunc);
  qreturn_if(not file_list.is_open()
    , Unexpected("E::Could not open list of files '{}' to compress", path_file_list)
  );
  // Check if source directory exists and is a directory
  qreturn_if(not fs::exists(path_dir_src), Unexpected("E::Source directory '{}' does not exist", path_dir_src));
  qreturn_if(not fs::is_directory(path_dir_src), Unexpected("E::Source '{}' is not a directory", path_dir_src));
  // Gather files to compress
  for(auto&& entry = fs::recursive_directory_iterator(path_dir_src)
    ; entry != fs::recursive_directory_iterator()
    ; ++entry)
  {
    // Get full file path to entry
    fs::path path_entry = entry->path();
    // Regular file or symlink
    if(entry->is_regular_file() or entry->is_symlink())
    {
      file_list
        << path_entry.lexically_relative(path_dir_src).string()
        << '\n';      
    }
    // Is a directory
    else if(entry->is_directory())
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
  qreturn_if(not ret, Unexpected("E::mkdwarfs process exited abnormally"));
  qreturn_if(ret.value() != 0, Unexpected("E::mkdwarfs process exited with error code '{}'", ret.value()));
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
  qreturn_if(not file_binary.is_open(), Unexpected("E::Failed to open output file '{}'", path_file_binary));
  std::ifstream file_layer(path_file_layer, std::ios::in | std::ios::binary);
  qreturn_if(not file_layer.is_open(), Unexpected("E::Failed to open input file '{}'", path_file_layer));
  // Get byte size
  uint64_t file_size = fs::file_size(path_file_layer);
  // Write byte size
  file_binary.write(reinterpret_cast<char*>(&file_size), sizeof(file_size));
  char buff[8192];
  while( file_layer.read(buff, sizeof(buff)) or file_layer.gcount() > 0 )
  {
    file_binary.write(buff, file_layer.gcount());
    qreturn_if(not file_binary, Unexpected("E::Error writing data to file"));
  }
  ns_log::info()("Included novel layer from file '{}'", path_file_layer);
  return {};
}

/**
 * @brief Commit changes into a novel layer and appends it to the FlatImage
 * 
 * @param path_file_binary
 * @param path_dir_src
 * @param path_file_layer_tmp
 * @param path_file_list_tmp
 * @param layer_compression_level
 * @return Expected<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> commit(
    fs::path const& path_file_binary
  , fs::path const& path_dir_src
  , fs::path const& path_file_layer_tmp
  , fs::path const& path_file_list_tmp
  , uint32_t layer_compression_level)
{
  // Create filesystem based on the contents of src
  Expect(ns_layers::create(path_dir_src
    , path_file_layer_tmp
    , path_file_list_tmp
    , layer_compression_level
  ));
  // Include filesystem in the image
  Expect(add(path_file_binary, path_file_layer_tmp));
  // Remove layer file
  std::error_code ec;
  if(not fs::remove(path_file_layer_tmp))
  {
    ns_log::error()("Could not erase layer file '{}'", path_file_layer_tmp.string());
  }
  // Remove files from the compression list
  std::ifstream file_list(path_file_list_tmp);
  qreturn_if(not file_list.is_open(), Unexpected("E::Could not open file list for erasing files..."));
  std::string line;
  while(std::getline(file_list, line))
  {
    fs::path path_file_target = path_dir_src / line;
    fs::path path_dir_parent = path_file_target.parent_path();
    // Remove target file
    if(not fs::remove(path_file_target, ec))
    {
      ns_log::error()("Could not remove file {}"_fmt(path_file_target));
    }
    // Remove empty directory
    if(fs::is_empty(path_dir_parent, ec) and not fs::remove(path_dir_parent, ec))
    {
      ns_log::error()("Could not remove directory {}"_fmt(path_dir_parent));
    }
  }
  return {};
}

} // namespace ns_layers

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
