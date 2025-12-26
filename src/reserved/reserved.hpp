/**
 * @file reserved.hpp
 * @author Ruan Formigoni
 * @brief Manages reserved space
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <expected>
#include <fcntl.h>
#include <filesystem>
#include <cstdint>
#include <fstream>

#include "../common.hpp"
#include "../macro.hpp"
#include "../std/expected.hpp"

/**
 * @namespace ns_reserved
 * @brief Management of ELF binary reserved space for configuration storage
 *
 * This namespace provides core functionality for reading and writing configuration data
 * to a reserved section within the FlatImage ELF binary. It defines the memory layout
 * (offsets and sizes) for all configuration components and provides low-level read/write
 * operations. The reserved space allows post-build reconfiguration without rebuilding
 * the entire binary, storing permissions, environment variables, bind mounts, desktop
 * integration data, icons, and more.
 */
namespace ns_reserved
{

extern "C" uint32_t FIM_RESERVED_OFFSET;

namespace
{
namespace fs = std::filesystem;

/**
 * @brief Defines offsets for all reserved space components
 *
 * This struct provides compile-time constants defining the layout of the
 * reserved space within the FlatImage binary. It also validates that the
 * total reserved space doesn't exceed FIM_RESERVED_SIZE.
 */
struct Reserved
{
  // Permissions
  constexpr static uint64_t const fim_reserved_offset_permissions_begin = 0;
  constexpr static uint64_t const fim_reserved_offset_permissions_end = fim_reserved_offset_permissions_begin + 8;
  // notify
  constexpr static uint64_t const fim_reserved_offset_notify_begin = fim_reserved_offset_permissions_end;
  constexpr static uint64_t const fim_reserved_offset_notify_end = fim_reserved_offset_notify_begin + 1; 
  // overlay
  constexpr static uint64_t const fim_reserved_offset_overlay_begin = fim_reserved_offset_notify_end;
  constexpr static uint64_t const fim_reserved_offset_overlay_end = fim_reserved_offset_overlay_begin + 1; 
  // casefold
  constexpr static uint64_t const fim_reserved_offset_casefold_begin = fim_reserved_offset_overlay_end;
  constexpr static uint64_t const fim_reserved_offset_casefold_end = fim_reserved_offset_casefold_begin + 1; 
  // desktop
  constexpr static uint64_t const fim_reserved_offset_desktop_begin = fim_reserved_offset_casefold_end;
  constexpr static uint64_t const fim_reserved_offset_desktop_end = fim_reserved_offset_desktop_begin + 4_kib;
  // boot
  constexpr static uint64_t const fim_reserved_offset_boot_begin = fim_reserved_offset_desktop_end;
  constexpr static uint64_t const fim_reserved_offset_boot_end = fim_reserved_offset_boot_begin + 8_kib;
  // icon
  constexpr static uint64_t const fim_reserved_offset_icon_begin = fim_reserved_offset_boot_end;
  constexpr static uint64_t const fim_reserved_offset_icon_end = fim_reserved_offset_icon_begin + 1_mib;
  // environment
  constexpr static uint64_t const fim_reserved_offset_environment_begin = fim_reserved_offset_icon_end;
  constexpr static uint64_t const fim_reserved_offset_environment_end = fim_reserved_offset_environment_begin + 1_mib;
  // bindings
  constexpr static uint64_t const fim_reserved_offset_bindings_begin = fim_reserved_offset_environment_end;
  constexpr static uint64_t const fim_reserved_offset_bindings_end = fim_reserved_offset_bindings_begin + 1_mib;
  // remote
  constexpr static uint64_t const fim_reserved_offset_remote_begin = fim_reserved_offset_bindings_end;
  constexpr static uint64_t const fim_reserved_offset_remote_end = fim_reserved_offset_remote_begin + 4_kib;
  // unshare
  constexpr static uint64_t const fim_reserved_offset_unshare_begin = fim_reserved_offset_remote_end;
  constexpr static uint64_t const fim_reserved_offset_unshare_end = fim_reserved_offset_unshare_begin + 2;

  /**
   * @brief Validates reserved space layout at compile-time
   */
  constexpr Reserved()
  {
    static_assert(fim_reserved_offset_unshare_end < FIM_RESERVED_SIZE, "Insufficient reserved space");
  }
};

constexpr Reserved reserved;
}

// Permissions
uint64_t const FIM_RESERVED_OFFSET_PERMISSIONS_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_permissions_begin;
uint64_t const FIM_RESERVED_OFFSET_PERMISSIONS_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_permissions_end;
// Notify
uint64_t const FIM_RESERVED_OFFSET_NOTIFY_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_notify_begin;
uint64_t const FIM_RESERVED_OFFSET_NOTIFY_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_notify_end;
// Overlay
uint64_t const FIM_RESERVED_OFFSET_OVERLAY_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_overlay_begin;
uint64_t const FIM_RESERVED_OFFSET_OVERLAY_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_overlay_end;
// Casefold
uint64_t const FIM_RESERVED_OFFSET_CASEFOLD_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_casefold_begin;
uint64_t const FIM_RESERVED_OFFSET_CASEFOLD_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_casefold_end;
// Desktop
uint64_t const FIM_RESERVED_OFFSET_DESKTOP_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_desktop_begin;
uint64_t const FIM_RESERVED_OFFSET_DESKTOP_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_desktop_end;
// Boot
uint64_t const FIM_RESERVED_OFFSET_BOOT_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_boot_begin;
uint64_t const FIM_RESERVED_OFFSET_BOOT_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_boot_end;
// Icon
uint64_t const FIM_RESERVED_OFFSET_ICON_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_icon_begin;
uint64_t const FIM_RESERVED_OFFSET_ICON_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_icon_end;
// Environment
uint64_t const FIM_RESERVED_OFFSET_ENVIRONMENT_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_environment_begin;
uint64_t const FIM_RESERVED_OFFSET_ENVIRONMENT_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_environment_end;
// Bindings
uint64_t const FIM_RESERVED_OFFSET_BINDINGS_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_bindings_begin;
uint64_t const FIM_RESERVED_OFFSET_BINDINGS_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_bindings_end;
// Remote
uint64_t const FIM_RESERVED_OFFSET_REMOTE_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_remote_begin;
uint64_t const FIM_RESERVED_OFFSET_REMOTE_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_remote_end;
// Unshare
uint64_t const FIM_RESERVED_OFFSET_UNSHARE_BEGIN = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_unshare_begin;
uint64_t const FIM_RESERVED_OFFSET_UNSHARE_END = FIM_RESERVED_OFFSET + reserved.fim_reserved_offset_unshare_end;



/**
 * @brief Writes data to a file in binary format
 * 
 * @param path_file_binary The binary file to modify
 * @param offset_begin The starting offset in bytes where to starting writing
 * @param offset_end The ending offset in bytes where to stop writing
 * @param data The data to write into the file
 * @param length The length of the data to write into the file
 * @return On success it returns void, otherwise it returns the number of bytes written
 */
[[nodiscard]] inline Value<void> write(fs::path const& path_file_binary
  , uint64_t offset_begin
  , uint64_t offset_end
  , const char* data
  , uint64_t length) noexcept
{
  uint64_t size = offset_end - offset_begin;
  return_if(length > size, Error("E::Size of data exceeds available space"));
  // Open output binary file
  std::fstream file_binary(path_file_binary, std::ios::binary | std::ios::in | std::ios::out);
  return_if(not file_binary.is_open(), Error("E::Failed to open input file"));
  // Write blank data
  std::vector<char> blank(size, 0);
  return_if(not file_binary.seekp(offset_begin), Error("E::Failed to seek offset to blank"));
  return_if(not file_binary.write(blank.data(), size), Error("E::Failed to write blank data"));
  // Write data with length
  return_if(not file_binary.seekp(offset_begin), Error("E::Failed to seek offset to write data"));
  return_if(not file_binary.write(data, length), Error("E::Failed to write data"));
  return {};
}

/**
 * @brief Reads data from a file in binary format
 * 
 * @param path_file_binary The binary file to read
 * @param offset The starting offset in bytes where to starting writing
 * @param data The data to write into the file
 * @param length The length of the data to write into the file
 * @return The number of bytes read
 */
[[nodiscard]] inline Value<std::streamsize> read(fs::path const& path_file_binary
  , uint64_t offset
  , char* data
  , uint64_t length) noexcept
{
  // Open binary file
  std::ifstream file_binary(path_file_binary, std::ios::binary | std::ios::in);
  return_if(not file_binary.is_open(), Error("E::Failed to open input file"));
  // Advance towards data
  return_if(not file_binary.seekg(offset), Error("E::Failed to seek to file offset for read"));
  // Read data
  return_if(not file_binary.read(data, length), Error("E::Failed to read data from binary file"));
  // Return number of read bytes
  return file_binary.gcount();
}

} // namespace ns_reserved

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
