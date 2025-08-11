///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : match
///

#pragma once

#include <functional>
#include <expected>

#include "../std/concept.hpp"
#include "../macro.hpp"

namespace ns_match
{

// class lhs_comparator {{{
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
}; // }}}

// fn: equal() {{{
template<typename... Args>
requires ns_concept::Uniform<std::remove_cvref_t<Args>...>
  and (not ns_concept::SameAs<char const*, std::decay_t<Args>...>)
[[nodiscard]] decltype(auto) equal(Args&&... args) noexcept
{
  return lhs_comparator<std::equal_to<>,Args...>(std::forward<Args>(args)...);
}; // fn: equal() }}}

// fn: equal() {{{
template<typename... Args>
requires ns_concept::SameAs<char const*, std::decay_t<Args>...>
[[nodiscard]] decltype(auto) equal(Args&&... args) noexcept
{
  auto to_string = [](auto&& e){ return std::string(e); };
  return lhs_comparator<std::equal_to<>, std::invoke_result_t<decltype(to_string), Args>...>(std::string(args)...);
}; // fn: equal() }}}

// operator>>= {{{
template<typename... T, typename U>
[[nodiscard]] decltype(auto) operator>>=(lhs_comparator<T...> const& partial_comp, U const& rhs) noexcept
{
  // Make it lazy
  return [&](auto const& e)
  {
    if constexpr ( std::regular_invocable<U> )
    {
      if constexpr ( std::is_void_v<std::invoke_result_t<U>>  )
      {
        return (partial_comp(e))? (rhs(), Expected<void>{}) : Unexpected("");
      } // if
      else
      {
        return (partial_comp(e))? Expected<std::invoke_result_t<U>>(rhs()) : Unexpected("");
      } // else
    } // if
    else
    {
      return (partial_comp(e))? Expected<U>(rhs) : Unexpected("");
    } // else if
  };
} // }}}

// match() {{{
template<typename T, typename... Args>
requires ( sizeof...(Args) > 0 )
and ( std::is_invocable_v<Args,T> and ... )
and ( ns_concept::IsInstanceOf<std::invoke_result_t<Args,T>, std::expected> and ... )
[[nodiscard]] auto match(T&& t, Args&&... args) noexcept
  -> Expected<typename std::invoke_result_t<std::tuple_element_t<0,std::tuple<Args...>>, T>::value_type>
{
  Expected<typename std::invoke_result_t<std::tuple_element_t<0,std::tuple<Args...>>, T>::value_type> result;

  // Use fold expression to evaluate each argument
  if(not ((result = args(t), result.has_value()) || ...))
  {
    result = Unexpected("No match value match for type");
  }
  
  return result;
} // match() }}}

} // namespace ns_match

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
