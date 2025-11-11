/**
 * @file recipe.hpp
 * @author Ruan Formigoni
 * @brief Defines a class that manages FlatImage's recipe configuration
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <vector>

#include "../std/expected.hpp"
#include "db.hpp"

namespace ns_db::ns_recipe
{

class Recipe
{
  private:
    std::string m_description;
    std::vector<std::string> m_packages;
    std::vector<std::string> m_dependencies;
  public:
    Recipe();
    [[maybe_unused]] std::string const& get_description() const { return m_description; }
    [[maybe_unused]] std::vector<std::string> const& get_packages() const { return m_packages; }
    [[maybe_unused]] std::vector<std::string> const& get_dependencies() const { return m_dependencies; }
    [[maybe_unused]] void set_description(std::string_view str_description) { m_description = str_description; }
    [[maybe_unused]] void set_packages(std::vector<std::string> const& vec_packages) { m_packages = vec_packages; }
    [[maybe_unused]] void set_dependencies(std::vector<std::string> const& vec_dependencies) { m_dependencies = vec_dependencies; }
  friend Value<Recipe> deserialize(std::string_view raw_json) noexcept;
  friend Value<std::string> serialize(Recipe const& recipe) noexcept;
}; // Recipe

/**
 * @brief Construct a new Recipe:: Recipe object
 */
inline Recipe::Recipe()
  : m_description()
  , m_packages()
  , m_dependencies()
{}

/**
 * @brief Deserializes a string into a `Recipe` class
 *
 * @param raw_json The json string which to deserialize
 * @return The `Recipe` class or the respective error
 */
[[maybe_unused]] [[nodiscard]] inline Value<Recipe> deserialize(std::string_view str_raw_json) noexcept
{
  Recipe recipe;
  return_if(str_raw_json.empty(), Error("D::Empty json data"));
  // Open DB
  auto db = Pop(ns_db::from_string(str_raw_json));
  // Parse description (required)
  recipe.m_description = Pop(db("description").template value<std::string>(), "E::Missing 'description' field");
  // Parse packages (required)
  recipe.m_packages = Pop(db("packages").value<std::vector<std::string>>(), "E::Missing 'packages' field");
  // Parse dependencies (optional, defaults to empty vector)
  recipe.m_dependencies = db("dependencies").value<std::vector<std::string>>().or_default();
  return recipe;
}

/**
 * @brief Serializes a `Recipe` class into a json string
 *
 * @param recipe The `Recipe` object to serialize
 * @return The serialized json data;
 */
[[maybe_unused]] [[nodiscard]] inline Value<std::string> serialize(Recipe const& recipe) noexcept
{
  auto db = ns_db::Db();
  db("description") = recipe.m_description;
  db("packages") = recipe.m_packages;
  if (!recipe.m_dependencies.empty())
  {
    db("dependencies") = recipe.m_dependencies;
  }
  return db.dump();
}

} // namespace ns_db::ns_recipe

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
