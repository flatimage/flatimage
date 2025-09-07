/**
 * @file casefold.hpp
 * @author Ruan Formigoni
 * @brief Manages the casefold reserved space
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdint>
#include <filesystem>

#include "../macro.hpp"
#include "reserved.hpp"

namespace ns_reserved::ns_casefold
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Writes a notification flag to the flatimage binary
 * 
 * @param path_file_binary Path to the flatimage binary
 * @param is_casefold Whether casefold should be enabled or not
 * @return Expected<void> 
 */
inline Expected<void> write(fs::path const& path_file_binary, uint8_t is_casefold)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_CASEFOLD_BEGIN;
  uint64_t offset_end = ns_reserved::FIM_RESERVED_OFFSET_CASEFOLD_END;
  uint64_t size = offset_end - offset_begin;
  qreturn_if(size != sizeof(uint8_t), Unexpected("Incorrect number of bytes to write notification flag: {} vs {}"_fmt(size, sizeof(uint8_t))));
  return ns_reserved::write(path_file_binary, offset_begin, offset_end, reinterpret_cast<char*>(&is_casefold), sizeof(uint8_t));
}

/**
 * @brief Read a notification flag from the flatimage binary
 * 
 * @param path_file_binary Path to the flatimage binary
 * @return On success, if casefold is enabled or not. Or the respective error.
 */
inline Expected<uint8_t> read(fs::path const& path_file_binary)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_CASEFOLD_BEGIN;
  uint8_t is_casefold;
  ssize_t bytes = Expect(ns_reserved::read(path_file_binary, offset_begin, reinterpret_cast<char*>(&is_casefold), sizeof(uint8_t)));
  qreturn_if(bytes != 1, Unexpected("Error to read notify byte, count is {}"_fmt(bytes)));
  return is_casefold;
}

} // namespace ns_reserved::ns_casefold

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
