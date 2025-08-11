///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : environment
///

#pragma once

#include <ranges>
#include <filesystem>
#include <format>

#include "../lib/env.hpp"
#include "../db/db.hpp"

namespace ns_config::ns_environment
{

namespace
{

namespace fs = std::filesystem;

inline std::vector<std::pair<std::string,std::string>> key_value(std::vector<std::string> const& entries)
{
  return entries
    | std::views::transform([](auto&& e)
      {
        auto it = e.find('=');
        ethrow_if(it == std::string::npos, "Failed to read equal sign separator");
        return std::make_pair(e.substr(0, it), e.substr(it+1, std::string::npos));
      })
    | std::ranges::to<std::vector<std::pair<std::string,std::string>>>();
} // key_value

inline void validate(std::vector<std::string> const& entries)
{
  for (auto&& entry : entries)
  {
    ethrow_if(std::ranges::count_if(entry, [](char c){ return c == '='; }) == 0
      , std::format("Argument '{}' is invalid", entry)
    );
  } // for
} // validate

} // namespace

inline void del(fs::path const& path_file_config_environment, std::vector<std::string> const& entries)
{
  auto db = ns_db::read_file(path_file_config_environment).value_or(ns_db::Db());
  std::ranges::for_each(entries, [&](auto&& entry)
  {
    if( db.erase(entry) )
    {
      ns_log::info()("Erase key '{}'", entry);
    }
    else
    {
      ns_log::info()("Key '{}' not found for deletion", entry);
    }
  });
  ereturn_if(not ns_db::write_file(path_file_config_environment, db), "Failure to write deletions to database");
}

inline void add(fs::path const& path_file_config_environment, std::vector<std::string> entries)
{
  validate(entries);
  del(path_file_config_environment, entries);
  auto db = ns_db::read_file(path_file_config_environment).value_or(ns_db::Db());
  for (auto&& [key,value] : key_value(entries))
  {
    db(key) = value;
    ns_log::info()("Included variable '{}' with value '{}'", key, value);
  }
  ereturn_if(not ns_db::write_file(path_file_config_environment, db), "Failure to modify database");
}

inline void set(fs::path const& path_file_config_environment, std::vector<std::string> entries)
{
  fs::remove(path_file_config_environment);
  add(path_file_config_environment, entries);
}

inline std::vector<std::string> get(fs::path const& path_file_config_environment)
{
  // Get environment
  auto db_environment = ns_db::read_file(path_file_config_environment).value_or(ns_db::Db());
  // Merge variables with values
  std::vector<std::string> environment;
  for (auto&& [key,value] : db_environment.items())
  {
    environment.push_back(std::format("{}={}", key, value.template value<std::string>().value()));
  } // for
  // Expand variables
  for(auto& variable : environment)
  {
    if(auto expanded = ns_env::expand(variable))
    {
      variable = *expanded;
    } // if
    else
    {
      ns_log::error()("Failed to expand variable: {}", expanded.error());
    } // else
  } // for
  return environment;
}

} // namespace ns_config::ns_environment


/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
