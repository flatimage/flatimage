///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : layers
///

#pragma once

#include <filesystem>
#include <unistd.h>

#include "../../lib/subprocess.hpp"

namespace
{

namespace fs = std::filesystem;

}

namespace ns_layers
{

// fn: create() {{{
inline void create(fs::path const& path_dir_src
  , fs::path const& path_file_dst
  , fs::path const& path_file_list_tmp
  , uint64_t compression_level)
{
  // Find mkdwarfs binary
  auto opt_path_file_mkdwarfs = ns_subprocess::search_path("mkdwarfs");
  ethrow_if(not opt_path_file_mkdwarfs, "Could not find 'mkdwarfs' binary");

  // Compression level must be at least 1 and less or equal to 10
  compression_level = std::clamp(compression_level, uint64_t{0}, uint64_t{9});
  
  // Search for all viable files to compress
  ns_log::info()("Gathering files to compress...");
  std::ofstream file_list(path_file_list_tmp, std::ios::out | std::ios::trunc);
  ethrow_if(not file_list.is_open(), "Could not open list of files to compress");
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
  auto ret = ns_subprocess::Subprocess(*opt_path_file_mkdwarfs)
    .with_args("-f")
    .with_args("-i", path_dir_src, "-o", path_file_dst)
    .with_args("-l", compression_level)
    .with_args("--input-list", path_file_list_tmp)
    .spawn()
    .wait();
  ethrow_if(not ret, "mkdwarfs process exited abnormally");
  ethrow_if(*ret != 0, "mkdwarfs process exited with error code '{}'"_fmt(*ret));
} // fn: create() }}}

// fn: add() {{{
inline void add(fs::path const& path_file_binary, fs::path const& path_file_layer)
{
  // Open binary file for writing
  std::ofstream file_binary(path_file_binary, std::ios::app | std::ios::binary);
  std::ifstream file_layer(path_file_layer, std::ios::in | std::ios::binary);
  ereturn_if(not file_binary.is_open(), "Failed to open output file '{}'"_fmt(path_file_binary))
  ereturn_if(not file_layer.is_open(), "Failed to open input file '{}'"_fmt(path_file_layer))
  // Get byte size
  uint64_t file_size = fs::file_size(path_file_layer);
  // Write byte size
  file_binary.write(reinterpret_cast<char*>(&file_size), sizeof(file_size));
  char buff[8192];
  while( file_layer.read(buff, sizeof(buff)) or file_layer.gcount() > 0 )
  {
    file_binary.write(buff, file_layer.gcount());
    ereturn_if(not file_binary, "Error writing data to file");
  } // while
  ns_log::info()("Included novel layer from file '{}'", path_file_layer);
} // fn: add() }}}

} // namespace ns_layers

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
