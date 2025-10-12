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

#include "../macro.hpp"

namespace ns_reserved
{

extern "C" uint32_t FIM_RESERVED_OFFSET;

namespace
{
namespace fs = std::filesystem;

// Check if there is enough reserved space at compile-time
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
  constexpr Reserved()
  {
    static_assert(fim_reserved_offset_icon_end < FIM_RESERVED_SIZE, "Insufficient reserved space");
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
[[nodiscard]] inline Expected<void> write(fs::path const& path_file_binary
  , uint64_t offset_begin
  , uint64_t offset_end
  , const char* data
  , uint64_t length) noexcept
{
  uint64_t size = offset_end - offset_begin;
  qreturn_if(length > size, Unexpected("Size of data exceeds available space"));
  // Open output binary file
  std::fstream file_binary(path_file_binary, std::ios::binary | std::ios::in | std::ios::out);
  qreturn_if(not file_binary.is_open(), Unexpected("Failed to open input file"));
  // Write blank data
  std::vector<char> blank(size, 0);
  qreturn_if(not file_binary.seekp(offset_begin), Unexpected("Failed to seek offset to blank"));
  qreturn_if(not file_binary.write(blank.data(), size), Unexpected("Failed to write blank data"));
  // Write data with length
  qreturn_if(not file_binary.seekp(offset_begin), Unexpected("Failed to seek offset to write data"));
  qreturn_if(not file_binary.write(data, length), Unexpected("Failed to write data"));
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
[[nodiscard]] inline Expected<std::streamsize> read(fs::path const& path_file_binary
  , uint64_t offset
  , char* data
  , uint64_t length) noexcept
{
  // Open binary file
  std::ifstream file_binary(path_file_binary, std::ios::binary | std::ios::in);
  qreturn_if(not file_binary.is_open(), Unexpected("Failed to open input file"));
  // Advance towards data
  qreturn_if(not file_binary.seekg(offset), Unexpected("Failed to seek to file offset for read"));
  // Read data
  qreturn_if(not file_binary.read(data, length), Unexpected("Failed to read data from binary file"));
  // Return number of read bytes
  return file_binary.gcount();
}

} // namespace ns_reserved

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
