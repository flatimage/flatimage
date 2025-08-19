///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : path
///

#pragma once

#include <cstring>
#include <filesystem>
#include <expected>
#include <linux/limits.h>
#include <ranges>

#include "../common.hpp"
#include "../macro.hpp"

namespace ns_filesystem
{

namespace fs = std::filesystem;

namespace ns_path
{
  
/**
 * @brief Check if 'path' exists, if not, tries to create all components of it
 * 
 * @param path The path to check if exists or create
 * @return Expected<fs::path> The input 'path' if it exists or was created
 */
[[nodiscard]] inline Expected<fs::path> create_if_not_exists(fs::path const& path) noexcept
{
  std::error_code ec;
  if(not fs::exists(path, ec) and not fs::create_directories(path))
  {
    return Unexpected("Could not create directory '{}'"_fmt(path));
  }
  return path;
}

// canonical() {{{
// Try to make path canonical
inline Expected<fs::path> canonical(fs::path const& path)
{
  fs::path ret{path};

  // Adjust for relative path
  if (not ret.string().starts_with("/"))
  {
    ret = fs::path{"./"} / ret;
  } // if

  // Make path cannonical
  std::error_code ec;
  ret = fs::canonical(ret, ec);
  if ( ec )
  {
    return Unexpected("Could not make cannonical path for parent of '{}'"_fmt(path));
  } // if

  return ret;
} // function: canonical }}}

// file_self() {{{
inline Expected<fs::path> file_self()
{
  std::error_code ec;

  auto path_file_self = fs::read_symlink("/proc/self/exe", ec);

  if ( ec )
  {
    return Unexpected("Failed to fetch location of self");
  } // if

  return path_file_self;
} // file_self() }}}

// realpath() {{{
inline Expected<fs::path> realpath(fs::path const& path_file_src)
{
  char str_path_file_resolved[PATH_MAX];
  if ( ::realpath(path_file_src.c_str(), str_path_file_resolved) == nullptr )
  {
    return Unexpected(strerror(errno));
  } // if
  return str_path_file_resolved;
} // realpath() }}}

// list_files() {{{
inline Expected<std::vector<fs::path>> list_files(fs::path const& path_dir_src)
{
  std::error_code ec;
  std::vector<fs::path> files = fs::directory_iterator(path_dir_src, ec)
    | std::views::transform([](auto&& e){ return e.path(); })
    | std::views::filter([](auto&& e){ return fs::is_regular_file(e); })
    | std::ranges::to<std::vector<fs::path>>();
  qreturn_if(ec, Unexpected(ec.message()));
  return files;
} // list_files() }}}

} // namespace ns_path

} // namespace ns_filesystem

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
