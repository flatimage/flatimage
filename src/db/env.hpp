/**
 * @file env.hpp
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

/**
 * @namespace ns_db::ns_env
 * @brief Environment variable database management
 *
 * Manages environment variables stored in FlatImage's reserved space. Provides validation,
 * serialization/deserialization, and key-value parsing for environment entries in 'KEY=VALUE'
 * format. Supports reading from and writing to the binary's embedded configuration with
 * configurable environment variable injection into the container.
 */
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
[[nodiscard]] inline std::unordered_map<std::string,std::string> map(std::vector<std::string> const& entries)
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
[[nodiscard]] inline Value<void> validate(std::vector<std::string> const& entries)
{
  for (auto&& entry : entries)
  {
    if (std::ranges::count_if(entry, [](char c){ return c == '='; }) == 0)
    {
      return Error("C::Variable assignment '{}' is invalid", entry);
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
[[nodiscard]] inline Value<void> del(fs::path const& path_file_binary, std::vector<std::string> const& entries)
{
  ns_db::Db db = ns_db::from_string(Pop(ns_reserved::ns_env::read(path_file_binary))).value_or(ns_db::Db());
  std::ranges::for_each(entries, [&](auto const& entry)
  {
    if( db.erase(entry) )
    {
      logger("I::Erase key '{}'", entry);
    }
    else
    {
      logger("I::Key '{}' not found for deletion", entry);
    }
  });
  Pop(ns_reserved::ns_env::write(path_file_binary, Pop(db.dump())));
  return {};
}

/**
 * @brief Adds a new environment variable to the database
 *
 * @param path_file_binary The path to the environment variable database
 * @param entries List of environment variables to append to the existing ones
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> add(fs::path const& path_file_binary, std::vector<std::string> const& entries)
{
  // Validate entries
  Pop(validate(entries));
  // Insert environment variables in the database
  ns_db::Db db = ns_db::from_string(Pop(ns_reserved::ns_env::read(path_file_binary))).value_or(ns_db::Db());
  for (auto&& [key,value] : map(entries))
  {
    db(key) = value;
    logger("I::Included variable '{}' with value '{}'", key, value);
  }
  // Write to the database
  Pop(ns_reserved::ns_env::write(path_file_binary, Pop(db.dump())));
  return {};
}

/**
 * @brief Resets all defined environment variables to the ones passed as an argument
 *
 * @param path_file_binary The path to the environment variable database
 * @param entries List of environment variables to set
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> set(fs::path const& path_file_binary, std::vector<std::string> const& entries)
{
  Pop(ns_reserved::ns_env::write(path_file_binary, Pop(ns_db::Db().dump())));
  Pop(add(path_file_binary, entries));
  return {};
}

/**
 * @brief Get existing variables from the database
 *
 * @param path_file_binary The path to the environment variable database
 * @return The list of environment variables, or the respective error
 */
[[nodiscard]] inline Value<std::vector<std::string>> get(fs::path const& path_file_binary)
{
  // Get environment
  ns_db::Db db = ns_db::from_string(Pop(ns_reserved::ns_env::read(path_file_binary))).value_or(ns_db::Db());
  // Merge variables with values
  std::vector<std::string> environment;
  for (auto&& [key,value] : db.items())
  {
    environment.push_back(std::format("{}={}", key, Pop(value.template value<std::string>())));
  }
  // Expand variables
  for(auto& variable : environment)
  {
    variable = ::ns_env::expand(variable).value_or(variable);
  }
  return environment;
}

} // namespace ns_db::ns_env


/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
