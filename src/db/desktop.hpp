///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : desktop
///

#pragma once

#include <string>
#include <set>

#include "../std/enum.hpp"
#include "db.hpp"

namespace ns_db::ns_desktop
{

ENUM(IntegrationItem, ENTRY, MIMETYPE, ICON);

namespace
{

}
// struct Desktop {{{
class Desktop
{
  private:
    std::string m_name;
    Expected<fs::path> m_path_file_icon;
    std::set<IntegrationItem> m_set_integrations;
    std::set<std::string> m_set_categories;
  public:
    Desktop();
    [[maybe_unused]] std::string const& get_name() const { return m_name; }
    [[maybe_unused]] Expected<fs::path> const& get_path_file_icon() const { return m_path_file_icon; }
    [[maybe_unused]] std::set<IntegrationItem> const& get_integrations() const { return m_set_integrations; }
    [[maybe_unused]] std::set<std::string> const& get_categories() const { return m_set_categories; }
    [[maybe_unused]] void set_name(std::string_view str_name) { m_name = str_name; }
    [[maybe_unused]] void set_integrations(std::set<IntegrationItem> const& set_integrations) { m_set_integrations = set_integrations; }
    [[maybe_unused]] void set_categories(std::set<std::string> const& set_categories) { m_set_categories = set_categories; }
  friend Expected<Desktop> from_string(std::string_view raw_json) noexcept;
  friend Expected<std::string> serialize(Desktop const& desktop) noexcept;
}; // Desktop }}}


inline Desktop::Desktop()
  : m_name()
  , m_path_file_icon(std::unexpected("m_path_file_icon is undefined"))
  , m_set_integrations()
  , m_set_categories()
{}

inline Expected<Desktop> from_string(std::string_view raw_json) noexcept
{
  Desktop desktop;
  // Open DB
  auto db = Expect(ns_db::from_string(raw_json));
  // Parse name (required)
  desktop.m_name = Expect(db("name").template value<std::string>());
  // Parse icon path (optional)
  desktop.m_path_file_icon = db("icon").template value<std::string>();
  // Parse enabled integrations (optional)
  if (auto integrations = db("integrations").template value<std::vector<std::string>>())
  {
    std::ranges::for_each(integrations.value(), [&](auto&& e){ desktop.m_set_integrations.insert(e); });
  } // if
  // Parse categories (required)
  auto db_categories = db("categories").template value<std::vector<std::string>>().value();
  std::ranges::for_each(db_categories, [&](auto&& e){ desktop.m_set_categories.insert(e); });
  return {};
}

// deserialize() {{{
[[maybe_unused]] [[nodiscard]] inline Expected<Desktop> deserialize(std::string_view str_raw_json) noexcept
{
  return from_string(str_raw_json);
}
// deserialize() }}}

// deserialize() {{{
[[maybe_unused]] [[nodiscard]] inline Expected<Desktop> deserialize(std::ifstream& stream_raw_json) noexcept
{
  std::stringstream ss;
  ss << stream_raw_json.rdbuf();
  return from_string(ss.str());
}
// deserialize() }}}

// serialize() {{{
[[maybe_unused]] [[nodiscard]] inline Expected<std::string> serialize(Desktop const& desktop) noexcept
{
  return ns_exception::to_expected([&]
  {
    auto db = ns_db::Db();
    db("name") = desktop.m_name;
    db("integrations") = desktop.m_set_integrations;
    if (desktop.m_path_file_icon)
    {
      db("icon") = desktop.m_path_file_icon.value();
    }
    db("categories") = desktop.m_set_categories;
    return db.dump();
  });
} // serialize() }}}

} // namespace ns_db::ns_desktop

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
