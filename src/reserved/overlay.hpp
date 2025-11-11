/**
 * @file overlay.hpp
 * @author Ruan Formigoni
 * @brief Manages the overlay reserved space
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdint>
#include <filesystem>

#include "../std/expected.hpp"
#include "../macro.hpp"
#include "../std/enum.hpp"
#include "reserved.hpp"

namespace ns_reserved::ns_overlay
{

namespace
{

namespace fs = std::filesystem;

}

ENUM(OverlayType, BWRAP, OVERLAYFS, UNIONFS)

/**
 * @brief Writes a overlay mask to the flatimage binary
 * 
 * @param path_file_binary Path to the flatimage binary
 * @param overlay Overlay type enumeration
 * @return Value<void> Nothing on success, or the respective error
 */
inline Value<void> write(fs::path const& path_file_binary, OverlayType const& overlay)
{
  uint8_t mask{};
  switch(overlay)
  {
    case OverlayType::BWRAP: mask = 1 << 1; break;
    case OverlayType::OVERLAYFS: mask = 1 << 2; break;
    case OverlayType::UNIONFS: mask = 1 << 3; break;
    case OverlayType::NONE:
    default: return Error("E::Invalid overlay option");
  }
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_OVERLAY_BEGIN;
  uint64_t offset_end = ns_reserved::FIM_RESERVED_OFFSET_OVERLAY_END;
  uint64_t size = offset_end - offset_begin;
  return_if(size != sizeof(uint8_t), Error("E::Incorrect number of bytes to write overlay mask: {} vs {}", size, sizeof(uint8_t)));
  return ns_reserved::write(path_file_binary, offset_begin, offset_end, reinterpret_cast<char*>(&mask), sizeof(uint8_t));
}

/**
 * @brief Read the overlay mask from the flatimage binary
 * 
 * @param path_file_binary Path to the flatimage binary
 * @return On success, returns the type of overlay currently in use. Or the respective error.
 */
inline Value<OverlayType> read(fs::path const& path_file_binary)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_OVERLAY_BEGIN;
  uint8_t mask;
  ssize_t bytes = Pop(ns_reserved::read(path_file_binary, offset_begin, reinterpret_cast<char*>(&mask), sizeof(uint8_t)));
  log_if(bytes != 1, "E::Possible error to read overlay byte, count is {}", bytes);
  switch(mask)
  {
    case 1 << 1: return OverlayType::BWRAP;
    case 1 << 2: return OverlayType::OVERLAYFS;
    case 1 << 3: return OverlayType::UNIONFS;
    default: return std::unexpected("Invalid overlay option");
  }
}

} // namespace ns_reserved::ns_overlay

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
