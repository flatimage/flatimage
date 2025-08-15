/**
 * @file env.hpp
 * @author Ruan Formigoni
 * @brief A library for manipulating environment variables
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdlib>
#include <wordexp.h>
#include <ranges>

#include "../std/string.hpp"
#include "../common.hpp"
#include "../macro.hpp"

namespace ns_env
{
  
namespace
{
namespace fs = std::filesystem;
}

enum class Replace
{
  Y,
  N,
};

/**
 * @brief Sets an environment variable
 * 
 * @tparam T StringRepresentable
 * @tparam U StringRepresentable
 * @param name Variable name
 * @param value Variable value
 * @param replace Should it be replace if it exists?
 */
template<ns_concept::StringRepresentable T, ns_concept::StringRepresentable U>
void set(T&& name, U&& value, Replace replace)
{
  setenv(ns_string::to_string(name).c_str(), ns_string::to_string(value).c_str(), (replace == Replace::Y));
}

/**
 * @brief Get the value of an environment variable or throw if undefined
 * 
 * @param name The name of the environment variable
 * @return T The value of the environment variable or the respective error
 */
template<typename T = const char*>
inline T get_or_throw(const char* name)
{
  const char* value = std::getenv(name);
  ethrow_if(not value, "Variable '{}' is undefined"_fmt(name));
  return value;
}

/**
 * @brief Get the value of an environment variable
 * 
 * @param name The name of the variable
 * @return Expected<std::string> The value of the variable or the respective error
 */
inline Expected<std::string> get_expected(std::string_view name)
{
  const char * var = std::getenv(name.data());
  return (var != nullptr)?
      Expected<std::string>(var)
    : Unexpected("Could not read variable '{}'"_fmt(name));
}

/**
 * @brief Checks if variable exists and equals value
 * 
 * @param name Name of the variable
 * @param value Expected value of the variable
 * @return True if it exists and matches the expected value, or false otherwise
 */
inline bool exists(std::string_view name, std::string_view value)
{
  const char* value_real = getenv(name.data());
  qreturn_if(not value_real, false);
  return std::string_view{value_real} == value;
}

/**
 * @brief Performs variable expansion analogous to a POSIX shell
 * 
 * @param var Source string to expand
 * @return Expected<std::string> The expanded value or the respective error
 */
inline Expected<std::string> expand(ns_concept::StringRepresentable auto&& var)
{
  std::string expanded = ns_string::to_string(var);

  // Perform word expansion
  wordexp_t data;
  if (int ret = wordexp(expanded.c_str(), &data, 0); ret == 0)
  {
    if (data.we_wordc > 0)
    {
      expanded = data.we_wordv[0];
    } // if
    wordfree(&data);
  } // if
  else
  {
    std::string error;
    switch(ret)
    {
      case WRDE_BADCHAR: error = "WRDE_BADCHAR"; break;
      case WRDE_BADVAL: error = "WRDE_BADVAL"; break;
      case WRDE_CMDSUB: error = "WRDE_CMDSUB"; break;
      case WRDE_NOSPACE: error = "WRDE_NOSPACE"; break;
      case WRDE_SYNTAX: error = "WRDE_SYNTAX"; break;
      default: error = "unknown";
    } // switch
    return Unexpected(error);
  } // else

  return expanded;
}

/**
 * @brief Returns or computes the value of XDG_DATA_HOME
 * 
  * @tparam T The return type, defaults to std::string
 * @return Expected<T> The path to XDG_DATA_HOME or the respective error
 */
template<typename T = std::string>
inline Expected<T> xdg_data_home() noexcept
{
  const char* var = std::getenv("XDG_DATA_HOME");
  qreturn_if(var, var);
  const char* home = std::getenv("HOME");
  qreturn_if(not home, Unexpected("HOME is undefined"));
  return std::string{home} + "/.local/share";
}

/**
 * @brief Search the directories in the PATH variable for the given input file name
 * 
 * @param query The file name to search for in PATH directories
 * @return Expected<fs::path> The path of the found file or the respective error
 */
[[nodiscard]] inline Expected<fs::path> search_path(std::string const& query)
{
  std::string env_path = Expect(ns_env::get_expected("PATH"));
  // Avoid searching in these directories in they exist in PATH
  Expected<fs::path> env_dir_global_bin = ns_env::get_expected("FIM_DIR_GLOBAL_BIN");
  Expected<fs::path> env_dir_static = ns_env::get_expected("FIM_DIR_STATIC");
  // Query should be a file name
  if ( fs::path{query}.is_absolute() )
  {
    return Unexpected("Query should be a file name, not an absolute path");
  }
  // Search directories in PATH
  for(fs::path directory : env_path
    | std::views::split(':')
    | std::ranges::to<std::vector<std::string>>())
  {
    qcontinue_if(env_dir_global_bin && directory == env_dir_global_bin.value());
    qcontinue_if(env_dir_static && directory == env_dir_static.value());
    fs::path path_full = directory / query;
    qreturn_if(fs::exists(path_full), path_full);
  }
  return Unexpected("File not found in PATH");
}

} // namespace ns_env

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
