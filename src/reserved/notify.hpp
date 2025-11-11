/**
 * @file notify.hpp
 * @author Ruan Formigoni
 * @brief Manages the notify reserved space
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdint>
#include <filesystem>

#include "../std/expected.hpp"
#include "../macro.hpp"
#include "reserved.hpp"

namespace ns_reserved::ns_notify
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Writes a notification flag to the flatimage binary
 * 
 * @param path_file_binary Path to the flatimage binary
 * @param is_notify Whether notifications should be enabled or not
 * @return Value<void> 
 */
inline Value<void> write(fs::path const& path_file_binary, uint8_t is_notify)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_NOTIFY_BEGIN;
  uint64_t offset_end = ns_reserved::FIM_RESERVED_OFFSET_NOTIFY_END;
  uint64_t size = offset_end - offset_begin;
  return_if(size != sizeof(uint8_t), Error("E::Incorrect number of bytes to write notification flag: {} vs {}", size, sizeof(uint8_t)));
  return ns_reserved::write(path_file_binary, offset_begin, offset_end, reinterpret_cast<char*>(&is_notify), sizeof(uint8_t));
}

/**
 * @brief Read a notification flag from the flatimage binary
 * 
 * @param path_file_binary Path to the flatimage binary
 * @return On success, if notifications are be enabled or not. Or the respective error.
 */
inline Value<uint8_t> read(fs::path const& path_file_binary)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_NOTIFY_BEGIN;
  uint8_t is_notify;
  ssize_t bytes = Pop(ns_reserved::read(path_file_binary, offset_begin, reinterpret_cast<char*>(&is_notify), sizeof(uint8_t)));
  log_if(bytes != 1, "E::Possible error to read notify byte, count is {}", bytes);
  return is_notify;
}

} // namespace ns_reserved::ns_notify

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
