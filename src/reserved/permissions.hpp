/**
 * @file permissions.hpp
 * @author Ruan Formigoni
 * @brief Manages the permissions reserved space
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

namespace ns_reserved::ns_permissions
{

namespace
{

namespace fs = std::filesystem;

}

// Permission bits
using Bits = uint64_t;

// Permissions as fields
ENUM(Permission,ALL,HOME,MEDIA,AUDIO,WAYLAND,XORG,DBUS_USER,DBUS_SYSTEM,UDEV,USB,INPUT,GPU,NETWORK,DEV,SHM,OPTICAL);

// Corresponding bit position index
inline std::map<Permission,Bits> const permission_mask =
{
  {Permission::HOME, Bits{1} << 0},
  {Permission::MEDIA, Bits{1} << 1},
  {Permission::AUDIO, Bits{1} << 2},
  {Permission::WAYLAND, Bits{1} << 3},
  {Permission::XORG, Bits{1} << 4},
  {Permission::DBUS_USER, Bits{1} << 5},
  {Permission::DBUS_SYSTEM, Bits{1} << 6},
  {Permission::UDEV, Bits{1} << 7},
  {Permission::USB, Bits{1} << 8},
  {Permission::INPUT, Bits{1} << 9},
  {Permission::GPU, Bits{1} << 10},
  {Permission::NETWORK, Bits{1} << 11},
  {Permission::DEV, Bits{1} << 12},
  {Permission::SHM, Bits{1} << 13},
  {Permission::OPTICAL, Bits{1} << 14},
};

/**
 * @brief Sets a bit permission with the target value
 * 
 * @param bits Permission bits
 * @param permission Permission to change in the bits
 * @param value Value to set the target permission
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> bit_set(Bits& bits, Permission const& permission, bool value) noexcept
{
  auto it = permission_mask.find(permission);
  return_if(it == permission_mask.end(), Error("E::Permission '{}' not found", permission));
  Bits mask = it->second;
  if (value) { bits |= mask;  }
  else       { bits &= ~mask; }
  return {};
}

/**
 * @brief Creates a set of lowercase string permission representations
 * 
 * @param bits Permission bits
 * @return std::set<std::string> The string permission list
 */
[[nodiscard]] inline std::set<std::string> to_strings(Bits const& bits) noexcept
{
  std::set<std::string> out;
  for(auto&& [permission,mask] : permission_mask)
  {
    if(bits & mask)
    {
      std::string str = permission;
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
  static_assert(sizeof(Bits) == sizeof(uint64_t), "Bits size must be 8 bytes");
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_PERMISSIONS_BEGIN;
  uint64_t offset_end = ns_reserved::FIM_RESERVED_OFFSET_PERMISSIONS_END;
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
  static_assert(sizeof(Bits) == sizeof(uint64_t), "Bits size must be 8 bytes");
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_PERMISSIONS_BEGIN;
  uint64_t size = ns_reserved::FIM_RESERVED_OFFSET_PERMISSIONS_END - offset_begin;
  constexpr size_t const size_bits = sizeof(Bits);
  return_if(size_bits != size, Error("E::Trying to read an exceeding number of bytes: {} vs {}", size_bits, size));
  Bits bits;
  Pop(ns_reserved::read(path_file_binary, offset_begin, reinterpret_cast<char*>(&bits), size_bits));
  return bits;
}

/**
 * @brief Manages FlatImage permissions stored in reserved space
 */
class Permissions
{
  private:
    fs::path m_path_file_binary;

    Value<void> set_permissions(Bits bits, std::set<Permission> const& permissions, bool value)
    {
      if(permissions.contains(Permission::NONE))
      {
        return Error("E::Invalid permission 'NONE'");
      }
      if(permissions.contains(Permission::ALL))
      {
        return_if(permissions.size() > 1, Error("E::Permission 'all' should not be used with others"));
        return this->set_all(true);
      }
      for(Permission const& permission : permissions)
      {
        Pop(bit_set(bits, permission, value));
      };
      Pop(write(m_path_file_binary, bits));
      return {};
    }
  public:
    /**
     * @brief Constructs a Permissions manager for the given binary
     * @param path_file_binary Path to the FlatImage binary file
     */
    Permissions(fs::path const& path_file_binary)
      : m_path_file_binary(path_file_binary)
    {}

    /**
     * @brief Sets all permissions to the specified value
     * @param value True to enable all permissions, false to disable
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> set_all(bool value)
    {
      auto permissions = permission_mask
        | std::views::transform([](auto&& e){ return e.first; })
        | std::views::filter([](auto&& e){ return e != Permission::ALL; })
        | std::ranges::to<std::set<Permission>>();
      return set_permissions(Bits{}, permissions, value);
    }

    /**
     * @brief Sets the specified permissions (replaces existing)
     * @param permissions Set of permissions to enable
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> set(std::set<Permission> const& permissions)
    {
      return set_permissions(Bits{}, permissions, true);
    }

    /**
     * @brief Adds permissions to existing configuration
     * @param permissions Set of permissions to add
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> add(std::set<Permission> const& permissions)
    {
      return set_permissions(Pop(read(m_path_file_binary)), permissions, true);
    }

    /**
     * @brief Removes permissions from existing configuration
     * @param permissions Set of permissions to remove
     * @return Value<void> Success or error
     */
    [[nodiscard]] inline Value<void> del(std::set<Permission> const& permissions)
    {
      return set_permissions(Pop(read(m_path_file_binary)), permissions, false);
    }

    /**
     * @brief Checks if a specific permission is enabled
     * @param permission Permission to check
     * @return True if the permission is enabled, false otherwise
     */
    [[nodiscard]] inline bool contains(Permission const& permission) const noexcept
    {
      return_if(permission == Permission::NONE or permission == Permission::ALL, false);
      Bits bits = read(m_path_file_binary).value_or(0);
      auto it = permission_mask.find(permission);
      return_if(it == permission_mask.end(), false);
      return (bits & it->second) != 0;
    }

    /**
     * @brief Converts enabled permissions to string representations
     * @return Value containing a set of permission names
     */
    [[nodiscard]] inline Value<std::set<std::string>> to_strings() const noexcept
    {
      return ::ns_reserved::ns_permissions::to_strings(Pop(read(m_path_file_binary)));
    }
};

} // namespace ns_reserved::ns_permissions

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
