/**
 * @file macro.hpp
 * @author Ruan Formigoni
 * @brief Macros to simplify common patterns
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

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
