///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : env
///

#pragma once

#include <cstdlib>
#include <wordexp.h>

#include "../std/string.hpp"
#include "../common.hpp"
#include "../macro.hpp"
#include "log.hpp"

// Environment variable handling {{{
namespace ns_env
{

// enum class Replace {{{
enum class Replace
{
  Y,
  N,
}; // enum class Replace }}}

// set() {{{
// Sets an environment variable
template<ns_concept::StringRepresentable T, ns_concept::StringRepresentable U>
void set(T&& name, U&& value, Replace replace)
{
  // ns_log::debug()("ENV: {} -> {}", name , value);
  setenv(ns_string::to_string(name).c_str(), ns_string::to_string(value).c_str(), (replace == Replace::Y));
} // set() }}}

// prepend() {{{
// Prepends 'extra' to an environment variable 'name'
inline void prepend(const char* name, std::string const& extra)
{
  // Append to var
  if ( const char* var_curr = std::getenv(name); var_curr )
  {
    // ns_log::debug()("ENV: {} -> {}", name, extra + var_curr);
    setenv(name, std::string{extra + var_curr}.c_str(), 1);
  } // if
  else
  {
    ns_log::error()("Variable '{}' is not set"_fmt(name));
  } // else
} // prepend() }}}

// print() {{{
// print an env variable
inline void print(const char* name, std::ostream& os = std::cout)
{
  if ( const char* var = std::getenv(name); var )
  {
    os << var;
  } // if
} // print() }}}

// get_or_throw() {{{
// Get an env variable
template<typename T = const char*>
inline T get_or_throw(const char* name)
{
  const char* value = std::getenv(name);
  ethrow_if(not value, "Variable '{}' is undefined"_fmt(name));
  return value;
} // get_or_throw() }}}

// get_or_else() {{{
// Get an env variable
inline std::string get_or_else(std::string_view name, std::string_view alternative)
{
  const char* value = std::getenv(name.data());
  return_if_else(value != nullptr, value, alternative.data());
} // get_or_else() }}}

// get() {{{
// Get an env variable
inline const char* get(const char* name)
{
  return std::getenv(name);
} // get() }}}

// get_optional() {{{
// get_optional an env variable
template<typename T = std::string_view>
inline std::optional<T> get_optional(std::string_view name)
{
  const char* var = std::getenv(name.data());
  return (var)? std::make_optional(var) : std::nullopt;
} // get_optional() }}}

// get_expected() {{{
// Get an env variable
inline Expected<std::string_view> get_expected(const char* name)
{
  const char * var = std::getenv(name);
  return (var != nullptr)?
      Expected<std::string_view>(var)
    : Unexpected("Could not read variable '{}'"_fmt(name));
} // get_expected() }}}

// exists() {{{
// Checks if variable exists
inline bool exists(const char* var)
{
  return get(var) != nullptr;
} // exists() }}}

// exists() {{{
// Checks if variable exists and equals value
inline bool exists(const char* var, std::string_view target)
{
  const char* value = get(var);
  qreturn_if(not value, false);
  return std::string_view{value} == target;
} // exists() }}}

// expand() {{{
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
} // expand() }}}

// xdg_data_home() {{{
template<typename T = std::string_view>
Expected<T> xdg_data_home() noexcept
{
  const char* var = std::getenv("XDG_DATA_HOME");
  qreturn_if(var, var);
  const char* home = std::getenv("HOME");
  qreturn_if(not home, Unexpected("HOME is undefined"));
  return std::string{home} + "/.local/share";
} // xdg_data_home() }}}

} // namespace ns_env }}}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
