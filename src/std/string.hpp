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

template<size_t N>
struct static_string
{
  char data[N];
  // Functions
  constexpr static_string(const char (&str)[N])
  {
    std::copy_n(str, N, data);
  }
  constexpr static_string() = default;
  constexpr operator const char*() const { return data; }
  constexpr operator std::string_view() const { return std::string_view(data, N - 1); }
  constexpr size_t size() const { return N - 1; }
  // Concatenate this string with other static_strings
  // Use 'auto' to accept any static_string<M> as NTTP
  template <auto... Strs>
  constexpr auto join() const noexcept
  {
    // Total string length
    constexpr std::size_t total_len = (N - 1) + (Strs.size() + ... + 0);
    // Result of the operation
    static_string<total_len + 1> result{};
    // Copy the member string
    std::copy_n(data, N, result.data);
    // Copy all other strings
    auto append = [i=N-1, &result](auto const& s) mutable
    {
      std::copy_n(s.data, s.size(), result.data+i);
      i += s.size();
    };
    (append(Strs), ...);
    // String terminator
    result.data[total_len] = '\0';
    return result;
  }
};

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

/**
 * @brief Replace all occurrences of "{}" placeholders in a string with provided values
 *
 * This function iterates through the string and replaces "{}" placeholders with
 * the provided arguments in order.
 *
 * @tparam Args The types of the arguments to replace
 * @param str The string containing "{}" placeholders
 * @param args The values to replace placeholders with
 * @return The string with placeholders replaced
 */
template<typename... Args>
[[nodiscard]] inline std::string placeholders_replace(std::string str, Args&&... args)
{
  std::vector<std::string> replacements;
  // Convert all arguments to strings
  ((replacements.push_back(to_string(std::forward<Args>(args)))), ...);

  // Replace all occurrences of "{}" with the replacements in order
  std::size_t arg_index = 0;
  while (arg_index < replacements.size())
  {
    size_t pos = str.find("{}");
    if (pos == std::string::npos) break;
    str.replace(pos, 2, replacements[arg_index++]);
  }
  return str;
}

} // namespace ns_string

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
