/**
 * @file unshare.hpp
 * @author Ruan Formigoni
 * @brief Manages the unshare namespace options reserved space
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <map>
#include <cstring>
#include <set>
#include <ranges>

#include "reserved.hpp"
#include "../std/enum.hpp"
#include "../std/expected.hpp"

/**
 * @namespace ns_reserved::ns_unshare
 * @brief Unshare namespace options bitfield management in reserved space
 *
 * This namespace manages the namespace unshare options for bubblewrap.
 * It provides operations for setting, adding, removing, and
 * querying unshare options such as user, ipc, pid, net, uts, and cgroup namespaces.
 * Options default to zero (no unsharing), and must be explicitly enabled by the user.
 *
 * Note: USER and CGROUP options will use the '-try' variants in bwrap for permissiveness.
 */
namespace ns_reserved::ns_unshare
{

namespace
{

namespace fs = std::filesystem;

}

// Unshare bits (stored as uint16_t, 2 bytes)
using Bits = uint16_t;

// Unshare options as fields
ENUM(Unshare,ALL,USER,IPC,PID,NET,UTS,CGROUP);

// Corresponding bit position index
inline std::map<Unshare,Bits> const unshare_mask =
{
  {Unshare::USER, Bits{1} << 0},
  {Unshare::IPC, Bits{1} << 1},
  {Unshare::PID, Bits{1} << 2},
  {Unshare::NET, Bits{1} << 3},
  {Unshare::UTS, Bits{1} << 4},
  {Unshare::CGROUP, Bits{1} << 5},
};

/**
 * @brief Sets a bit unshare option with the target value
 *
 * @param bits Unshare bits
 * @param unshare Unshare option to change in the bits
 * @param value Value to set the target unshare option
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> bit_set(Bits& bits, Unshare const& unshare, bool value) noexcept
{
  auto it = unshare_mask.find(unshare);
  return_if(it == unshare_mask.end(), Error("E::Unshare option '{}' not found", unshare));
  Bits mask = it->second;
  if (value) { bits |= mask;  }
  else       { bits &= ~mask; }
  return {};
}

/**
 * @brief Creates a set of lowercase string unshare option representations
 *
 * @param bits Unshare bits
 * @return std::set<std::string> The string unshare option list
 */
[[nodiscard]] inline std::set<std::string> to_strings(Bits const& bits) noexcept
{
  std::set<std::string> out;
  for(auto&& [unshare,mask] : unshare_mask)
  {
    if(bits & mask)
    {
      std::string str = unshare;
      std::ranges::transform(str, str.begin(), ::tolower);
      out.insert(str);
    }
  }
  return out;
}

/**
 * @brief Write the Bits struct to the given binary
 *
 * @param path_file_binary Binary in which to write the Bits struct
 * @param bits The bits struct to write into the binary
 * @return Value<void> Nothing on success, or the respective error
 */
inline Value<void> write(fs::path const& path_file_binary, Bits const& bits) noexcept
{
  static_assert(sizeof(Bits) == sizeof(uint16_t), "Bits size must be 2 bytes");
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_UNSHARE_BEGIN;
  uint64_t offset_end = ns_reserved::FIM_RESERVED_OFFSET_UNSHARE_END;
  return ns_reserved::write(path_file_binary, offset_begin, offset_end, reinterpret_cast<char const*>(&bits), sizeof(bits));
}

/**
 * @brief Read the Bits struct from the given binary
 *
 * @param path_file_binary Binary which to read the Bits struct from
 * @return The Bits struct on success, or the respective error
 */
inline Value<Bits> read(fs::path const& path_file_binary) noexcept
{
  static_assert(sizeof(Bits) == sizeof(uint16_t), "Bits size must be 2 bytes");
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_UNSHARE_BEGIN;
  uint64_t size = ns_reserved::FIM_RESERVED_OFFSET_UNSHARE_END - offset_begin;
  constexpr size_t const size_bits = sizeof(Bits);
  return_if(size_bits != size, Error("E::Trying to read an exceeding number of bytes: {} vs {}", size_bits, size));
  Bits bits;
  Pop(ns_reserved::read(path_file_binary, offset_begin, reinterpret_cast<char*>(&bits), size_bits));
  return bits;
}

/**
 * @brief Manages FlatImage unshare options stored in reserved space
 */
class Unshares
{
  private:
    fs::path m_path_file_binary;

    Value<void> set_unshares(Bits bits, std::set<Unshare> const& unshares, bool value)
    {
      if(unshares.contains(Unshare::NONE))
      {
        return Error("E::Invalid unshare option 'NONE'");
      }
      if(unshares.contains(Unshare::ALL))
      {
        return_if(unshares.size() > 1, Error("E::Unshare option 'all' should not be used with others"));
        return this->set_all(true);
      }
      for(Unshare const& unshare : unshares)
      {
        Pop(bit_set(bits, unshare, value));
      };
      Pop(write(m_path_file_binary, bits));
      return {};
    }
  public:
    /**
     * @brief Constructs an Unshares manager for the given binary
     * @param path_file_binary Path to the FlatImage binary file
     */
    Unshares(fs::path const& path_file_binary)
      : m_path_file_binary(path_file_binary)
    {}

    /**
     * @brief Sets all unshare options to the specified value
     * @param value True to enable all unshare options, false to disable
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> set_all(bool value)
    {
      auto unshares = unshare_mask
        | std::views::transform([](auto&& e){ return e.first; })
        | std::views::filter([](auto&& e){ return e != Unshare::ALL; })
        | std::ranges::to<std::set<Unshare>>();
      return set_unshares(Bits{}, unshares, value);
    }

    /**
     * @brief Sets the specified unshare options (replaces existing)
     * @param unshares Set of unshare options to enable
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> set(std::set<Unshare> const& unshares)
    {
      return set_unshares(Bits{}, unshares, true);
    }

    /**
     * @brief Adds unshare options to existing configuration
     * @param unshares Set of unshare options to add
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> add(std::set<Unshare> const& unshares)
    {
      return set_unshares(Pop(read(m_path_file_binary)), unshares, true);
    }

    /**
     * @brief Removes unshare options from existing configuration
     * @param unshares Set of unshare options to remove
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> del(std::set<Unshare> const& unshares)
    {
      return set_unshares(Pop(read(m_path_file_binary)), unshares, false);
    }

    /**
     * @brief Checks if a specific unshare option is enabled
     * @param unshare Unshare option to check
     * @return True if the unshare option is enabled, false otherwise
     */
    [[nodiscard]] inline bool contains(Unshare const& unshare) const noexcept
    {
      return_if(unshare == Unshare::NONE or unshare == Unshare::ALL, false);
      Bits bits = read(m_path_file_binary).value_or(0);
      auto it = unshare_mask.find(unshare);
      return_if(it == unshare_mask.end(), false);
      return (bits & it->second) != 0;
    }

    /**
     * @brief Clears all unshare options
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> clear() noexcept
    {
      return write(m_path_file_binary, Bits{});
    }

    /**
     * @brief Converts enabled unshare options to string representations
     * @return Value containing a set of unshare option names
     */
    [[nodiscard]] inline Value<std::set<std::string>> to_strings() const noexcept
    {
      return ::ns_reserved::ns_unshare::to_strings(Pop(read(m_path_file_binary)));
    }
};

} // namespace ns_reserved::ns_unshare

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
