///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : bind
///@created     : Wednesday Sep 11, 2024 15:00:09 -03
///

#pragma once

#include <filesystem>
#include <algorithm>
#include "../../cpp/lib/db.hpp"
#include "../../cpp/lib/db/bind.hpp"
#include "../../cpp/std/enum.hpp"
#include "../../cpp/std/variant.hpp"

namespace ns_cmd::ns_bind
{

ENUM(CmdBindOp,ADD,DEL,LIST);
ENUM(CmdBindType,RO,RW,DEV);

using index_t = uint64_t;
using bind_t = struct { CmdBindType type; std::string src; std::string dst; };
using data_t = std::variant<index_t,bind_t,std::false_type>;

struct CmdBind
{
  ns_cmd::ns_bind::CmdBindOp op;
  data_t data;
};

namespace
{

namespace fs = std::filesystem;

// fn: get_highest_index() {{{
inline index_t get_highest_index(ns_db::Db const& db)
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
inline void add(fs::path const& path_file_config, CmdBind const& cmd)
{
  // Get src and dst paths
  auto binds = ns_variant::get_if_holds_alternative<bind_t>(cmd.data);
  ethrow_if(not binds, "Invalid data type for 'add' command");
  // Open database or reset if doesnt exist or is corrupted
  auto db = ns_db::read_file(path_file_config).value_or(ns_db::Db());
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
  auto result_write = ns_db::write_file(path_file_config, db);
  ethrow_if(not result_write, result_write.error());
} // fn: add() }}}

// fn: del() {{{
inline void del(fs::path const& path_file_config, CmdBind const& cmd)
{
  // Get index to erase
  auto index = std::get_if<index_t>(&cmd.data);
  ereturn_if(not index, "Failed to read index value from bind database");
  //  Open file
  std::ifstream file{path_file_config};
  ereturn_if(not file, "Failed to open database to delete entry");
  // Deserialize bindings
  auto expected_binds = ns_db::ns_bind::deserialize(file);
  ereturn_if(not expected_binds, expected_binds.error());
  // Erase index if exists
  expected_binds->erase(*index);
  // Write back to file
  auto serialized = ns_db::ns_bind::serialize(expected_binds.value());
  ereturn_if(not serialized, "Could not serialize bindings");
  ereturn_if(not ns_db::write_file(path_file_config, serialized.value()),
    "Failed to write bindings database"
  );
} // fn: del() }}}

// fn: list() {{{
inline void list(fs::path const& path_file_config)
{
  // Open db
  auto db = ns_db::read_file(path_file_config).value_or(ns_db::Db());
  // Print entries to stdout
  println(db.dump());
} // fn: list() }}}

} // namespace ns_cmd::ns_bind

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
