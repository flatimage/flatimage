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
#include "desktop.hpp"

/**
 * @namespace ns_db::ns_recipe
 * @brief Package recipe configuration management
 *
 * Handles recipe definitions for installing distribution-specific packages.
 * Manages package lists, dependencies, and descriptions for pre-configured software bundles
 * (e.g., audio drivers, GPU support, desktop environments). Supports JSON-based recipe
 * format with recursive dependency resolution.
 */
namespace ns_db::ns_recipe
{

class Recipe
{
  private:
    std::string m_description;
    std::vector<std::string> m_packages;
    std::vector<std::string> m_dependencies;
    Value<ns_desktop::Desktop> m_desktop;
  public:
    Recipe();
    [[maybe_unused]] std::string const& get_description() const { return m_description; }
    [[maybe_unused]] std::vector<std::string> const& get_packages() const { return m_packages; }
    [[maybe_unused]] std::vector<std::string> const& get_dependencies() const { return m_dependencies; }
    [[maybe_unused]] Value<ns_desktop::Desktop> const& get_desktop() const { return m_desktop; }
    [[maybe_unused]] void set_description(std::string_view str_description) { m_description = str_description; }
    [[maybe_unused]] void set_packages(std::vector<std::string> const& vec_packages) { m_packages = vec_packages; }
    [[maybe_unused]] void set_dependencies(std::vector<std::string> const& vec_dependencies) { m_dependencies = vec_dependencies; }
    [[maybe_unused]] void set_desktop(Value<ns_desktop::Desktop> const& desktop) { m_desktop = desktop; }
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
  , m_desktop(std::unexpected("m_desktop is undefined"))
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
  // Parse desktop integration (optional)
  if(auto desktop_json = db("desktop").dump(); desktop_json)
  {
    recipe.m_desktop = ns_desktop::deserialize(Pop(desktop_json));
  }
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
  if (recipe.m_desktop)
  {
    auto desktop_json_str = Pop(ns_desktop::serialize(Pop(recipe.m_desktop)));
    auto desktop_db = Pop(ns_db::from_string(desktop_json_str));
    db("desktop") = desktop_db.data();
  }
  return db.dump();
}

} // namespace ns_db::ns_recipe

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
