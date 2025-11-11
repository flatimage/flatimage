/**
 * @file common.hpp
 * @author Ruan Formigoni
 * @brief Common utility functions and helpers used throughout FlatImage
 * 
 * This file provides common utilities including:
 * - Format argument conversion helpers for string formatting
 * - Binary unit literal operators for memory size calculations
 * 
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <format>
#include <cstdlib>
#include "std/string.hpp"

namespace
{

/**
 * @brief Helper struct for converting arguments to format-compatible strings
 * 
 * This template struct converts all provided arguments to their string representations
 * using ns_string::to_string() and stores them in a tuple. The converted strings can
 * then be used with std::format functions.
 * 
 * @tparam Args Variadic template parameters for the types to be converted
 * 
 */
template<typename... Args>
struct format_args
{
  /**
   * @brief Tuple storing string representations of all arguments
   * 
   * Each element type is determined by the return type of ns_string::to_string()
   * when called with the corresponding argument type
   */
  std::tuple<decltype(ns_string::to_string(std::declval<Args>()))...> m_tuple_args;

  /**
   * @brief Constructs format_args by converting all arguments to strings
   * 
   * @param args Arguments to be converted to string representations
   * @note Uses perfect forwarding to preserve value categories
   */
  format_args(Args&&... args)
    : m_tuple_args(ns_string::to_string(std::forward<Args>(args))...)
  {} // format_args

  /**
   * @brief Dereference operator to get format arguments
   * 
   * Applies std::make_format_args to the stored string tuple elements
   * 
   * @return Format arguments compatible with std::format functions
   */
  auto operator*()
  {
    return std::apply([](auto&&... e) { return std::make_format_args(e...); }, m_tuple_args);
  } // operator*
};

} // namespace

/**
 * @defgroup binary_literals Binary Unit Literal Operators
 * @brief User-defined literals for binary (base-2) memory units
 * 
 * These operators provide convenient syntax for expressing memory sizes
 * using binary units (powers of 1024) rather than decimal units (powers of 1000).
 * 
 * @{
 */

/**
 * @brief User-defined literal operator for kibibytes (KiB)
 * 
 * Converts a numeric literal to bytes by multiplying by 1024 (2^10).
 * A kibibyte is 1024 bytes, following the IEC 60027-2 standard for binary prefixes.
 *
 * @param value The numeric value in kibibytes
 * @return unsigned long long The equivalent value in bytes
 * 
 * @note This operator is constexpr, allowing compile-time evaluation
 * @note Uses bit shifting (1 << 10) for efficiency instead of multiplication
 * 
 */
constexpr inline decltype(auto) operator ""_kib(unsigned long long value) noexcept
{
  return value * (1 << 10);
}

/**
 * @brief User-defined literal operator for mebibytes (MiB)
 * 
 * Converts a numeric literal to bytes by multiplying by 1,048,576 (2^20).
 * A mebibyte is 1,048,576 bytes (1024 * 1024), following the IEC 60027-2 standard.
 *
 * @param value The numeric value in mebibytes
 * @return unsigned long long The equivalent value in bytes
 * 
 * @note This operator is constexpr, allowing compile-time evaluation
 * @note Uses bit shifting (1 << 20) for efficiency instead of multiplication
 */
constexpr inline decltype(auto) operator ""_mib(unsigned long long value) noexcept
{
  return value * (1 << 20);
}

/** @} */ // end of binary_literals group

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
