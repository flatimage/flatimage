/**
 * @file bind.hpp
 * @author Ruan Formigoni
 * @brief Interface with the bindings database to (de-)serialize objects
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <algorithm>

#include "../../db/db.hpp"
#include "../../db/bind.hpp"
#include "../../reserved/bind.hpp"

namespace ns_cmd::ns_bind
{

namespace
{

namespace fs = std::filesystem;

/**
 * @brief Get the highest index object
 * 
 * @param db The database to query
 * @return index_t The highest index element or -1 or error
 */
[[nodiscard]] inline uint64_t get_highest_index(ns_db::ns_bind::Binds const& binds)
{
  auto it = std::ranges::max_element(binds.get(), {}, [](ns_db::ns_bind::Bind const& bind)
  {
    return bind.index;
  });
  return (it == std::ranges::end(binds.get()))? -1 : it->index;
}

} // namespace

/**
 * @brief Reads data from the reserved space
 * 
 * @param path_file_binary Path to the flatimage binary
 * @return Value<ns_db::ns_bind::Binds> The structured data or the respective error
 */
inline Value<ns_db::ns_bind::Binds> db_read(fs::path const& path_file_binary)
{
  auto reserved_data = Pop(ns_reserved::ns_bind::read(path_file_binary));
  return ns_db::ns_bind::deserialize(
    reserved_data.empty()? "{}" : reserved_data
  ).value_or(ns_db::ns_bind::Binds{});
}

/**
 * @brief Writes data to the reserved space
 * 
 * @param path_file_binary Path to the flatimage binary
 * @return Value<void> Nothing on success, or the respective error
 */
inline Value<void> db_write(fs::path const& path_file_binary, ns_db::ns_bind::Binds const& binds)
{
  Pop(ns_reserved::ns_bind::write(path_file_binary
    , Pop(Pop(ns_db::ns_bind::serialize(binds)).dump())
  ));
  return {};
}

/**
 * @brief Adds a binding instruction to the binding database
 * 
 * @param path_file_binary Path to the flatimage binary
 * @param cmd Binding instructions
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> add(fs::path const& path_file_binary
  , ns_db::ns_bind::Type bind_type
  , fs::path const& path_src
  , fs::path const& path_dst
)
{
  // Deserialize bindings
  ns_db::ns_bind::Binds binds = Pop(db_read(path_file_binary));
  // Find out the highest bind index
  ns_db::ns_bind::Bind bind
  {
    .index = get_highest_index(binds) + 1,
    .path_src = path_src,
    .path_dst = path_dst,
    .type = bind_type,
  };
  binds.push_back(bind);
  ns_log::info()("Binding index is '{}'", bind.index);
  // Write database
  Pop(db_write(path_file_binary, binds));
  return {};
}

/**
 * @brief Deletes a binding from the database
 * 
 * @param path_file_binary Path to the flatimage binary
 * @param index Index of the binding to erase
 * @return Value<void> Returns nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> del(fs::path const& path_file_binary, uint64_t index)
{
  std::error_code ec;
  // Deserialize bindings
  ns_db::ns_bind::Binds binds = Pop(db_read(path_file_binary));
  // Erase index if exists
  binds.erase(index);
  // Write database
  Pop(db_write(path_file_binary, binds));
  return {};
}

/**
 * @brief List bindings from the given bindings database
 * 
 * @param path_file_binary Path to the flatimage binary
 */
[[nodiscard]] inline Value<void> list(fs::path const& path_file_binary)
{
  // Deserialize bindings
  ns_db::ns_bind::Binds binds = Pop(db_read(path_file_binary));
  if(not binds.empty())
  {
    std::println("{}", Pop(Pop(ns_db::ns_bind::serialize(binds)).dump()));
  }
  return {};
}

} // namespace ns_cmd::ns_bind

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
