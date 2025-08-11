///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : reserved
///

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

// Permissions
uint64_t const FIM_RESERVED_OFFSET_PERMISSIONS_BEGIN = FIM_RESERVED_OFFSET;
uint64_t const FIM_RESERVED_OFFSET_PERMISSIONS_END = FIM_RESERVED_OFFSET_PERMISSIONS_BEGIN + 8;
// Notify
uint64_t const FIM_RESERVED_OFFSET_NOTIFY_BEGIN = FIM_RESERVED_OFFSET_PERMISSIONS_END;
uint64_t const FIM_RESERVED_OFFSET_NOTIFY_END = FIM_RESERVED_OFFSET_NOTIFY_BEGIN + 1; 
// Desktop
uint64_t const FIM_RESERVED_OFFSET_DESKTOP_BEGIN = FIM_RESERVED_OFFSET_NOTIFY_END;
uint64_t const FIM_RESERVED_OFFSET_DESKTOP_END = FIM_RESERVED_OFFSET_DESKTOP_BEGIN + (4*(1<<10)); 
// Icon
uint64_t const FIM_RESERVED_OFFSET_ICON_BEGIN = FIM_RESERVED_OFFSET_DESKTOP_END;
uint64_t const FIM_RESERVED_OFFSET_ICON_END = FIM_RESERVED_OFFSET_DESKTOP_BEGIN + (1<<20);

namespace
{
namespace fs = std::filesystem;
}


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
} // write() }}}

/**
 * @brief Reads data from a file in binary format
 * 
 * @param path_file_binary The binary file to read
 * @param offset The starting offset in bytes where to starting writing
 * @param data The data to write into the file
 * @param length The length of the data to write into the file
 * @return The number of bytes read
 */
[[nodiscard]] inline Expected<uint64_t> read(fs::path const& path_file_binary
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
} // read() }}}

} // namespace ns_reserved

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
