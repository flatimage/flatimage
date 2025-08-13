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

#include "../interface.hpp"
#include "../../db/db.hpp"
#include "../../db/bind.hpp"

namespace ns_cmd::ns_bind
{

using CmdBind = ns_parser::ns_interface::CmdBind;
using CmdBindType = ns_parser::ns_interface::CmdBindType;
using index_t = ns_parser::ns_interface::CmdBind::cmd_bind_index_t;
using bind_t = ns_parser::ns_interface::CmdBind::cmd_bind_t;

namespace
{

namespace fs = std::filesystem;

/**
 * @brief Get the highest index object
 * 
 * @param db The database to query
 * @return index_t The highest index element or -1 or error
 */
[[nodiscard]] inline index_t get_highest_index(ns_db::Db const& db)
{
  auto keys = db.keys();
  auto bindings = keys
    | std::views::filter([](auto&& e){ return not e.empty() and std::ranges::all_of(e, ::isdigit); })
    | std::views::transform([](auto&& e){ return std::stoi(e); })
    | std::ranges::to<std::vector<int>>();
  elog_if(keys.size() != bindings.size(), "Invalid indices in bindings database");
  auto it = std::ranges::max_element(bindings, std::less<>{});
  return ( it == bindings.end() )? -1 : *it;
}

} // namespace

/**
 * @brief Adds a binding instruction to the binding database
 * 
 * @param path_file_db_binding Path to the bindings database file
 * @param cmd Binding instructions
 * @return Expected<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Expected<void> add(fs::path const& path_file_db_binding
  , CmdBindType bind_type
  , fs::path const& path_src
  , fs::path const& path_dst
)
{
  // Open database or reset if doesnt exist or is corrupted
  ns_db::Db db = Expect(ns_db::read_file(path_file_db_binding));
  // Find out the highest bind index
  std::string idx = std::to_string(get_highest_index(db) + 1);
  ns_log::info()("Binding index is '{}'", idx);
  // Include bindings
  db(idx)("type") = (bind_type == CmdBindType::RO)? "ro"
    : (bind_type == CmdBindType::RW)? "rw"
    : "dev";
  db(idx)("src") = path_src;
  db(idx)("dst") =  path_dst;
  // Write database
  Expect(ns_db::write_file(path_file_db_binding, db));
  return {};
}

/**
 * @brief Deletes a binding from the database
 * 
 * @param path_file_db_binding Path to the bindings database file
 * @param cmd Structure with the index to remove from the database
 * @return Expected<void> 
 */
[[nodiscard]] inline Expected<void> del(fs::path const& path_file_db_binding, ns_bind::index_t index)
{
  std::error_code ec;
  // Check if database exists
  qreturn_if(not fs::exists(path_file_db_binding, ec), Unexpected("Empty binding database"));
  //  Open file
  std::ifstream file{path_file_db_binding};
  qreturn_if(not file.is_open(), Unexpected("Failed to open binding database to delete entry"));
  // Deserialize bindings
  ns_db::ns_bind::Binds binds = Expect(ns_db::ns_bind::deserialize(file));
  // Erase index if exists
  binds.erase(index);
  // Write back to file
  auto serialized = Expect(ns_db::ns_bind::serialize(binds));
  Expect(ns_db::write_file(path_file_db_binding, serialized));
  return {};
}

/**
 * @brief List bindings from the given bindings database
 * 
 * @param path_file_db_binding Path to the bindings database file
 */
[[nodiscard]] inline Expected<void> list(fs::path const& path_file_db_binding)
{
  auto db = Expect(ns_db::read_file(path_file_db_binding));
  if(not db.empty())
  {
    std::println("{}", db.dump());
  }
  return {};
}

} // namespace ns_cmd::ns_bind

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
