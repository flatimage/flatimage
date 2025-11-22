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

#include "../std/expected.hpp"
#include "../macro.hpp"
#include "reserved.hpp"

/**
 * @namespace ns_reserved::ns_casefold
 * @brief Case-insensitive filesystem flag management in reserved space
 *
 * This namespace manages a single-byte flag in the binary's reserved space that controls
 * whether the case-insensitive overlay (CIOPFS) is enabled. When enabled, CIOPFS provides
 * Windows-style case-insensitive filesystem semantics, which is essential for Wine/Proton
 * compatibility. Note that CIOPFS is not case-preserving and stores filenames in lowercase.
 */
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
 * @return Value<void> Success or error 
 */
inline Value<void> write(fs::path const& path_file_binary, uint8_t is_casefold)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_CASEFOLD_BEGIN;
  uint64_t offset_end = ns_reserved::FIM_RESERVED_OFFSET_CASEFOLD_END;
  uint64_t size = offset_end - offset_begin;
  return_if(size != sizeof(uint8_t)
    , Error("E::Incorrect number of bytes to write notification flag: {} vs {}", size, sizeof(uint8_t))
  );
  return ns_reserved::write(path_file_binary, offset_begin, offset_end, reinterpret_cast<char*>(&is_casefold), sizeof(uint8_t));
}

/**
 * @brief Read a notification flag from the flatimage binary
 * 
 * @param path_file_binary Path to the flatimage binary
 * @return On success, if casefold is enabled or not. Or the respective error.
 */
inline Value<uint8_t> read(fs::path const& path_file_binary)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_CASEFOLD_BEGIN;
  uint8_t is_casefold;
  ssize_t bytes = Pop(ns_reserved::read(path_file_binary, offset_begin, reinterpret_cast<char*>(&is_casefold), sizeof(uint8_t)));
  return_if(bytes != 1, Error("E::Error to read notify byte, count is {}", bytes));
  return is_casefold;
}

} // namespace ns_reserved::ns_casefold

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
