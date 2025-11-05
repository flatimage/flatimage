/**
 * @file desktop.hpp
 * @author Ruan Formigoni
 * @brief Defines a class that manages FlatImage's desktop integration
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <set>

#include "../std/expected.hpp"
#include "../std/enum.hpp"
#include "db.hpp"

namespace ns_db::ns_desktop
{

ENUM(IntegrationItem, ENTRY, MIMETYPE, ICON);

class Desktop
{
  private:
    std::string m_name;
    Value<fs::path> m_path_file_icon;
    std::set<IntegrationItem> m_set_integrations;
    std::set<std::string> m_set_categories;
  public:
    Desktop();
    [[maybe_unused]] std::string const& get_name() const { return m_name; }
    [[maybe_unused]] Value<fs::path> const& get_path_file_icon() const { return m_path_file_icon; }
    [[maybe_unused]] std::set<IntegrationItem> const& get_integrations() const { return m_set_integrations; }
    [[maybe_unused]] std::set<std::string> const& get_categories() const { return m_set_categories; }
    [[maybe_unused]] void set_name(std::string_view str_name) { m_name = str_name; }
    [[maybe_unused]] void set_integrations(std::set<IntegrationItem> const& set_integrations) { m_set_integrations = set_integrations; }
    [[maybe_unused]] void set_categories(std::set<std::string> const& set_categories) { m_set_categories = set_categories; }
  friend Value<Desktop> deserialize(std::string_view raw_json) noexcept;
  friend Value<std::string> serialize(Desktop const& desktop) noexcept;
}; // Desktop }}}


/**
 * @brief Construct a new Desktop:: Desktop object
 */
inline Desktop::Desktop()
  : m_name()
  , m_path_file_icon(std::unexpected("m_path_file_icon is undefined"))
  , m_set_integrations()
  , m_set_categories()
{}

/**
 * @brief Deserializes a string into a `Desktop` class
 * 
 * @param raw_json The json string which to deserialize
 * @return The `Desktop` class or the respective error
 */
[[maybe_unused]] [[nodiscard]] inline Value<Desktop> deserialize(std::string_view str_raw_json) noexcept
{
  Desktop desktop;
  qreturn_if(str_raw_json.empty(), Error("W::Empty json data"));
  // Open DB
  auto db = Pop(ns_db::from_string(str_raw_json));
  // Parse name (required)
  desktop.m_name = Pop(db("name").template value<std::string>());
  // Parse icon path (optional)
  desktop.m_path_file_icon = db("icon").template value<std::string>();
  // Parse enabled integrations (optional)
  for(auto&& item : db("integrations").value<std::vector<std::string>>().or_default())
  {
    desktop.m_set_integrations.insert(Pop(IntegrationItem::from_string(item)));
  }
  // Parse categories (required)
  auto db_categories = Pop(db("categories").template value<std::vector<std::string>>());
  std::ranges::for_each(db_categories, [&](auto&& e){ desktop.m_set_categories.insert(e); });
  return desktop;
}

/**
 * @brief Deserializes a input string stream into a `Desktop` class
 * 
 * @param stream_raw_json The json string which to deserialize
 * @return The `Desktop` class or the respective error
 */
[[maybe_unused]] [[nodiscard]] inline Value<Desktop> deserialize(std::ifstream& stream_raw_json) noexcept
{
  std::stringstream ss;
  ss << stream_raw_json.rdbuf();
  return deserialize(ss.str());
}

/**
 * @brief Serializes a `Desktop` class into a json string
 * 
 * @param desktop The `Desktop` object to deserialize
 * @return The serialized json data;
 */
[[maybe_unused]] [[nodiscard]] inline Value<std::string> serialize(Desktop const& desktop) noexcept
{
  auto db = ns_db::Db();
  db("name") = desktop.m_name;
  db("integrations") = desktop.m_set_integrations;
  if (desktop.m_path_file_icon)
  {
    db("icon") = Pop(desktop.m_path_file_icon);
  }
  db("categories") = desktop.m_set_categories;
  return db.dump();
}

} // namespace ns_db::ns_desktop

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
