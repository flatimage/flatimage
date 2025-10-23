/**
 * @file match.hpp
 * @author Ruan Formigoni
 * @brief A library for object matching in C++
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <functional>
#include <expected>

#include "../std/concept.hpp"
#include "../std/expected.hpp"

namespace ns_match
{

template<typename Comp, typename... Args>
requires std::is_default_constructible_v<Comp>
class lhs_comparator
{
  private:
    std::tuple<std::decay_t<Args>...> m_tuple;
  public:
    // Constructor
    lhs_comparator(Args&&... args)
      : m_tuple(std::forward<Args>(args)...)
    {}
    // Comparison
    template<typename U>
    requires ( std::predicate<Comp,U,Args> and ... )
    bool operator()(U&& u) const noexcept
    {
      return std::apply([&](auto&&... e){ return (Comp{}(e, u) or ...); }, m_tuple);
    } // operator()
};

template<typename... Args>
requires ns_concept::Uniform<std::remove_cvref_t<Args>...>
  and (not ns_concept::SameAs<char const*, std::decay_t<Args>...>)
[[nodiscard]] decltype(auto) equal(Args&&... args) noexcept
{
  return lhs_comparator<std::equal_to<>,Args...>(std::forward<Args>(args)...);
};

template<typename... Args>
requires ns_concept::SameAs<char const*, std::decay_t<Args>...>
[[nodiscard]] decltype(auto) equal(Args&&... args) noexcept
{
  auto to_string = [](auto&& e){ return std::string(e); };
  return lhs_comparator<std::equal_to<>, std::invoke_result_t<decltype(to_string), Args>...>(std::string(args)...);
};

template<typename... T, typename U>
[[nodiscard]] decltype(auto) operator>>=(lhs_comparator<T...> const& partial_comp, U const& rhs) noexcept
{
  return [&](auto const& e)
  {
    if constexpr ( std::regular_invocable<U> )
    {
      if constexpr ( std::is_void_v<std::invoke_result_t<U>>  )
      {
        return (partial_comp(e))? (rhs(), Expected<void>{}) : std::unexpected("No match void");
      } // if
      else
      {
        return (partial_comp(e))? Expected<std::invoke_result_t<U>>(rhs()) : std::unexpected("No match invocable");
      } // else
    } // if
    else
    {
      return (partial_comp(e))? Expected<U>(rhs) : std::unexpected("No match type");
    } // else if
  };
}

/**
 * @brief Matches the input object with one of the instances of args
 * 
 * @param t The object to match
 * @param Args The possible matches for the object
 */
template<typename T, typename... Args>
requires ( sizeof...(Args) > 0 )
and ( std::is_invocable_v<Args,T> and ... )
and ( ns_concept::IsInstanceOf<std::invoke_result_t<Args,T>, Expected> and ... )
[[nodiscard]] auto match(T&& t, Args&&... args) noexcept
  -> Expected<typename std::invoke_result_t<std::tuple_element_t<0,std::tuple<Args...>>, T>::value_type>
{
  using Type = typename std::invoke_result_t<std::tuple_element_t<0,std::tuple<Args...>>, T>::value_type;
  
  std::string msg_error;
  if constexpr (ns_concept::StringRepresentable<T>)
  {
    msg_error = "No match for '{}'"_fmt(ns_string::to_string(t));
  }
  else
  {
    msg_error = "No matched value";
  }

  if constexpr (std::is_void_v<Type>)
  {
    return (args(t) || ...)? Expected<Type>{} : std::unexpected(msg_error);
  }
  else
  {
    auto f_check = [](auto&& _t, auto&& _args, auto& _out)
    {
      auto expected_result = _args(_t);
      if(expected_result) { _out = expected_result.value(); }
      return expected_result;
    };
    if(Type result; (f_check(t, std::forward<Args>(args), result) || ...))
    {
      return result;
    }
    return Unexpected("E::{}", msg_error);
  }
}

} // namespace ns_match

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/