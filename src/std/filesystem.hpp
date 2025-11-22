/**
 * @file filesystem.hpp
 * @author Ruan Formigoni
 * @brief Filesystem helpers
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstring>
#include <expected>
#include <vector>
#include <ranges>
#include <string>
#include <filesystem>
#include <linux/limits.h>

#include "expected.hpp"
#include "string.hpp"

/**
 * @namespace ns_fs
 * @brief Enhanced filesystem utilities wrapping std::filesystem
 *
 * This namespace extends std::filesystem with additional helper functions for common filesystem
 * operations.
 */
namespace ns_fs
{

namespace fs = std::filesystem;

using path = fs::path;
using perms = fs::perms;
using copy_options = fs::copy_options;
using perm_options = fs::perm_options;

/**
 * @brief Resolves an input path using realpath(3)
 *
 * @param path_file_src Path to resolve
 * @return std::expected<fs::path, std::string> The resolved path or error message
 */
[[nodiscard]] inline std::expected<fs::path, std::string> realpath(fs::path const& path_file_src)
{
  char str_path_file_resolved[PATH_MAX];
  if ( ::realpath(path_file_src.c_str(), str_path_file_resolved) == nullptr )
  {
    return std::unexpected<std::string>(strerror(errno));
  }
  return fs::path{str_path_file_resolved};
}

/**
 * @brief List the files in a directory
 *
 * @param path_dir_src Path to the directory to query for files
 * @return Value<std::vector<fs::path>> The list of files or the respective error
 */
[[nodiscard]] inline Value<std::vector<fs::path>> regular_files(fs::path const& path_dir_src)
{
  std::error_code ec;
  std::vector<fs::path> files = fs::directory_iterator(path_dir_src, ec)
    | std::views::transform([](auto&& e){ return e.path(); })
    | std::views::filter([](auto&& e){ return fs::is_regular_file(e); })
    | std::ranges::to<std::vector<fs::path>>();
  if(ec)
  {
    return std::unexpected(ec.message());
  }
  return files;
}

/**
 * @brief Creates directories recursively.
 * @param p The path of directories to create.
 * @return std::expected<void, std::string> The directory path if it exists or was just created
 * , or error message.
 */
[[nodiscard]] inline Value<fs::path> create_directories(fs::path const& p)
{
    if(Try(fs::exists(p)))
    {
      return p;
    }
    if (not Try(fs::create_directories(p)))
    {
      return std::unexpected(std::format("Could not create directory {}", p.string()));
    }
    return p;
}

/**
 * @brief Replace placeholders in a path by traversing components
 *
 * This function iterates through each component of the path and replaces
 * matching components with the provided arguments in order.
 *
 * @tparam Args The types of the arguments to replace
 * @param path The path with placeholder components
 * @param args The values to replace matching placeholders with
 * @return The path with placeholders replaced
 */
template<typename... Args>
[[nodiscard]] inline fs::path placeholders_replace(fs::path const& path, Args&&... args)
{
  return ns_string::placeholders_replace(path, std::forward<Args>(args)...);
}

} // namespace: ns_fs
