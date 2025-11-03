/**
 * @file environment.hpp
 * @author Ruan Formigoni
 * @brief Manages environment variables in flatimage
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <ranges>
#include <filesystem>
#include <format>

#include "../std/expected.hpp"
#include "../lib/env.hpp"
#include "../reserved/env.hpp"
#include "db.hpp"

namespace ns_db::ns_env
{

namespace
{

namespace fs = std::filesystem;

/**
 * @brief Given a string with an environment variable entry, splits the variable into key and value.
 * 
 * @param entries The environment variables, each entry has the format 'key=var'
 * @return The list of key/value pairs, invalid entries are ignored
 */
[[nodiscard]] inline std::unordered_map<std::string,std::string> key_value(std::vector<std::string> const& entries)
{
  return entries
    | std::views::filter([](auto&& e){ return e.find('=') != std::string::npos; })
    | std::views::transform([](auto&& e)
      {
        auto it = e.find('=');
        return std::make_pair(e.substr(0, it), e.substr(it+1, std::string::npos));
      })
    | std::ranges::to<std::unordered_map<std::string,std::string>>();
}

/**
 * @brief Validates environment variable entries
 * 
 * The validation makes sure they are in the 'key=val' format
 * 
 * @param entries The entries to validate
 * @return Nothing if all variables are valid or the respective error
 */
[[nodiscard]] inline Expected<void> validate(std::vector<std::string> const& entries)
{
  for (auto&& entry : entries)
  {
    if (std::ranges::count_if(entry, [](char c){ return c == '='; }) == 0)
    {
      return Unexpected("C::Variable assignment '{}' is invalid", entry);
    }
  }
  return {};
}

} // namespace

/**
 * @brief Deletes a list of environment variables from the database
 * 
 * @param path_file_binary Path to the database with environment variables
 * @param entries List of environment variables to erase
 */
[[nodiscard]] inline Expected<void> del(fs::path const& path_file_binary, std::vector<std::string> const& entries)
{
  ns_db::Db db = ns_db::from_string(Expect(ns_reserved::ns_env::read(path_file_binary))).value_or(ns_db::Db());
  std::ranges::for_each(entries, [&](auto const& entry)
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
  Expect(ns_reserved::ns_env::write(path_file_binary, db.dump()));
  return {};
}

/**
 * @brief Adds a new environment variable to the jdatabase
 * 
 * @param path_file_binary The path to the environment variable database
 * @param entries List of environment variables to append to the existing ones
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> add(fs::path const& path_file_binary, std::vector<std::string> const& entries)
{
  // Validate entries
  Expect(validate(entries));
  // Insert environment variables in the database
  ns_db::Db db = ns_db::from_string(Expect(ns_reserved::ns_env::read(path_file_binary))).value_or(ns_db::Db());
  for (auto&& [key,value] : key_value(entries))
  {
    db(key) = value;
    ns_log::info()("Included variable '{}' with value '{}'", key, value);
  }
  // Write to the database
  Expect(ns_reserved::ns_env::write(path_file_binary, db.dump()));
  return {};
}

/**
 * @brief Resets all defined environment variables to the ones passed as an argument
 * 
 * @param path_file_binary The path to the environment variable database
 * @param entries List of environment variables to set
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Expected<void> set(fs::path const& path_file_binary, std::vector<std::string> const& entries)
{
  Expect(ns_reserved::ns_env::write(path_file_binary, ns_db::Db().dump()));
  Expect(add(path_file_binary, entries));
  return {};
}

/**
 * @brief Get existing variables from the database
 * 
 * @param path_file_binary The path to the environment variable database
 * @return The list of environment variables, or the respective error
 */
[[nodiscard]] inline Expected<std::vector<std::string>> get(fs::path const& path_file_binary)
{
  // Get environment
  ns_db::Db db = ns_db::from_string(Expect(ns_reserved::ns_env::read(path_file_binary))).value_or(ns_db::Db());
  // Merge variables with values
  std::vector<std::string> environment;
  for (auto&& [key,value] : db.items())
  {
    environment.push_back(std::format("{}={}", key, value.template value<std::string>().value()));
  }
  // Expand variables
  for(auto& variable : environment)
  {
    if(auto expanded = ::ns_env::expand(variable))
    {
      variable = *expanded;
    }
    else
    {
      ns_log::error()("Failed to expand variable: {}", expanded.error());
    }
  }
  return environment;
}

} // namespace ns_db::ns_env


/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
