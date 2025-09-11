/**
 * @file macro.hpp
 * @author Ruan Formigoni
 * @brief Macros to simplify common patterns
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include "lib/log.hpp"

// Pop rust-style for std::expected
#define Expect(expr,...)                             \
({                                                   \
  auto __expected_ret = (expr);                      \
  if (!__expected_ret)                               \
  {                                                  \
    __VA_OPT__(ns_log::error()(__VA_ARGS__));        \
    return std::unexpected(__expected_ret.error());  \
  }                                                  \
  __expected_ret.value();                            \
})

// Expected wrapper
template<typename T>
using Expected = std::expected<T, std::string>;

// Unexpect logs before creating std::unexpected
#define Unexpected(...)                                                 \
  ([&](auto&& _fmt, auto&&... _args) {                                  \
    std::string _msg;                                                   \
    if constexpr (sizeof...(_args) > 0) {                               \
      _msg = std::vformat(                                              \
        std::forward<decltype(_fmt)>(_fmt),                             \
        std::make_format_args(std::forward<decltype(_args)>(_args)...)  \
      );                                                                \
    } else {                                                            \
      _msg = std::string(std::forward<decltype(_fmt)>(_fmt));           \
    }                                                                   \
    ns_log::error()(_msg);                                              \
    return std::unexpected(std::move(_msg));                            \
  }(__VA_ARGS__))

// Expected or default-constructed
template<typename T>
T ExpectedOrDefault(Expected<T> expected)
{
  if(expected) { return expected.value(); }
  return T{};
}

#define expect_map_error(expr, fun)                  \
({                                                   \
  auto __expected_ret = (expr);                      \
  if (!__expected_ret)                               \
    return fun(__expected_ret.error());              \
  __expected_ret.value();                            \
})

// Throw
#define qthrow_if(cond, msg) \
  if (cond) { throw std::runtime_error(msg); }

#define dthrow_if(cond, msg) \
  if (cond) { ns_log::debug()(msg); throw std::runtime_error(msg); }

#define ithrow_if(cond, msg) \
  if (cond) { ns_log::info()(msg); throw std::runtime_error(msg); }

#define ethrow_if(cond, msg) \
  if (cond) { ns_log::error()(msg); throw std::runtime_error(msg); }

// Exit
#define qexit_if(cond, ret) \
  if (cond) { exit(ret); }

#define dexit_if(cond, msg, ret) \
  if (cond) { ns_log::debug()(msg); exit(ret); }

#define iexit_if(cond, msg, ret) \
  if (cond) { ns_log::info()(msg); exit(ret); }

#define eexit_if(cond, msg, ret) \
  if (cond) { ns_log::error()(msg); exit(ret); }

// Return
#define qreturn_if(cond, ...) \
  if (cond) { return __VA_ARGS__; }

#define dreturn_if(cond, msg, ...) \
  if (cond) { ns_log::debug()(msg); return __VA_ARGS__; }

#define ireturn_if(cond, msg, ...) \
  if (cond) { ns_log::info()(msg); return __VA_ARGS__; }

#define ereturn_if(cond, msg, ...) \
  if (cond) { ns_log::error()(msg); return __VA_ARGS__; }

#define return_if_else(cond, val1, val2) \
  if (cond) { return val1; } else { return val2; }

// Break
#define qbreak_if(cond) \
  if (cond) { break; }

#define ebreak_if(cond, msg) \
  if (cond) { ns_log::error()(msg); break; }

#define ibreak_if(cond, msg) \
  if (cond) { ns_log::info()(msg); break; }

#define dbreak_if(cond, msg) \
  if (cond) { ns_log::debug()(msg); break; }

// Continue
#define qcontinue_if(cond) \
  if (cond) { continue; }

#define econtinue_if(cond, msg) \
  if (cond) { ns_log::error()(msg); continue; }

#define icontinue_if(cond, msg) \
  if (cond) { ns_log::info()(msg); continue; }

#define dcontinue_if(cond, msg) \
  if (cond) { ns_log::debug()(msg); continue; }

// Exit from child
#define e_exitif(cond, msg, code) \
  if (cond) { ns_log::error()(msg); _exit(code); }

// Conditional log
#define elog_if(cond, msg) \
  if (cond) { ns_log::error()(msg); }

#define wlog_if(cond, msg) \
  if (cond) { ns_log::warn()(msg); }

#define ilog_if(cond, msg) \
  if (cond) { ns_log::info()(msg); }

#define dlog_if(cond, msg) \
  if (cond) { ns_log::debug()(msg); }

// Log expected error
template<typename U>
void elog_expected(std::string_view msg, Expected<U> const& expected)
{
  if (not expected)
  {
    ns_log::error()(msg, expected.error());
  }
}