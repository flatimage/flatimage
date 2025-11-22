/**
 * @file recipe.hpp
 * @author Ruan Formigoni
 * @brief Manages recipes in FlatImage
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <string>

#include "../../lib/subprocess.hpp"
#include "../../lib/log.hpp"
#include "../../std/expected.hpp"
#include "../../std/filesystem.hpp"
#include "../../db/recipe.hpp"
#include "../../config.hpp"

namespace
{

namespace fs = std::filesystem;

/**
 * @brief Constructs the path to a recipe file in the local cache
 *
 * @param path_dir_download Directory where recipes are cached
 * @param distribution Name of the distribution
 * @param recipe Name of the recipe
 * @return fs::path Path to the recipe JSON file
 */
inline fs::path get_path_recipe(
    fs::path const& path_dir_download
  , ns_config::Distribution const& distribution
  , std::string const& recipe
)
{
  return path_dir_download / "recipes" / distribution.lower() / "latest" / std::format("{}.json", recipe);
}

} // namespace

/**
 * @namespace ns_recipe
 * @brief Package recipe command implementation
 *
 * Implements the fim-recipe command for installing distribution-specific package recipes.
 * Handles recipe downloading from remote repositories, dependency resolution,
 * package installation via distribution package managers, and recipe caching.
 * Supports pre-configured software bundles like audio, GPU drivers, and desktop environments.
 */
namespace ns_recipe
{

/**
 * @brief Loads a recipe from local cache
 *
 * @param distribution Name of the distribution
 * @param path_dir_download Directory where recipes are cached
 * @param recipe Name of the recipe to load
 * @return Value<ns_db::ns_recipe::Recipe> Recipe object with contents on success, or the respective error
 */
[[nodiscard]] inline Value<ns_db::ns_recipe::Recipe> load_recipe(
    ns_config::Distribution const& distribution
  , fs::path const& path_dir_download
  , std::string const& recipe
)
{
  // Construct path to local recipe file
  fs::path recipe_file = get_path_recipe(path_dir_download, distribution, recipe);
  // Check if recipe file exists locally
  return_if(not fs::exists(recipe_file),
    Error("E::Recipe '{}' not found locally. Use 'fim-recipe fetch {}' first.", recipe, recipe));
  // Read the local JSON file
  std::ifstream file(recipe_file);
  return_if(!file.is_open(),
    Error("E::Could not open recipe file '{}'", recipe_file.string()));
  // Read json contents
  std::string json_contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  // Check if contents are empty
  return_if(json_contents.empty(), Error("E::Empty json file"));
  // Parse JSON and return
  return Pop(ns_db::ns_recipe::deserialize(json_contents), "E::Could not parse json file");
}

/**
 * @brief Internal implementation that fetches a recipe and its dependencies recursively with cycle detection
 *
 * @param distribution Name of the distribution
 * @param url_remote Remote repository URL
 * @param path_file_downloader Path to the downloader executable
 * @param path_dir_download Directory where the recipe will be downloaded
 * @param recipe Name of the recipe to download
 * @param use_existing If true, use existing local file if available; if false, always download
 * @param dependencies Set of fetched recipes to track and detect cycles
 * @return Value<void> Nothing on success, or error on failure/cycle detection
 */
[[nodiscard]] inline Value<void> fetch_impl(
    ns_config::Distribution const& distribution
  , std::string url_remote
  , fs::path const& path_file_downloader
  , fs::path const& path_dir_download
  , std::string const& recipe
  , bool use_existing
  , std::unordered_set<std::string>& dependencies
)
{
  auto f_fetch_dependencies = [&](ns_db::ns_recipe::Recipe const& recipe_obj) -> Value<void>
  {
    auto const& deps = recipe_obj.get_dependencies();
    if (!deps.empty())
    {
      for (auto const& sub_recipe : deps)
      {
        Pop(fetch_impl(distribution, url_remote, path_file_downloader, path_dir_download, sub_recipe, use_existing, dependencies));
      }
    }
    return {};
  };
  // Check cycle
  if(dependencies.contains(recipe))
  {
    return Error("E::Cyclic dependency for recipe '{}'", recipe);
  }
  else
  {
    dependencies.insert(recipe);
  }
  // Construct the output path
  fs::path path_file_output = get_path_recipe(path_dir_download, distribution, recipe);
  fs::path path_dir_output = path_file_output.parent_path();
  // If use_existing is true and file exists, use the cached version
  if (use_existing && Try(fs::exists(path_file_output)))
  {
    logger("I::Using existing recipe from '{}'", path_file_output.string());
    // Read the local JSON file
    std::ifstream file(path_file_output);
    if (!file.is_open())
    {
      return Error("E::Could not open existing recipe file '{}'", path_file_output.string());
    }
    // Read file contents
    std::string json_contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    // Parse JSON
    ns_db::ns_recipe::Recipe recipe_obj = Pop(ns_db::ns_recipe::deserialize(json_contents));
    // Fetch dependencies recursively if they exist
    Pop(f_fetch_dependencies(recipe_obj));
    return {};
  }
  // Download the recipe (either use_existing=false or file doesn't exist)
  // Remove trailing slash from URL if present
  if (url_remote.ends_with('/'))
  {
    url_remote.pop_back();
  }
  // Construct the recipe URL: URL/DISTRO/VERSION/<recipe>.json
  std::string recipe_url = std::format("{}/{}/latest/{}.json", url_remote, distribution.lower(), recipe);
  // Create the output directory if it doesn't exist
  Pop(ns_fs::create_directories(path_dir_output));
  // Download the recipe using wget
  logger("I::Downloading recipe from '{}'", recipe_url);
  logger("I::Saving to '{}'", path_file_output.string());
  // Execute wget to download the file
  Pop(ns_subprocess::Subprocess(path_file_downloader)
    .with_args("-O", path_file_output.string(), recipe_url)
    .spawn()->wait());
  logger("I::Successfully downloaded recipe '{}' to '{}'", recipe, path_file_output.string());
  // Read the downloaded JSON file
  std::ifstream file(path_file_output);
  if (!file.is_open())
  {
    return Error("E::Could not open downloaded recipe file '{}'", path_file_output.string());
  }
  // Read file contents
  std::string json_contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();
  // Parse JSON
  ns_db::ns_recipe::Recipe recipe_obj = Pop(ns_db::ns_recipe::deserialize(json_contents));
  // Fetch dependencies recursively if they exist
  Pop(f_fetch_dependencies(recipe_obj));
  return {};
}

/**
 * @brief Fetches a recipe from the remote repository along with all its dependencies recursively
 *
 * @param distribution Name of the distribution
 * @param url_remote Remote repository URL
 * @param path_file_downloder Path to the downloader executable
 * @param path_dir_download Directory where the recipe will be downloaded
 * @param recipe Name of the recipe to download
 * @param use_existing If true, use existing local file if available; if false, always download
 * @return Value<std::vector<std::string>> Vector of all fetched recipe names (including dependencies) on success, or error
 */
[[nodiscard]] inline Value<std::vector<std::string>> fetch(
    ns_config::Distribution const& distribution
  , std::string url_remote
  , fs::path const& path_file_downloder
  , fs::path const& path_dir_download
  , std::string const& recipe
  , bool use_existing = false
)
{
  std::unordered_set<std::string> dependencies;
  Pop(fetch_impl(distribution, url_remote, path_file_downloder, path_dir_download, recipe, use_existing, dependencies));
  return std::vector(dependencies.begin(), dependencies.end());
}

/**
 * @brief Displays information about a locally cached recipe
 *
 * @param distribution Name of the distribution
 * @param path_dir_download Directory where recipes are cached
 * @param recipe Name of the recipe to inspect
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> info(
    ns_config::Distribution const& distribution
  , fs::path const& path_dir_download
  , std::string const& recipe
)
{
  // Load recipe
  ns_db::ns_recipe::Recipe recipe_obj = Pop(load_recipe(distribution, path_dir_download, recipe));
  // Display recipe information
  fs::path recipe_file = get_path_recipe(path_dir_download, distribution, recipe);
  std::println("Recipe: {}", recipe);
  std::println("Location: {}", recipe_file.string());
  // Description
  std::println("Description: {}", recipe_obj.get_description());
  // Packages
  auto const& packages = recipe_obj.get_packages();
  std::println("Package count: {}", packages.size());
  std::println("Packages:");
  for (auto const& pkg : packages)
  {
    std::println("  - {}", pkg);
  }
  // Dependencies
  auto const& dependencies = recipe_obj.get_dependencies();
  if (!dependencies.empty())
  {
    std::println("Dependencies: {}", dependencies.size());
    for (auto const& dep : dependencies)
    {
      std::println("  - {}", dep);
    }
  }
  else
  {
    std::println("Dependencies: 0");
  }
  return {};
}

/**
 * @brief Installs packages from recipes using the appropriate package manager
 *
 * @tparam F Callable type for executing commands (signature: F(std::string, std::vector<std::string>&))
 * @param distribution Name of the current linux distribution (e.g., "arch", "alpine")
 * @param path_dir_download Directory where recipes are cached
 * @param recipes Vector of recipe names to install packages from
 * @param callback Function to execute the package manager command, receives program name and arguments
 * @return Value<int> Exit code on success, or the respective error
 */
template<typename F>
requires std::invocable<F,std::string,std::vector<std::string>&>
[[nodiscard]] inline Value<int> install(ns_config::Distribution const& distribution
  , fs::path const& path_dir_download
  , std::vector<std::string> const& recipes
  , F&& callback)
{

  // Get packages from recipes
  std::vector<std::string> packages;
  for(auto&& recipe : recipes)
  {
    // Load recipe
    ns_db::ns_recipe::Recipe recipe_obj = Pop(load_recipe(distribution, path_dir_download, recipe), "E::Could not load json recipe");
    // Extract and collect packages
    auto const& recipe_packages = recipe_obj.get_packages();
    std::ranges::copy(recipe_packages, std::back_inserter(packages));
  }
  // Determine package manager command based on distribution
  std::string program;
  std::vector<std::string> args;
  switch(distribution)
  {
    case ns_config::Distribution::ALPINE:
    {
      program = "apk";
      args = {"add", "--no-cache", "--update-cache", "--no-progress"};
    }
    break;
    case ns_config::Distribution::ARCH:
    {
      program = "pacman";
      args = {"-Syu", "--noconfirm", "--needed"};
    }
    break;
    case ns_config::Distribution::BLUEPRINT:
    {
      return Error("E::Blueprint does not support recipes");
    }
    break;
    case ns_config::Distribution::NONE:
    {
      return Error("E::Unsupported distribution '{}' for recipe installation", distribution);
    }
  }
  // Copy packages to arguments
  std::ranges::copy(packages, std::back_inserter(args));
  // Execute package manager using the provided callback
  return callback(program, args);
}

} // namespace ns_recipe

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
