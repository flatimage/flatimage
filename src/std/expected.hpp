/**
 * @file expected.hpp
 * @author Ruan Formigoni
 * @brief Enhanced error handling framework built on std::expected
 *
 * This module provides a sophisticated error handling system that extends C++23's
 * std::expected with integrated logging capabilities, convenient macros for error
 * propagation, and utilities for exception handling.
 *
 * Key Components:
 * - Value<T,E>: Enhanced std::expected with logging
 * - Pop macro: Unwrap-or-return with logging
 * - Try/Catch macros: Exception-to-expected conversion
 * - Error macro: Create unexpected values with logging
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

/**
 * @brief Enhanced expected type with integrated logging capabilities
 *
 * Value extends std::expected to provide automatic error logging and convenient
 * error handling methods. It maintains full compatibility with std::expected
 * while adding FlatImage-specific functionality.
 *
 * @tparam T The expected value type (can be void)
 * @tparam E The error type (defaults to std::string)
 *
 * @note Inherits all std::expected constructors and methods
 * @see std::expected
 */
template<typename T, typename E = std::string>
struct Value : std::expected<T, E>
{
  using std::expected<T, E>::expected;

private:
  /**
   * @brief Log level prefixes for error messages
   *
   * Maps to: Debug, Info, Warning, Error, Critical
   */
  constexpr static std::array<const char[6], 5> const m_prefix{"D::{}", "I::{}", "W::{}", "E::{}", "C::{}"};

  /**
   * @brief Select appropriate log prefix based on format string
   *
   * @tparam fmt Format string starting with log level indicator
   * @return Index into m_prefix array (0-4)
   */
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
  /**
   * @brief Log error and discard it if present
   *
   * If this Value contains an error, logs it at the specified level
   * along with any additional context, then continues execution.
   *
   * @tparam fmt Format string with log level prefix (D::, I::, W::, E::, C::)
   * @tparam Args Types of additional format arguments
   * @param loc Source location for logging
   * @param args Additional arguments for the format string
   *
   * @note Used internally by the discard() macro
   */
  template<ns_string::static_string fmt = "Q::", typename... Args>
  void discard_impl(ns_log::Location const& loc, Args&&... args)
  {
    if (not this->has_value())
    {
      logger_loc(loc, m_prefix[select_prefix<fmt>()], this->error());
      logger_loc(loc, fmt.data, std::forward<Args>(args)...);
    }
  }

  /**
   * @brief Forward error with logging or return value
   *
   * If this Value contains an error, logs it and returns a new Value
   * with the same error. Otherwise, returns a Value with the contained value.
   *
   * @tparam fmt Format string with log level prefix
   * @tparam Args Types of additional format arguments
   * @param loc Source location for logging
   * @param args Additional arguments for the format string
   * @return Value<T,E> Forwarded Value with same state
   *
   * @note Used internally by the forward() macro
   */
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

  /**
   * @brief Get value or default-constructed T
   *
   * @return T The contained value if present, otherwise T{}
   * @requires T must be default constructible
   *
   * @example
   * @code
   * Value<int> result = get_value();
   * int value = result.or_default();  // Returns 0 if error
   * @endcode
   */
  T or_default() requires std::default_initializable<T>
  {
    if (!this->has_value()) {
      return T{};
    }
    return std::move(**this);
  }
};

/**
 * @brief Lambda helper for Pop macro error returns
 * @internal
 */
constexpr auto __expected_fn = [](auto&& e) { return e; };

/**
 * @internal
 * @brief Helper macros for optional argument handling
 * @{
 */
#define NOPT(expr, ...) NOPT_IDENTITY(__VA_OPT__(NOPT_EAT) (expr))
#define NOPT_IDENTITY(...) __VA_ARGS__
#define NOPT_EAT(...)
/** @} */

/**
 * @brief Unwrap Value or return with error logging
 *
 * This macro evaluates an expression returning a Value/expected type.
 * If it contains a value, extracts and returns it. If it contains an
 * error, logs the error and returns from the current function with
 * an unexpected containing the error.
 *
 * @param expr Expression returning Value<T> or std::expected<T,E>
 * @param ... Optional additional log message and arguments
 *
 * @example
 * @code
 * Value<int> get_value();
 *
 * Value<void> process() {
 *   int val = Pop(get_value());  // Returns on error
 *   // val is guaranteed valid here
 *   return {};
 * }
 *
 * // With additional context:
 * int val2 = Pop(get_value(), "Failed to get value for processing");
 * @endcode
 */
#define Pop(expr,...)                                                         \
({                                                                            \
  auto __expected_ret = (expr);                                               \
  if (!__expected_ret)                                                        \
  {                                                                           \
    NOPT(logger("D::{}", __expected_ret.error()) __VA_OPT__(,) __VA_ARGS__);  \
    __VA_OPT__(logger(__VA_ARGS__));                                          \
    return __expected_fn(std::unexpected(std::move(__expected_ret).error())); \
  }                                                                           \
  std::move(__expected_ret).value();                                          \
})

/**
 * @brief Discard error with logging
 *
 * @param fmt Format string with log level prefix (e.g., "E::Error occurred")
 * @param ... Additional format arguments
 *
 * @example
 * @code
 * get_value().discard("W::Value not available");
 * @endcode
 */
#define discard(fmt,...) discard_impl<fmt>(ns_log::Location() __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Forward error with additional context
 *
 * @param fmt Format string with log level prefix
 * @param ... Additional format arguments
 * @return The same Value with logged context
 *
 * @example
 * @code
 * return get_value().forward("E::Failed in processing step");
 * @endcode
 */
#define forward(fmt,...) forward_impl<fmt>(ns_log::Location() __VA_OPT__(,) __VA_ARGS__)


/**
 * @brief Convert exceptions to Value errors
 *
 * Executes a callable and catches any exceptions, converting them
 * to Value errors with appropriate logging.
 *
 * @tparam Fn Callable type
 * @param f Function to execute with exception handling
 * @return Value containing result or error message
 *
 * @internal Used by Try and Catch macros
 */
template<typename Fn>
auto __except_impl(ns_log::Location const& loc, Fn&& f) -> Value<std::invoke_result_t<Fn>>
  requires (not ns_concept::IsInstanceOf<std::invoke_result_t<Fn>, Value>)
  and (not ns_concept::IsInstanceOf<std::invoke_result_t<Fn>, std::expected>)
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
    logger_loc(loc, "E::{}::Exception was thrown", e.what());
    return std::unexpected(e.what());
  }
  catch (...)
  {
    logger_loc(loc, "E::{}::Unknown exception was thrown");
    return std::unexpected("Unknown exception was thrown");
  }
}

/**
 * @brief Execute expression with exception handling and unwrapping
 *
 * Wraps expression in exception handler, converts exceptions to errors,
 * then unwraps the result (returning on error).
 *
 * @param expr Expression that might throw
 * @param ... Optional additional error context
 *
 * @example
 * @code
 * auto result = Try(risky_operation());
 * // Exceptions become errors, errors cause return
 * @endcode
 */
#define Try(expr,...) Pop(__except_impl(::ns_log::Location(), [&]{ return (expr); }), __VA_ARGS__)

/**
 * @brief Execute expression with exception handling
 *
 * Like Try but doesn't unwrap - returns Value<T> directly
 *
 * @param expr Expression that might throw
 * @return Value<T> containing result or error
 *
 * @example
 * @code
 * auto result = Catch(might_throw());
 * if (!result)  handle error
 * @endcode
 */
#define Catch(expr) (__except_impl(::ns_log::Location(), [&]{ return (expr); }))


/**
 * @brief Create an unexpected error with logging
 *
 * Creates a std::unexpected value while simultaneously logging the error.
 * The format string should start with a 3-character log level prefix
 * (e.g., "E::") which is stripped from the error message but used for logging.
 *
 * @param fmt Format string starting with log level prefix
 * @param ... Format arguments
 * @return std::unexpected containing formatted error message
 *
 * @example
 * @code
 * Value<int> divide(int a, int b) {
 *   if (b == 0) {
 *     return Error("E::Division by zero: {} / {}", a, b);
 *   }
 *   return a / b;
 * }
 * @endcode
 */
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
