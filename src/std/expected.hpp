/**
 * @file expected.hpp
 * @author Ruan Formigoni
 * @brief Wrappers to std::expected
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <expected>
#include <string>
#include <array>
#include <type_traits>

#include "string.hpp"
#include "../lib/log.hpp"

template<typename T, typename E = std::string>
struct Value : std::expected<T, E>
{
  using std::expected<T, E>::expected;
  
private:
  constexpr static std::array<const char[6], 5> const m_prefix{"D::{}", "I::{}", "W::{}", "E::{}", "C::{}"};

  template<ns_string::static_string fmt>
  constexpr static int select_prefix()
  {
    constexpr std::string_view sv{fmt.data};
    return sv.starts_with("D")? 0
      : sv.starts_with("I")? 1
      : sv.starts_with("W")? 2
      : sv.starts_with("E")? 3
      : 4;
  }
  
public:
  template<ns_string::static_string fmt = "Q::", typename... Args>
  void discard_impl(ns_log::Location const& loc, Args&&... args)
  {
    if (not this->has_value())
    {
      logger_loc(loc, m_prefix[select_prefix<fmt>()], this->error());
      logger_loc(loc, fmt.data, std::forward<Args>(args)...);
    }
  }

  template<ns_string::static_string fmt = "Q::", typename... Args>
  Value<T,E> forward_impl(ns_log::Location const& loc, Args&&... args)
  {
    if (not this->has_value())
    {
      logger_loc(loc, m_prefix[select_prefix<fmt>()], this->error());
      logger_loc(loc, fmt.data, std::forward<Args>(args)...);
      return std::unexpected(this->error());
    }
    return Value<T,E>(std::move(this->value()));
  }
  
  T or_default() requires std::default_initializable<T>
  {
    if (!this->has_value()) {
      return T{};
    }
    return std::move(**this);
  }
};

constexpr auto __expected_fn = [](auto&& e) { return e; };

#define NOPT(expr, ...) NOPT_IDENTITY(__VA_OPT__(NOPT_EAT) (expr))
#define NOPT_IDENTITY(...) __VA_ARGS__
#define NOPT_EAT(...)

#define Pop(expr,...)                                                                 \
({                                                                                    \
  auto __expected_ret = (expr);                                                       \
  if (!__expected_ret)                                                                \
  {                                                                                   \
    NOPT(ns_log::debug()("D::{}", __expected_ret.error()) __VA_OPT__(,) __VA_ARGS__); \
    __VA_OPT__(logger(__VA_ARGS__));                                                  \
    return __expected_fn(std::unexpected(std::move(__expected_ret).error()));         \
  }                                                                                   \
  std::move(__expected_ret).value();                                                  \
})

#define discard(fmt,...) discard_impl<fmt>(ns_log::Location() __VA_OPT__(,) __VA_ARGS__)
#define forward(fmt,...) forward_impl<fmt>(ns_log::Location() __VA_OPT__(,) __VA_ARGS__)


template<typename Fn>
auto __except_impl(Fn&& f) -> Value<std::invoke_result_t<Fn>>
{
  try
  {
    if constexpr (std::is_void_v<std::invoke_result_t<Fn>>)
    {
      f();
      return {};
    }
    else
    {
      return f();
    }
  }
  catch (std::exception const& e)
  {
    return std::unexpected(e.what());
  }
  catch (...)
  {
    return std::unexpected("Unknown exception was thrown");
  }
}

#define Try(expr,...) Pop(__except_impl([&]{ return (expr); }), __VA_ARGS__)

#define Catch(expr) (__except_impl([&]{ return (expr); }))


// Unexpect logs before creating std::unexpected
#define Error(fmt,...)                                 \
({                                                     \
  logger(fmt __VA_OPT__(,) __VA_ARGS__);               \
  [&](auto&&... __fmt_args)                            \
  {                                                    \
    if constexpr (sizeof...(__fmt_args) > 0)           \
    {                                                  \
      return std::unexpected(                          \
          std::format(std::string_view(fmt).substr(3)  \
        , ns_string::to_string(__fmt_args)...)         \
      );                                               \
    }                                                  \
    else                                               \
    {                                                  \
      return std::unexpected(                          \
        std::format(std::string_view(fmt).substr(3))   \
      );                                               \
    }                                                  \
  }(__VA_ARGS__);                                      \
})
