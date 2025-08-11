///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : bind
///@created     : Wednesday Sep 11, 2024 15:00:09 -03
///

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

// fn: get_highest_index() {{{
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
} // fn: get_highest_index() }}}

} // namespace

// fn: add() {{{
[[nodiscard]] inline std::expected<void,std::string> add(fs::path const& path_file_config, CmdBind const& cmd)
{
  // Get src and dst paths
  bind_t const * binds = std::get_if<bind_t>(&cmd.data);
  qreturn_if(not binds, std::unexpected("Invalid data type for 'add' command"));
  // Open database or reset if doesnt exist or is corrupted
  ns_db::Db db = expect(ns_db::read_file(path_file_config));
  // Find out the highest bind index
  std::string idx = std::to_string(get_highest_index(db) + 1);
  ns_log::info()("Binding index is '{}'", idx);
  // Include bindings
  db(idx)("type") = (binds->type == CmdBindType::RO)? "ro"
    : (binds->type == CmdBindType::RW)? "rw"
    : "dev";
  db(idx)("src") = binds->src;
  db(idx)("dst") =  binds->dst;
  // Write database
  expect(ns_db::write_file(path_file_config, db));
  return {};
} // fn: add() }}}

// fn: del() {{{
[[nodiscard]] inline std::expected<void,std::string> del(fs::path const& path_file_config, CmdBind const& cmd)
{
  // Get index to erase
  auto index = std::get_if<index_t>(&cmd.data);
  qreturn_if(not index, std::unexpected("Failed to read index value from bind database"));
  //  Open file
  std::ifstream file{path_file_config};
  qreturn_if(not file, std::unexpected("Failed to open database to delete entry"));
  // Deserialize bindings
  auto expected_binds = ns_db::ns_bind::deserialize(file);
  qreturn_if(not expected_binds, std::unexpected(expected_binds.error()));
  // Erase index if exists
  expected_binds->erase(*index);
  // Write back to file
  auto serialized = ns_db::ns_bind::serialize(expected_binds.value());
  qreturn_if(not serialized, std::unexpected("Could not serialize bindings"));
  qreturn_if(not ns_db::write_file(path_file_config, serialized.value()),
    std::unexpected("Failed to write bindings database")
  );
  return {};
} // fn: del() }}}

// fn: list() {{{
inline void list(fs::path const& path_file_config)
{
  println(ns_db::read_file(path_file_config).value_or(ns_db::Db()).dump());
} // fn: list() }}}

} // namespace ns_cmd::ns_bind

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
