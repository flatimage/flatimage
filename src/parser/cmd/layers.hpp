/**
 * @file layers.hpp
 * @author Ruan Formigoni
 * @brief Manages filesystem layers in FlatImage
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <algorithm>
#include <filesystem>
#include <regex>
#include <unistd.h>
#include <iostream>
#include <format>

#include "../../lib/subprocess.hpp"
#include "../../lib/env.hpp"
#include "../../std/expected.hpp"
#include "../../filesystems/layers.hpp"

namespace
{

namespace fs = std::filesystem;

}

/**
 * @namespace ns_layers
 * @brief Filesystem layer management command implementation
 *
 * Implements the fim-layer command for creating, listing, and managing DwarFS compressed
 * filesystem layers. Supports layer commit operations to snapshot the current writable overlay
 * into a compressed read-only layer, and layer enumeration to display all available layers.
 */
namespace ns_layers
{

/**
 * @brief Creates a layer (filesystem) from a source directory
 *
 * @param path_dir_src Path to the source directory
 * @param path_file_dst Path to the output filesystem file
 * @param path_file_list Path to a temporary file to store the list of files to compress
 * @param compression_level The compression level to create the filesystem
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> create(fs::path const& path_dir_src
  , fs::path const& path_file_dst
  , fs::path const& path_file_list
  , uint64_t compression_level)
{
  // Find mkdwarfs binary
  auto path_file_mkdwarfs = Pop(ns_env::search_path("mkdwarfs"));
  // Compression level
  return_if(compression_level > 9, Error("E::Out-of-bounds compression level '{}'", compression_level));
  // Search for all viable files to compress
  logger("I::Gathering files to compress...");
  std::ofstream file_list(path_file_list, std::ios::out | std::ios::trunc);
  return_if(not file_list.is_open()
    , Error("E::Could not open list of files '{}' to compress", path_file_list)
  );
  // Check if source directory exists and is a directory
  return_if(not Catch(fs::exists(path_dir_src)).value_or(false), Error("E::Source directory '{}' does not exist", path_dir_src));
  return_if(not Catch(fs::is_directory(path_dir_src)).value_or(false), Error("E::Source '{}' is not a directory", path_dir_src));
  // Gather files to compress
  for(auto&& entry = Try(fs::recursive_directory_iterator(path_dir_src))
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
        logger("I::Insufficient permissions to enter directory '{}'", path_entry);
        entry.disable_recursion_pending();
        continue;
      }
      // Add empty directory to the list
      if(Catch(fs::is_empty(path_entry)).value_or(false))
      {
        file_list
          << path_entry.lexically_relative(path_dir_src).string()
          << '\n';
      }
    }
    // Unsupported for compression
    else
    {
      logger("I::Ignoring file '{}'", path_entry);
    }
  }
  file_list.close();

  // Compress filesystem
  logger("I::Compression level: '{}'", compression_level);
  logger("I::Compress filesystem to '{}'", path_file_dst);
  Pop(ns_subprocess::Subprocess(path_file_mkdwarfs)
    .with_args("-f")
    .with_args("-i", path_dir_src, "-o", path_file_dst)
    .with_args("-l", compression_level)
    .with_args("--input-list", path_file_list)
    .spawn()->wait());
  return{};
}

/**
 * @brief Includes a filesystem in the target FlatImage
 *
 * @param path_file_binary Path to the target FlatImage in which to include the filesystem
 * @param path_file_layer Path to the filesystem to include in the FlatImage
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> add(fs::path const& path_file_binary, fs::path const& path_file_layer)
{
  // Open binary file for writing
  std::ofstream file_binary(path_file_binary, std::ios::app | std::ios::binary);
  return_if(not file_binary.is_open(), Error("E::Failed to open output file '{}'", path_file_binary));
  std::ifstream file_layer(path_file_layer, std::ios::in | std::ios::binary);
  return_if(not file_layer.is_open(), Error("E::Failed to open input file '{}'", path_file_layer));
  // Get byte size
  uint64_t file_size = Try(fs::file_size(path_file_layer));
  // Write byte size
  file_binary.write(reinterpret_cast<char*>(&file_size), sizeof(file_size));
  char buff[8192];
  while( file_layer.read(buff, sizeof(buff)) or file_layer.gcount() > 0 )
  {
    file_binary.write(buff, file_layer.gcount());
    return_if(not file_binary, Error("E::Error writing data to file"));
  }
  logger("I::Included novel layer from file '{}'", path_file_layer);
  return {};
}

/**
 * @brief Finds the next available layer number in the layers directory
 *
 * @param path_dir_layers Path to the layers directory
 * @return uint64_t Next available layer number (0-indexed)
 */
[[nodiscard]] inline Value<uint64_t> find_next_layer_number(fs::path const& path_dir_layers)
{
  // Check if layers directory is accessible
  if (not Try(fs::exists(path_dir_layers)) or not Try(fs::is_directory(path_dir_layers)))
  {
    return Error("E::Layers directory is missing or not a directory");
  }
  // Get current layers
  std::regex regex_layer(R"(layer-[0-9]+\.layer)");
  auto vec = Try(fs::directory_iterator(path_dir_layers)
    | std::views::filter([](auto&& e){ return e.is_regular_file(); })
    | std::views::transform([](auto&& e){ return e.path().filename().string(); })
    | std::views::filter([&](auto&& e){ return std::regex_search(e, regex_layer); })
    | std::views::transform([](auto&& e){ return std::stoi(e.substr(6, e.substr(6).find('.'))); })
    | std::ranges::to<std::vector<int>>()
  );
  // Get max layer number
  int next = (vec.empty())? 0 : *std::ranges::max_element(vec) + 1;
  return_if(next > 999, Error("E::Maximum number of layers exceeded"));
  return next;
}

/**
 * @brief Handles layer placement based on commit mode
 *
 * @param path_file_binary Path to the FlatImage binary
 * @param path_file_layer_tmp Temporary layer file path
 * @param mode Commit mode (binary, layer, or file)
 * @param path_dst Destination path (required for layer/file modes)
 * @return Value<void> Nothing on success, or the respective error
 */
enum class CommitMode { BINARY, LAYER, FILE };

[[nodiscard]] inline Value<void> commit_mode(
    fs::path const& path_file_binary
  , fs::path const& path_file_layer_tmp
  , CommitMode mode
  , std::optional<fs::path> const& path_dst = std::nullopt)
{
  switch(mode)
  {
    case CommitMode::BINARY:
    {
      // Include filesystem in the image
      Pop(add(path_file_binary, path_file_layer_tmp));
      // Remove layer file
      return_if(not Catch(fs::remove(path_file_layer_tmp)).value_or(false)
        , Error("E::Could not erase layer file '{}'", path_file_layer_tmp.string())
      );
      logger("I::Filesystem appended to binary without errors");
    }
    break;
    case CommitMode::LAYER:
    {
      return_if(not path_dst.has_value(), Error("E::Layer mode requires a destination directory"));
      return_if(not Catch(fs::is_directory(path_dst.value())).value_or(false)
        , Error("E::Destination should be a directory")
      );
      // Find next available layer number, return on error
      uint64_t layer_num = Pop(find_next_layer_number(path_dst.value()));
      // Create layer path as layer-xxx.layer
      fs::path layer_path = path_dst.value() / std::format("layer-{:03d}.layer", layer_num);
      logger("I::Layer number: {}", layer_num);
      logger("I::Layer path: {}", layer_path);
      // Move layer file to layers directory, or return on error
      Try(fs::rename(path_file_layer_tmp, layer_path));
      logger("I::Layer saved to '{}'", layer_path.string());
    }
    break;
    case CommitMode::FILE:
    {
      // Return if no destination file was specified
      return_if(not path_dst.has_value(), Error("E::File mode requires a destination path"));
      // Return if file exists or check fails
      return_if(Try(fs::exists(path_dst.value())), Error("E::Destination file already exists"));
      // Move layer file to destination, or return on error
      Try(fs::rename(path_file_layer_tmp, path_dst.value()));
      logger("I::Layer saved to '{}'", path_dst.value().string());
    }
    break;
  }
  return {};
}

/**
 * @brief Commit changes into a novel layer (binary/layer/file modes)
 *
 * @param path_file_binary Path to the FlatImage binary
 * @param path_dir_src Source directory with overlay changes
 * @param path_file_layer_tmp Temporary layer file path
 * @param path_file_list_tmp Temporary file list path
 * @param layer_compression_level Compression level for the layer
 * @param mode Commit mode (binary, layer, or file)
 * @param path_dst Destination path (only for file mode)
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> commit(
    fs::path const& path_file_binary
  , fs::path const& path_dir_src
  , fs::path const& path_file_layer_tmp
  , fs::path const& path_file_list_tmp
  , uint32_t layer_compression_level
  , CommitMode mode
  , std::optional<fs::path> const& path_dst = std::nullopt)
{
  // Create filesystem based on the contents of src
  Pop(ns_layers::create(path_dir_src
    , path_file_layer_tmp
    , path_file_list_tmp
    , layer_compression_level
  ));
  // Handle the layer based on the commit mode
  Pop(commit_mode(path_file_binary, path_file_layer_tmp, mode, path_dst));
  // Remove files from the compression list
  std::ifstream file_list(path_file_list_tmp);
  return_if(not file_list.is_open(), Error("E::Could not open file list for erasing files..."));
  std::string line;
  // getline doesn't throw by default, no need to wrap
  while(std::getline(file_list, line))
  {
    fs::path path_file_target = path_dir_src / line;
    fs::path path_dir_parent = path_file_target.parent_path();
    // Remove target file, permissive
    if(not Catch(fs::remove(path_file_target)).value_or(false))
    {
      logger("W::Could not remove file {}", path_file_target.string());
    }
    // Remove empty directory, permissive
    if(Catch(fs::is_empty(path_dir_parent)).value_or(false))
    {
      if(not Catch(fs::remove(path_dir_parent)).value_or(false))
      {
        logger("W::Could not remove directory {}", path_dir_parent.string());
      }
    }
  }
  logger("I::Finished erasing files");
  return {};
}

/**
 * @brief Lists all layers in the format index:offset:size:path
 *
 * @param layers The Layers object containing all filesystem layers
 */
inline void list(ns_filesystems::ns_layers::Layers const& layers)
{
  for(uint64_t index = 0; auto const& layer : layers.get_layers())
  {
    std::cout << std::format("{}:{}:{}:{}\n", index, layer.offset, layer.size, layer.path.string());
    ++index;
  }
}

} // namespace ns_layers

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
