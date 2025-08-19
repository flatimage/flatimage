/**
 * @file vector.hpp
 * @author Ruan Formigoni
 * @brief Vector helpers
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <vector>

#include "concept.hpp"
#include "string.hpp"

namespace ns_vector
{

/**
 * @brief Appends a range of values from a src to a dst
 *
 * This is available in C++23, not implemented yet on gcc however
 *
 * @tparam R1 An iterable type
 * @tparam R2 An iterable type
 * @param to The target to push elements into
 * @param from The source to read elements from
 */
template<ns_concept::IterableConst R1, ns_concept::IterableConst R2>
inline void append_range(R1& to, R2 const& from) noexcept
{
  std::ranges::for_each(from, [&](auto&& e){ to.push_back(e); });
}

/**
 * @brief Helper to push back multiple elements into an input iterator
 * 
 * @tparam R An iterable type
 * @tparam Args Type of the arguments to push back
 * @param r The range to push elements into
 * @param args The arguments to push into the range
 */
template<ns_concept::IterableConst R, typename... Args>
inline void push_back(R& r, Args&&... args) noexcept
{
  ( r.push_back(std::forward<Args>(args)), ... );
}

/**
 * @brief Creates a range from a string
 * 
 * @tparam R A container type, defaults to std::vector<std::string>
 * @param t The string representable value to convert into a container
 * @param delimiter The delimiter to split elements in the string
 * @return R The container built from separating the string with the given delimiter
 */
template<ns_concept::Iterable R = std::vector<std::string>>
inline R from_string(ns_concept::StringRepresentable auto&& t, char delimiter) noexcept
{
  R tokens;
  std::string token;
  std::istringstream stream_token(ns_string::to_string(t));

  while (std::getline(stream_token, token, delimiter))
  {
    tokens.push_back(token);
  }

  return tokens;
}

} // namespace ns_vector

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
