/**
 * @file remote.hpp
 * @author Ruan Formigoni
 * @brief Manages the remote URL reserved space
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <filesystem>

#include "../std/expected.hpp"
#include "../macro.hpp"
#include "reserved.hpp"

/**
 * @namespace ns_reserved::ns_remote
 * @brief Remote recipe repository URL storage in reserved space
 *
 * This namespace manages the remote recipe repository URL. The URL points to a
 * repository of package recipes that can be installed into the FlatImage to add
 * additional dependencies, libraries, or applications.  This enables dynamic
 * package installation without rebuilding the base image, supporting
 * distribution-specific package managers (apk for Alpine, pacman for Arch).
 */
namespace ns_reserved::ns_remote
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Writes the remote URL string to the target binary
 *
 * @param path_file_binary Target binary to write the URL string
 * @param url URL string to write to the target file as binary data
 * @return Value<void> Nothing on success, or the respective error message
 */
inline Value<void> write(fs::path const& path_file_binary, std::string_view const& url)
{
  uint64_t space_available = ns_reserved::FIM_RESERVED_OFFSET_REMOTE_END - ns_reserved::FIM_RESERVED_OFFSET_REMOTE_BEGIN;
  uint64_t space_required = url.size();
  return_if(space_available <= space_required, Error("E::Not enough space to fit remote URL"));
  Pop(ns_reserved::write(path_file_binary
    , FIM_RESERVED_OFFSET_REMOTE_BEGIN
    , FIM_RESERVED_OFFSET_REMOTE_END
    , url.data()
    , url.size()
  ));
  return {};
}

/**
 * @brief Reads the remote URL string from the target binary
 *
 * @param path_file_binary Target binary to read the URL string
 * @return On success it returns the read data, or the respective error message
 */
inline Value<std::string> read(fs::path const& path_file_binary)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_REMOTE_BEGIN;
  uint64_t size = ns_reserved::FIM_RESERVED_OFFSET_REMOTE_END - offset_begin;
  auto buffer = std::make_unique<char[]>(size);
  Pop(ns_reserved::read(path_file_binary, offset_begin, buffer.get(), size));
  return buffer.get();
}

} // namespace ns_reserved::ns_remote

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
