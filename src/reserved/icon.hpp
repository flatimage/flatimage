/**
 * @file icon.hpp
 * @author Ruan Formigoni
 * @brief Manages the icon reserved space
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <algorithm>
#include <filesystem>

#include "../std/expected.hpp"
#include "../macro.hpp"
#include "reserved.hpp"

/**
 * @namespace ns_reserved::ns_icon
 * @brief Application icon image storage in reserved space
 *
 * This namespace manages embedded icon image data stored in the binary's reserved space.
 * It stores icons in PNG or JPEG format along with the file extension
 * and size metadata. The embedded icon is used for desktop integration, appearing in
 * application launchers, taskbars, and .desktop files. Icons are processed through
 * ImageMagick for resizing as needed.
 */
namespace ns_reserved::ns_icon
{

namespace
{

namespace fs = std::filesystem;

}

#pragma pack(push, 1)
/**
 * @brief Stores icon data in reserved space
 *
 * Packed structure for embedding icon image data within the FlatImage binary.
 */
struct Icon
{
  char m_ext[4];
  char m_data[(1<<20) - 12];
  uint64_t m_size;

  /**
   * @brief Default constructor, initializes to empty icon
   */
  Icon()
    : m_size(0)
  {
    std::fill_n(m_ext, std::size(m_ext), 0);
    std::fill_n(m_data, std::size(m_data), 0);
  }

  /**
   * @brief Constructs an icon with the specified data
   * @param ext File extension (3 characters + null terminator)
   * @param data Icon image data
   * @param size Size of the icon data in bytes
   */
  Icon(char* ext, char* data, uint64_t size)
    : m_size(size)
  {
    // xxx + '\0'
    std::copy(ext, ext+3, m_ext);
    m_ext[3] = '\0';
    std::copy(data, data+size, m_data);
  } // Icon
};
#pragma pack(pop)

/**
 * @brief Writes the Icon struct to the target binary
 *
 * @param path_file_binary Target binary to write the struct to
 * @param icon Struct to write to the target file as binary data
 * @return Value<void> Nothing on success, or the respective error message
 */
inline Value<void> write(fs::path const& path_file_binary, Icon const& icon)
{
  uint64_t space_available = ns_reserved::FIM_RESERVED_OFFSET_ICON_END - ns_reserved::FIM_RESERVED_OFFSET_ICON_BEGIN;
  uint64_t space_required = sizeof(Icon);
  return_if(space_available < space_required, Error("E::Not enough space to fit icon data: {} vs {}", space_available, space_required));
  Pop(ns_reserved::write(path_file_binary
    , ns_reserved::FIM_RESERVED_OFFSET_ICON_BEGIN
    , ns_reserved::FIM_RESERVED_OFFSET_ICON_END
    , reinterpret_cast<char const*>(&icon)
    , sizeof(icon)
  ));
  return {};
}

/**
 * @brief Reads the Icon struct from the target binary
 *
 * @param path_file_binary Target binary to write the struct from
 * @return On success it returns the read Icon struct, or the respective error message
 */
inline Value<Icon> read(fs::path const& path_file_binary)
{
  Icon icon;
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_ICON_BEGIN;
  uint64_t size = ns_reserved::FIM_RESERVED_OFFSET_ICON_END - offset_begin;
  Pop(ns_reserved::read(path_file_binary, offset_begin, reinterpret_cast<char*>(&icon), size));
  return icon;
}

} // namespace ns_reserved::ns_icon

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
