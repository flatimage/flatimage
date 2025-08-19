/**
 * @file string.hpp
 * @author Ruan Formigoni
 * @brief String helpers
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <algorithm>
#include <sstream>
#include <format>
#include <expected>

#include "concept.hpp"

namespace ns_string
{

/**
 * @brief Converts a type to a string
 * 
 * @tparam T A string representable type
 * @param t The value to convert to a string
 * @return std::string The type string representation
 */
template<typename T>
[[nodiscard]] inline std::string to_string(T&& t) noexcept
{
  if constexpr ( ns_concept::StringConvertible<T> )
  {
    return t;
  } // if
  else if constexpr ( ns_concept::StringConstructible<T> )
  {
    return std::string{t};
  } // else if
  else if constexpr ( ns_concept::Numeric<T> )
  {
    return std::to_string(t);
  } // else if 
  else if constexpr ( ns_concept::StreamInsertable<T> )
  {
    std::stringstream ss;
    ss << t;
    return ss.str();
  } // else if 
  else if constexpr ( ns_concept::IterableConst<T> )
  {
    std::stringstream ss;
    ss << '[';
    std::for_each(t.cbegin(), t.cend(), [&](auto&& e){ ss << std::format("'{}',", e); });
    ss << ']';
    return ss.str();
  } // else if 
  else
  {
    static_assert(false, "Cannot convert type to string");
  }
}

/**
 * @brief Converts a container into a string if it has string convertible elements
 * 
 * @tparam T A container type
 * @param t The container to convert to a string
 * @param sep The separator to use between elements of the container
 * @return std::string The result of the conversion operation
 */
template<typename T>
[[nodiscard]] std::string from_container(T&& t, std::optional<char> sep = std::nullopt) noexcept
{
  std::stringstream ret;
  for( auto it = t.begin(); it != t.end(); ++it )
  {
    ret << *it;
    if ( std::next(it) != t.end() and sep ) { ret << *sep; }
  } // if
  return ret.str();
}

/**
 * @brief Converts a container into a string if it has string convertible elements
 * 
 * @tparam T A container type
 * @param begin The begin iterator of the container to convert
 * @param end The end iterator of the container to convert
 * @param sep The separator to use between elements of the container
 * @return std::string The result of the conversion operation
 */
template<std::input_iterator It>
[[nodiscard]] std::string from_container(It&& begin, It&& end, std::optional<char> sep = std::nullopt) noexcept
{
  return from_container(std::ranges::subrange(begin, end), sep);
}

} // namespace ns_string

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
