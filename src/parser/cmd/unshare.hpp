/**
 * @file unshare.hpp
 * @author Ruan Formigoni
 * @brief Interface for managing unshare namespace options
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <print>

#include "../../reserved/unshare.hpp"

/**
 * @namespace ns_cmd::ns_unshare
 * @brief Unshare namespace options command implementation
 *
 * Implements the fim-unshare command for managing namespace unsharing options.
 * Provides operations to set, add, clear, and list unshare options such as user,
 * ipc, pid, net, uts, and cgroup namespaces. Persists configurations to reserved space.
 */
namespace ns_cmd::ns_unshare
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Sets the unshare options (replaces existing)
 *
 * @param path_file_binary Path to the flatimage binary
 * @param unshares Set of unshare options to enable
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> set(fs::path const& path_file_binary
  , std::set<ns_reserved::ns_unshare::Unshare> const& unshares
)
{
  ns_reserved::ns_unshare::Unshares manager(path_file_binary);
  Pop(manager.set(unshares));
  return {};
}

/**
 * @brief Adds unshare options to existing configuration
 *
 * @param path_file_binary Path to the flatimage binary
 * @param unshares Set of unshare options to add
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> add(fs::path const& path_file_binary
  , std::set<ns_reserved::ns_unshare::Unshare> const& unshares
)
{
  ns_reserved::ns_unshare::Unshares manager(path_file_binary);
  Pop(manager.add(unshares));
  return {};
}

/**
 * @brief Removes unshare options from existing configuration
 *
 * @param path_file_binary Path to the flatimage binary
 * @param unshares Set of unshare options to remove
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> del(fs::path const& path_file_binary
  , std::set<ns_reserved::ns_unshare::Unshare> const& unshares
)
{
  ns_reserved::ns_unshare::Unshares manager(path_file_binary);
  Pop(manager.del(unshares));
  return {};
}

/**
 * @brief Clears all unshare options
 *
 * @param path_file_binary Path to the flatimage binary
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> clear(fs::path const& path_file_binary)
{
  ns_reserved::ns_unshare::Unshares manager(path_file_binary);
  Pop(manager.clear());
  return {};
}

/**
 * @brief Lists all enabled unshare options
 *
 * @param path_file_binary Path to the flatimage binary
 * @return Value<void> Nothing on success or the respective error
 */
[[nodiscard]] inline Value<void> list(fs::path const& path_file_binary)
{
  ns_reserved::ns_unshare::Unshares manager(path_file_binary);
  auto unshares = Pop(manager.to_strings());
  for(auto const& unshare : unshares)
  {
    std::println("{}", unshare);
  }
  return {};
}

} // namespace ns_cmd::ns_unshare

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
