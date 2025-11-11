/**
 * @file macro.hpp
 * @author Ruan Formigoni
 * @brief Simplified macros for common control flow patterns with optional logging
 * 
 * This file provides a minimal set of preprocessor macros designed to reduce
 * boilerplate code by combining conditional statements with actions like return,
 * break, and continue. All macros support optional logging by including a format
 * string and arguments after the action.
 * 
 * The logging level is determined by the prefix in the format string itself
 * (D::, I::, W::, E::, C::) which is processed by the logger at compile time.
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include "lib/log.hpp"

/**
 * @brief Conditionally return from function with optional logging
 * 
 * Evaluates a condition and if true, optionally logs a message and returns
 * the specified value. For void functions, use 'void' as the return value.
 * 
 * @param condition The condition to evaluate
 * @param value The value to return (use ',,' for void functions)
 * @param ... Optional format string and arguments for logging
 * 
 * @example
 * @code
 * // Void function with logging
 * return_if(ptr == nullptr,,"E::Null pointer encountered");
 * 
 * // Return with value and logging
 * return_if(error_code != 0, -1, "E::Operation failed with code {}", error_code);
 * 
 * // Return without logging
 * return_if(done, result);
 * @endcode
 */
#define return_if(condition, value, ...) \
  if (condition) { \
    __VA_OPT__(logger(__VA_ARGS__);) \
    return value; \
  }

/**
 * @brief Conditionally break from loop with optional logging
 * 
 * Evaluates a condition and if true, optionally logs a message and breaks
 * from the current loop.
 * 
 * @param condition The condition to evaluate
 * @param ... Optional format string and arguments for logging
 * 
 * @example
 * @code
 * for(auto& item : items) {
 *   break_if(item.is_invalid(), "E::Invalid item found: {}", item.name());
 * }
 * @endcode
 */
#define break_if(condition, ...) \
  if (condition) { \
    __VA_OPT__(logger(__VA_ARGS__);) \
    break; \
  }

/**
 * @brief Conditionally continue to next iteration with optional logging
 * 
 * Evaluates a condition and if true, optionally logs a message and continues
 * to the next iteration of the current loop.
 * 
 * @param condition The condition to evaluate
 * @param ... Optional format string and arguments for logging
 * 
 * @example
 * @code
 * for(auto& file : files) {
 *   continue_if(file.is_hidden(), "I::Skipping hidden file: {}", file.name());
 * }
 * @endcode
 */
#define continue_if(condition, ...) \
  if (condition) { \
    __VA_OPT__(logger(__VA_ARGS__);) \
    continue; \
  }

/**
 * @brief Conditionally log a message without affecting control flow
 * 
 * Evaluates a condition and if true, logs the specified message.
 * Program execution continues normally after logging.
 * 
 * @param condition The condition to evaluate
 * @param ... Format string and arguments for logging
 * 
 * @note The log level is determined by the prefix in the format string:
 *       - D:: for debug
 *       - I:: for info  
 *       - W:: for warning
 *       - E:: for error
 *       - C:: for critical
 * 
 * @example
 * @code
 * log_if(threshold_exceeded, "W::Threshold exceeded: {} > {}", value, max);
 * log_if(debug_mode, "D::Current state: {}", state);
 * @endcode
 */
#define log_if(condition, ...) \
  if (condition) { \
    logger(__VA_ARGS__); \
  }

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/