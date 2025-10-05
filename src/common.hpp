/**
 * @file common.hpp
 * @author Ruan Formigoni
 * @brief Commonly used helpers
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <format>
#include <iostream>
#include <cstdlib>
#include "std/string.hpp"

namespace
{

template<typename... Args>
struct format_args
{
  // Create a tuple where each type is the result of ns_string::to_string(Args...)
  std::tuple<decltype(ns_string::to_string(std::declval<Args>()))...> m_tuple_args;

  // Initializes the tuple elements to be string representations of the arguments
  format_args(Args&&... args)
    : m_tuple_args(ns_string::to_string(std::forward<Args>(args))...)
  {} // format_args

  // Get underlying arguments
  auto operator*()
  {
    return std::apply([](auto&&... e) { return std::make_format_args(e...); }, m_tuple_args);
  } // operator*
};

} // namespace

// Print to stdout
inline auto operator""_print(const char* c_str, std::size_t) noexcept
{
  return [=]<typename... Args>(Args&&... args)
  {
    std::cout << std::vformat(c_str, *format_args<Args...>(std::forward<Args>(args)...));
  };
}

/**
 * @brief Format strings with a user-defined literal
 * 
 * @param format Format of the string
 * @return decltype(auto) Formatter functor
 */
inline decltype(auto) operator ""_fmt(const char* format, size_t) noexcept
{
  return [format]<typename... Args>(Args&&... args)
  {
    return std::vformat(format, *format_args<Args...>(std::forward<Args>(args)...)) ;
  };
}

/**
 * @brief Multiplies the value by 1 kibibyte
 * 
 * @param value The value to multiply with
 * @return The result of the operation
 */
constexpr inline decltype(auto) operator ""_kib(unsigned long long value) noexcept
{
  return value * (1 << 10);
}

/**
 * @brief Multiplies the value by 1 mebibyte
 * 
 * @param value The value to multiply with
 * @return The result of the operation
 */
constexpr inline decltype(auto) operator ""_mib(unsigned long long value) noexcept
{
  return value * (1 << 20);
}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
