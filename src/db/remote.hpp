/**
 * @file remote.hpp
 * @author Ruan Formigoni
 * @brief Manages remote URL in flatimage
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <string>

#include "../std/expected.hpp"
#include "../reserved/remote.hpp"
#include "db.hpp"

namespace ns_db::ns_remote
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Sets a remote URL in the database
 *
 * @param path_file_binary Path to the binary with remote URL database
 * @param url The remote URL to set
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> set(fs::path const& path_file_binary, std::string const& url)
{
  // Create database
  ns_db::Db db;
  // Insert URL
  db("url") = url;
  ns_log::info()("Set remote URL to '{}'", url);
  // Write to the database
  Pop(ns_reserved::ns_remote::write(path_file_binary, Pop(db.dump())));
  return {};
}

/**
 * @brief Gets the remote URL from the database
 *
 * @param path_file_binary Path to the binary with remote URL database
 * @return The remote URL on success, or the respective error
 */
[[nodiscard]] inline Value<std::string> get(fs::path const& path_file_binary)
{
  // Read database
  ns_db::Db db = ns_db::from_string(
    Pop(ns_reserved::ns_remote::read(path_file_binary))
  ).value_or(ns_db::Db());
  // Check if URL exists
  if (db.empty() or not db.contains("url"))
  {
    return Error("E::No remote URL configured");
  }
  // Return URL
  return Pop(db("url").value<std::string>(), "E::Could not read URL");
}

/**
 * @brief Clears the remote URL from the database
 *
 * @param path_file_binary Path to the binary with remote URL database
 * @return Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> clear(fs::path const& path_file_binary)
{
  // Create empty database
  ns_db::Db db;
  // Write empty database
  Pop(ns_reserved::ns_remote::write(path_file_binary, Pop(db.dump())));
  ns_log::info()("Cleared remote URL");
  return {};
}

} // namespace ns_db::ns_remote

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
