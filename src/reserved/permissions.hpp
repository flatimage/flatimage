///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : permissions
///

#pragma once

#include <cstring>

#include "../lib/match.hpp"
#include "reserved.hpp"

namespace ns_reserved::ns_permissions
{

namespace
{

namespace fs = std::filesystem;

}

struct Bits
{
  uint64_t home        : 1 = 0;
  uint64_t media       : 1 = 0;
  uint64_t audio       : 1 = 0;
  uint64_t wayland     : 1 = 0;
  uint64_t xorg        : 1 = 0;
  uint64_t dbus_user   : 1 = 0;
  uint64_t dbus_system : 1 = 0;
  uint64_t udev        : 1 = 0;
  uint64_t usb         : 1 = 0;
  uint64_t input       : 1 = 0;
  uint64_t gpu         : 1 = 0;
  uint64_t network     : 1 = 0;
  Bits() noexcept = default;
  [[nodiscard]] Expected<void> set(std::string permission, bool value) noexcept
  {
    std::ranges::transform(permission, permission.begin(), ::tolower);
    return ns_match::match(permission
      , ns_match::equal("home")        >>= [&,this]{ home = value; }
      , ns_match::equal("media")       >>= [&,this]{ media = value; }
      , ns_match::equal("audio")       >>= [&,this]{ audio = value; }
      , ns_match::equal("wayland")     >>= [&,this]{ wayland = value; }
      , ns_match::equal("xorg")        >>= [&,this]{ xorg = value; }
      , ns_match::equal("dbus_user")   >>= [&,this]{ dbus_user = value; }
      , ns_match::equal("dbus_system") >>= [&,this]{ dbus_system = value; }
      , ns_match::equal("udev")        >>= [&,this]{ udev = value; }
      , ns_match::equal("usb")         >>= [&,this]{ usb = value; }
      , ns_match::equal("input")       >>= [&,this]{ input = value; }
      , ns_match::equal("gpu")         >>= [&,this]{ gpu = value; }
      , ns_match::equal("network")     >>= [&,this]{ network = value; }
    );
  } // set
  [[nodiscard]] std::vector<std::string> to_vector_string() noexcept
  {
    std::vector<std::string> out;
    if ( home )        { out.push_back("home"); }
    if ( media )       { out.push_back("media"); }
    if ( audio )       { out.push_back("audio"); }
    if ( wayland )     { out.push_back("wayland"); }
    if ( xorg )        { out.push_back("xorg"); }
    if ( dbus_user )   { out.push_back("dbus_user"); }
    if ( dbus_system ) { out.push_back("dbus_system"); }
    if ( udev )        { out.push_back("udev"); }
    if ( usb )         { out.push_back("usb"); }
    if ( input )       { out.push_back("input"); }
    if ( gpu )         { out.push_back("gpu"); }
    if ( network )     { out.push_back("network"); }
    return out;
  }
};

/**
 * @brief Write the Bits struct to the given binary
 * 
 * @param path_file_binary Binary in which to write the Bits struct
 * @param bits The bits struct to write into the binary
 * @return void on success, or the respective error
 */
inline Expected<void> write(fs::path const& path_file_binary, Bits const& bits) noexcept
{
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
inline Expected<Bits> read(fs::path const& path_file_binary) noexcept
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_PERMISSIONS_BEGIN;
  uint64_t size = ns_reserved::FIM_RESERVED_OFFSET_PERMISSIONS_END - offset_begin;
  constexpr size_t const size_bits = sizeof(Bits);
  char buffer[size_bits];
  qreturn_if(size_bits != size, Unexpected("Trying to read an exceeding number of bytes: {} vs {}"_fmt(size_bits, size)));
  Expect(ns_reserved::read(path_file_binary, offset_begin, buffer, size_bits));
  Bits bits;
  std::memcpy(&bits, buffer, sizeof(bits));
  return bits;
}

class Permissions
{
  private:
    fs::path m_path_file_binary;
  public:
    Permissions() = default;
    Permissions(fs::path const& path_file_binary) : m_path_file_binary(path_file_binary) {}
    template<ns_concept::Iterable R>
    [[nodiscard]] inline Expected<void> set(R&& r)
    {
      Bits bits;
      for(auto&& e : r) { Expect(bits.set(e, true)); };
      Expect(write(m_path_file_binary, bits));
      return {};
    }

    template<ns_concept::Iterable R>
    [[nodiscard]] inline Expected<void> add(R&& r)
    {
      Bits bits = Expect(read(m_path_file_binary));
      for(auto&& e : r) { Expect(bits.set(e, true)); };
      Expect(write(m_path_file_binary, bits));
      return {};
    }

    template<ns_concept::Iterable R>
    [[nodiscard]] inline Expected<void> del(R&& r)
    {
      Bits bits = Expect(read(m_path_file_binary));
      for(auto&& e : r) { Expect(bits.set(e, false)); };
      Expect(write(m_path_file_binary, bits));
      return {};
    }

    [[nodiscard]] inline Expected<Bits> get() noexcept
    {
      return read(m_path_file_binary);
    }

    [[nodiscard]] inline std::vector<std::string> to_vector_string() noexcept
    {
      std::vector<std::string> out;
      auto expected = read(m_path_file_binary);
      ereturn_if(not expected, "Failed to read permissions: {}"_fmt(expected.error()), out);
      return expected->to_vector_string();
    }
};

} // namespace ns_reserved::ns_permissions

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
