/**
 * @file boot.hpp
 * @author Ruan Formigoni
 * @brief Defines a class that manages FlatImage's boot configuration
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <vector>

#include "../std/expected.hpp"
#include "db.hpp"

/**
 * @namespace ns_db::ns_boot
 * @brief Boot command configuration storage
 *
 * Manages the default boot command and arguments for FlatImage applications. Stores the program
 * path and argument vector that will be executed when the FlatImage is launched without explicit
 * commands. Supports JSON serialization/deserialization for persistent storage in reserved space.
 */
namespace ns_db::ns_boot
{

class Boot
{
  private:
    std::string m_program;
    std::vector<std::string> m_args;
  public:
    Boot();
    [[maybe_unused]] std::string const& get_program() const { return m_program; }
    [[maybe_unused]] std::vector<std::string> const& get_args() const { return m_args; }
    [[maybe_unused]] void set_program(std::string_view str_program) { m_program = str_program; }
    [[maybe_unused]] void set_args(std::vector<std::string> const& vec_args) { m_args = vec_args; }
  friend Value<Boot> deserialize(std::string_view str_raw_json) noexcept;
  friend Value<std::string> serialize(Boot const& boot) noexcept;
};

/**
 * @brief Construct a new Boot:: Boot object
 */
inline Boot::Boot()
  : m_program()
  , m_args()
{}

/**
 * @brief Deserializes a json string into a `Boot` class
 *
 * @param str_raw_json The json string which to deserialize
 * @return The `Boot` class or the respective error
 */
[[maybe_unused]] [[nodiscard]] inline Value<Boot> deserialize(std::string_view str_raw_json) noexcept
{
  Boot boot;
  return_if(str_raw_json.empty(), Error("D::Empty json data"));
  // Open DB
  auto db = Pop(ns_db::from_string(str_raw_json));
  // Parse program (optional, defaults to empty string)
  boot.m_program = Pop(db("program").template value<std::string>());
  // Parse args (optional, defaults to empty vector)
  boot.m_args = Pop(db("args").value<std::vector<std::string>>());
  return boot;
}

/**
 * @brief Serializes a `Boot` class into a json string
 *
 * @param boot The `Boot` object to serialize
 * @return The serialized json data;
 */
[[maybe_unused]] [[nodiscard]] inline Value<std::string> serialize(Boot const& boot) noexcept
{
  auto db = ns_db::Db();
  db("program") = boot.m_program;
  db("args") = boot.m_args;
  return db.dump();
}

} // namespace ns_db::ns_boot

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
