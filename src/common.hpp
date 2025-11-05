/**
 * @file common.hpp
 * @author Ruan Formigoni
 * @brief Commonly used helpers
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <format>
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

/**
 * @brief Multiplies the value by 1 kibibyte (1024 bytes)
 *
 * @param value The value to multiply with
 * @return unsigned long long The result of the operation (value * 1024)
 */
constexpr inline decltype(auto) operator ""_kib(unsigned long long value) noexcept
{
  return value * (1 << 10);
}

/**
 * @brief Multiplies the value by 1 mebibyte (1048576 bytes)
 *
 * @param value The value to multiply with
 * @return unsigned long long The result of the operation (value * 1048576)
 */
constexpr inline decltype(auto) operator ""_mib(unsigned long long value) noexcept
{
  return value * (1 << 20);
}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
