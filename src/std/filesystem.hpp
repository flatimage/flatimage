/**
 * @file filesystem.hpp
 * @author Ruan Formigoni
 * @brief Filesystem helpers
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstring>
#include <filesystem>
#include <expected>
#include <linux/limits.h>
#include <ranges>

#include "../macro.hpp"
#include "expected.hpp"

#include <filesystem>
#include <expected>
#include <system_error>

namespace ns_fs
{

namespace fs = std::filesystem;

using path = fs::path;
using perms = fs::perms;
using copy_options = fs::copy_options;
using perm_options = fs::perm_options;

/**
 * @brief Gets the path to the current binary from /proc/self/exe
 * 
 * @return Expected<fs::path> The path to self or the respective error
 */
[[nodiscard]] inline Expected<fs::path> file_self()
{
  std::error_code ec;
  auto path_file_self = fs::read_symlink("/proc/self/exe", ec);
  if ( ec )
  {
    return std::unexpected("Failed to fetch location of self");
  }
  return path_file_self;
}

/**
 * @brief Resolves an input path
 * 
 * @param path_file_src Path to resolve
 * @return Expected<fs::path> The resolve path or the respective error
 */
[[nodiscard]] inline Expected<fs::path> realpath(fs::path const& path_file_src)
{
  char str_path_file_resolved[PATH_MAX];
  if ( ::realpath(path_file_src.c_str(), str_path_file_resolved) == nullptr )
  {
    return std::unexpected(strerror(errno));
  }
  return str_path_file_resolved;
}

/**
 * @brief List the files in a directory
 * 
 * @param path_dir_src Path to the directory to query for files
 * @return Expected<std::vector<fs::path>> The list of files or the respective error
 */
[[nodiscard]] inline Expected<std::vector<fs::path>> regular_files(fs::path const& path_dir_src)
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
 * @brief Checks whether path refers to existing file system object.
 * @param p The path to check.
 * @return std::expected<bool, std::string> True if the path exists, or error message.
 */
[[nodiscard]] inline Expected<bool> exists(const fs::path& p)
{
    std::error_code ec;
    bool result = fs::exists(p, ec);
    if (ec) {
        return std::unexpected(ec.message());
    }
    return result;
}

/**
 * @brief Creates directories recursively.
 * @param p The path of directories to create.
 * @return std::expected<void, std::string> The directory path if it exists or was just created
 * , or error message.
 */
[[nodiscard]] inline Expected<fs::path> create_directories(const fs::path& p)
{
    std::error_code ec;
    fs::create_directories(p, ec);
    if (ec)
    {
      return std::unexpected(ec.message());
    }
    return p;
}

/**
 * @brief Obtains the target of a symbolic link.
 * @param p The path to the symbolic link.
 * @return std::expected<fs::path, std::string> The target path or error message.
 */
[[nodiscard]] inline Expected<fs::path> read_symlink(const fs::path& p)
{
    std::error_code ec;
    fs::path result = fs::read_symlink(p, ec);
    if (ec) {
        return std::unexpected(ec.message());
    }
    return result;
}

/**
 * @brief Modifies file access permissions.
 * @param p The path to the file or directory.
 * @param prms The permissions to set.
 * @param opts Options controlling the permission modification.
 * @return std::expected<void, std::string> Success or error message.
 */
[[nodiscard]] inline Expected<void> permissions(
    const fs::path& p,
    fs::perms prms,
    fs::perm_options opts = fs::perm_options::replace)
{
    std::error_code ec;
    fs::permissions(p, prms, opts, ec);
    if (ec) {
        return std::unexpected(ec.message());
    }
    return {};
}

/**
 * @brief Creates a symbolic link to a file.
 * @param target The path that the symbolic link will point to.
 * @param link The path where the symbolic link will be created.
 * @return std::expected<void, std::string> Success or error message.
 */
[[nodiscard]] inline Expected<void> create_symlink(
    const fs::path& target,
    const fs::path& link)
{
    std::error_code ec;
    fs::create_symlink(target, link, ec);
    if (ec) {
        return std::unexpected(ec.message());
    }
    return {};
}

/**
 * @brief Returns the size of a file.
 * @param p The path to the file.
 * @return std::expected<std::uintmax_t, std::string> The file size in bytes or error message.
 */
[[nodiscard]] inline Expected<uintmax_t> file_size(fs::path const& p)
{
  std::error_code ec;
  std::uintmax_t result = fs::file_size(p, ec);
  if (ec)
  {
    return std::unexpected(ec.message());
  }
  return result;
}

/**
 * @brief Returns the size of a file.
 * @param p The path to the file.
 * @return std::expected<std::uintmax_t, std::string> The file size in bytes or error message.
 */
[[nodiscard]] inline Expected<bool> copy_file(fs::path const& from, fs::path const& to, copy_options options)
{
  std::error_code ec;
  bool result = fs::copy_file(from, to, options, ec);
  if(ec)
  {
    std::unexpected(ec.message());
  }
  return result;
}

} // namespace: ns_fs
