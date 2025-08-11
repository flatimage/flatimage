///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : desktop
///

#pragma once

#include <string>
#include <filesystem>

#include "../../macro.hpp"
#include "reserved.hpp"

namespace ns_reserved::ns_desktop
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Writes the desktop json string to the target binary
 * 
 * @param path_file_binary Target binary to write the json string
 * @param json Json string to write to the target file as binary data
 * @return On success void, or the respective error message
 */
inline std::expected<void,std::string> write(fs::path const& path_file_binary, std::string_view const& json)
{
  uint64_t space_available = ns_reserved::FIM_RESERVED_OFFSET_DESKTOP_END - ns_reserved::FIM_RESERVED_OFFSET_DESKTOP_BEGIN;
  uint64_t space_required = json.size();
  qreturn_if(space_available <= space_required, std::unexpected("Not enough space to fit json data"));
  expect(ns_reserved::write(path_file_binary
    , FIM_RESERVED_OFFSET_DESKTOP_BEGIN
    , FIM_RESERVED_OFFSET_DESKTOP_END
    , json.data()
    , json.size()
  ));
  return {};
}

/**
 * @brief Reads the desktop json string from the target binary
 * 
 * @param path_file_binary Target binary to write the json string
 * @return On success it returns the read data, or the respective error message
 */
inline std::expected<std::string,std::string> read(fs::path const& path_file_binary)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_DESKTOP_BEGIN;
  uint64_t size = ns_reserved::FIM_RESERVED_OFFSET_DESKTOP_END - offset_begin;
  auto buffer = std::make_unique<char[]>(size);
  expect(ns_reserved::read(path_file_binary, offset_begin, buffer.get(), size));
  return buffer.get();
}

} // namespace ns_reserved::ns_desktop

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
