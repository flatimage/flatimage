/**
 * @file test_expected.cpp
 * @brief Unit tests for expected.hpp error handling utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../../src/lib/log.hpp"
#include "../../../src/std/expected.hpp"

namespace fs = std::filesystem;

// ============================================================================
// LOG CAPTURE HELPER
// ============================================================================

/**
 * @brief Helper class to capture log output to file for testing
 */
class LogCapture
{
private:
  fs::path m_log_path;
  ns_log::Level m_old_level;

public:
  LogCapture()
  {
    // Create unique temporary log file
    m_log_path = fs::temp_directory_path() / ("test_expected_" + std::to_string(getpid()) + ".log");

    // Save old level and set to DEBUG to capture everything
    m_old_level = ns_log::get_level();
    ns_log::set_level(ns_log::Level::DEBUG);

    // Set log sink to our test file
    ns_log::set_sink_file(m_log_path);
  }

  ~LogCapture()
  {
    // Restore old level and reset sink
    ns_log::set_level(m_old_level);
    ns_log::set_sink_file("/dev/null");

    // Clean up log file
    std::error_code ec;
    fs::remove(m_log_path, ec);
  }

  /**
   * @brief Read all log lines from the log file
   * @return Vector of log lines (trimmed)
   */
  std::vector<std::string> read_logs()
  {
    std::vector<std::string> lines;
    std::ifstream file(m_log_path);

    if (!file.is_open())
    {
      return lines;
    }

    std::string line;
    while (std::getline(file, line))
    {
      // Trim whitespace
      line.erase(0, line.find_first_not_of(" \t\r\n"));
      line.erase(line.find_last_not_of(" \t\r\n") + 1);

      if (!line.empty())
      {
        lines.push_back(line);
      }
    }

    return lines;
  }

  /**
   * @brief Extract "PREFIX::Message" from log line
   * Converts "PREFIX::filename::line::Message" to "PREFIX::Message"
   * @return String in format "PREFIX::Message" (e.g., "D::Operation failed")
   */
  static std::string extract_log_message(const std::string& log_line)
  {
    size_t pos = 0;
    // Get PREFIX
    size_t prefix_end = log_line.find("::", pos);
    if (prefix_end == std::string::npos)
      return "";
    std::string prefix = log_line.substr(0, prefix_end);

    pos = prefix_end + 2;
    // Skip filename::
    pos = log_line.find("::", pos);
    if (pos == std::string::npos)
      return "";
    pos += 2;
    // Skip line::
    pos = log_line.find("::", pos);
    if (pos == std::string::npos)
      return "";
    pos += 2;

    // Return PREFIX::Message
    return prefix + "::" + log_line.substr(pos);
  }
};

// ============================================================================
// TEST HELPER FUNCTIONS
// ============================================================================

// Test helper functions that return Value<T>
Value<int> success_function()
{
  return 42;
}

Value<int> error_function()
{
  return std::unexpected<std::string>("Error occurred");
}

Value<int> divide(int a, int b)
{
  if (b == 0)
  {
    return std::unexpected<std::string>("Division by zero");
  }
  return a / b;
}

TEST_CASE("Value<T> holds successful values")
{
  Value<int> result = success_function();

  REQUIRE(result.has_value());
  CHECK(result.value() == 42);
}

TEST_CASE("Value<T> holds error values")
{
  Value<int> result = error_function();

  REQUIRE_FALSE(result.has_value());
  CHECK(result.error() == "Error occurred");
}

TEST_CASE("Value<T>::or_default returns value when present")
{
  Value<int> result = success_function();

  CHECK(result.or_default() == 42);
}

TEST_CASE("Value<T>::or_default returns default when error")
{
  Value<int> result = error_function();

  CHECK(result.or_default() == 0); // int default is 0
}

TEST_CASE("Value<void> represents success without value")
{
  Value<void> result{};

  CHECK(result.has_value());
}

TEST_CASE("Value<void> can hold errors")
{
  Value<void> result = std::unexpected<std::string>("Operation failed");

  REQUIRE_FALSE(result.has_value());
  CHECK(result.error() == "Operation failed");
}

TEST_CASE("Value<T> works with custom error types")
{
  Value<int, int> int_error_result = std::unexpected<int>(404);

  REQUIRE_FALSE(int_error_result.has_value());
  CHECK(int_error_result.error() == 404);
}

TEST_CASE("Value chaining with expected operations")
{
  auto result1 = divide(10, 2);
  REQUIRE(result1.has_value());
  CHECK(result1.value() == 5);

  auto result2 = divide(10, 0);
  REQUIRE_FALSE(result2.has_value());
  CHECK(result2.error() == "Division by zero");
}

TEST_CASE("Value implicit conversions")
{
  Value<int> result = 100;

  REQUIRE(result.has_value());
  CHECK(*result == 100);
}

TEST_CASE("Value with different types")
{
  Value<std::string> str_result{"hello"};
  Value<double> double_result{3.14};
  Value<bool> bool_result{true};

  CHECK(str_result.has_value());
  CHECK(double_result.has_value());
  CHECK(bool_result.has_value());
  CHECK(str_result.value() == "hello");
  CHECK(double_result.value() == 3.14);
  CHECK(bool_result.value() == true);
}

// Test that Value properly inherits std::expected behavior
TEST_CASE("Value inherits std::expected interface")
{
  Value<int> success = 42;
  Value<int> failure = std::unexpected<std::string>("error");

  // Test has_value
  CHECK(success.has_value());
  CHECK_FALSE(failure.has_value());

  // Test value_or
  CHECK(success.value_or(0) == 42);
  CHECK(failure.value_or(99) == 99);
}

TEST_CASE("Value supports move semantics")
{
  Value<std::string> result{"moveable"};
  std::string extracted = std::move(result).value();

  CHECK(extracted == "moveable");
}

// ============================================================================
// MACRO TESTS
// ============================================================================

// Helper functions for Pop testing - these return Value<T>
Value<int> may_fail(bool should_fail)
{
  if (should_fail)
  {
    return std::unexpected<std::string>("Operation failed");
  }
  return 100;
}

Value<void> may_fail_void(bool should_fail)
{
  if (should_fail)
  {
    return std::unexpected<std::string>("Void operation failed");
  }
  return {};
}

// Helper functions for Try/Catch testing - these are exception-safe (non-Value returning)
int throws_exception()
{
  throw std::runtime_error("Exception thrown");
}

int returns_value()
{
  return 42;
}

void throws_void_exception()
{
  throw std::runtime_error("Void exception thrown");
}

void succeeds_void()
{
  // Do nothing, success
}

int risky_divide(int a, int b)
{
  if (b == 0)
  {
    throw std::runtime_error("Division by zero");
  }
  return a / b;
}

// ============================================================================
// Pop MACRO TESTS - Used with Value<T> returning functions
// ============================================================================

TEST_CASE("Pop macro extracts value from successful Value")
{
  auto test_func = []() -> Value<int>
  {
    auto result = may_fail(false);
    int value = Pop(result);
    return value;
  };

  auto final_result = test_func();
  REQUIRE(final_result.has_value());
  CHECK(final_result.value() == 100);
}

TEST_CASE("Pop macro propagates error on failure")
{
  LogCapture capture;

  auto test_func = []() -> Value<int>
  {
    auto result = may_fail(true);
    int value = Pop(result); // Should return early with error
    return value + 10;       // This line should not execute
  };

  auto final_result = test_func();
  REQUIRE_FALSE(final_result.has_value());
  CHECK(final_result.error() == "Operation failed");

  // Verify log stack: should only have DEBUG log of original error
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 1);
  CHECK(LogCapture::extract_log_message(logs[0]) == "D::Operation failed");
}

TEST_CASE("Pop macro with custom error message")
{
  LogCapture capture;

  auto test_func = []() -> Value<int>
  {
    auto result = may_fail(true);
    int value = Pop(result, "E::Custom error occurred");
    return value + 10;
  };

  auto final_result = test_func();
  REQUIRE_FALSE(final_result.has_value());
  // Pop replaces the error with custom message
  CHECK(final_result.error() == "Custom error occurred");

  // Verify log stack: DEBUG (original) then ERROR (replacement)
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 2);
  CHECK(LogCapture::extract_log_message(logs[0]) == "D::Operation failed");
  CHECK(LogCapture::extract_log_message(logs[1]) == "E::Custom error occurred");
}

TEST_CASE("Nested Pop macro calls")
{
  auto inner_func = []() -> Value<int>
  {
    auto r1 = may_fail(false);
    return Pop(r1) + 10;
  };

  auto outer_func = [&]() -> Value<int>
  {
    auto r2 = inner_func();
    return Pop(r2) * 2;
  };

  auto result = outer_func();
  REQUIRE(result.has_value());
  CHECK(result.value() == 220); // (100 + 10) * 2
}

TEST_CASE("Nested Pop with error propagation")
{
  LogCapture capture;

  auto inner_func = []() -> Value<int>
  {
    auto r1 = may_fail(true); // This fails
    return Pop(r1) + 10;
  };

  auto outer_func = [&]() -> Value<int>
  {
    auto r2 = inner_func();
    return Pop(r2) * 2; // Should not reach here
  };

  auto result = outer_func();
  REQUIRE_FALSE(result.has_value());
  CHECK(result.error() == "Operation failed");

  // Verify log stack: error bubbles up through both Pops
  // First Pop logs the original error, second Pop logs it again
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 2);
  CHECK(LogCapture::extract_log_message(logs[0]) == "D::Operation failed");
  CHECK(LogCapture::extract_log_message(logs[1]) == "D::Operation failed");
}

TEST_CASE("Nested Pop with context messages builds error stack")
{
  LogCapture capture;

  auto low_level = []() -> Value<int>
  {
    auto r1 = may_fail(true);
    return Pop(r1, "E::Failed to read file");
  };

  auto mid_level = [&]() -> Value<int>
  {
    auto r2 = low_level();
    return Pop(r2, "W::Configuration loading failed");
  };

  auto high_level = [&]() -> Value<int>
  {
    auto r3 = mid_level();
    return Pop(r3, "C::Application startup failed");
  };

  auto result = high_level();
  REQUIRE_FALSE(result.has_value());
  // Final error is from the highest level
  CHECK(result.error() == "Application startup failed");

  // Verify complete error stack in logs
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 6);

  // Low level: original error (DEBUG) + replacement (ERROR)
  CHECK(LogCapture::extract_log_message(logs[0]) == "D::Operation failed");
  CHECK(LogCapture::extract_log_message(logs[1]) == "E::Failed to read file");

  // Mid level: received error (DEBUG) + replacement (WARN)
  CHECK(LogCapture::extract_log_message(logs[2]) == "D::Failed to read file");
  CHECK(LogCapture::extract_log_message(logs[3]) == "W::Configuration loading failed");

  // High level: received error (DEBUG) + replacement (CRITICAL)
  CHECK(LogCapture::extract_log_message(logs[4]) == "D::Configuration loading failed");
  CHECK(LogCapture::extract_log_message(logs[5]) == "C::Application startup failed");
}

TEST_CASE("Value<void> with Pop macro")
{
  auto test_func = []() -> Value<void>
  {
    auto result = may_fail_void(false);
    Pop(result); // Extract void value
    return {};
  };

  auto final_result = test_func();
  CHECK(final_result.has_value());
}

TEST_CASE("Value<void> Pop with error")
{
  auto test_func = []() -> Value<void>
  {
    auto result = may_fail_void(true);
    Pop(result); // Should propagate error
    return {};
  };

  auto final_result = test_func();
  REQUIRE_FALSE(final_result.has_value());
  CHECK(final_result.error() == "Void operation failed");
}

// ============================================================================
// Try/Catch MACRO TESTS - Used with exception-safe (non-Value) functions
// ============================================================================

TEST_CASE("Try macro handles successful non-Value operations")
{
  auto test_func = []() -> Value<int>
  {
    return Try(returns_value());
  };

  auto result = test_func();
  REQUIRE(result.has_value());
  CHECK(result.value() == 42);
}

TEST_CASE("Try macro catches exceptions from non-Value functions")
{
  auto test_func = []() -> Value<int>
  {
    return Try(throws_exception());
  };

  auto result = test_func();
  REQUIRE_FALSE(result.has_value());
  CHECK(result.error().find("Exception thrown") != std::string::npos);
}

TEST_CASE("Try macro with risky division - success")
{
  auto test_func = []() -> Value<int>
  {
    return Try(risky_divide(10, 2));
  };

  auto result = test_func();
  REQUIRE(result.has_value());
  CHECK(result.value() == 5);
}

TEST_CASE("Try macro with risky division - exception")
{
  auto test_func = []() -> Value<int>
  {
    return Try(risky_divide(10, 0));
  };

  auto result = test_func();
  REQUIRE_FALSE(result.has_value());
  CHECK(result.error().find("Division by zero") != std::string::npos);
}

TEST_CASE("Try macro with void function - success")
{
  auto test_func = []() -> Value<void>
  {
    Try(succeeds_void());
    return {};
  };

  auto result = test_func();
  CHECK(result.has_value());
}

TEST_CASE("Catch macro converts exceptions to Value errors")
{
  auto result = Catch(throws_exception());

  REQUIRE_FALSE(result.has_value());
  CHECK(result.error().find("Exception thrown") != std::string::npos);
}

TEST_CASE("Catch macro with successful non-Value expression")
{
  auto result = Catch(returns_value());

  REQUIRE(result.has_value());
  CHECK(result.value() == 42);
}

TEST_CASE("Catch macro with inline lambda")
{
  auto result = Catch(
      []() -> int
      {
        return 100;
      }());

  REQUIRE(result.has_value());
  CHECK(result.value() == 100);
}

TEST_CASE("Catch macro with throwing lambda")
{
  auto result = Catch(
      []() -> int
      {
        throw std::runtime_error("Lambda threw");
      }());

  REQUIRE_FALSE(result.has_value());
  CHECK(result.error().find("Lambda threw") != std::string::npos);
}

TEST_CASE("Catch macro with void function - success")
{
  auto result = Catch(succeeds_void());

  CHECK(result.has_value());
}

TEST_CASE("Catch macro with void function - exception")
{
  auto result = Catch(throws_void_exception());

  REQUIRE_FALSE(result.has_value());
  CHECK(result.error().find("Void exception thrown") != std::string::npos);
}

// ============================================================================
// Error MACRO TESTS
// ============================================================================

TEST_CASE("Error macro creates unexpected value")
{
  LogCapture capture;

  auto test_func = []() -> Value<int>
  {
    return Error("E::Test error message");
  };

  auto result = test_func();
  REQUIRE_FALSE(result.has_value());
  CHECK(result.error() == "Test error message");

  // Verify log output
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 1);
  CHECK(LogCapture::extract_log_message(logs[0]) == "E::Test error message");
}

TEST_CASE("Error macro with format arguments")
{
  LogCapture capture;

  auto test_func = [](int code) -> Value<std::string>
  {
    return Error("E::Error code: {}", code);
  };

  auto result = test_func(404);
  REQUIRE_FALSE(result.has_value());
  CHECK(result.error() == "Error code: 404");

  // Verify log output
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 1);
  CHECK(LogCapture::extract_log_message(logs[0]) == "E::Error code: 404");
}

// ============================================================================
// discard/forward MACRO TESTS
// ============================================================================

TEST_CASE("discard macro on error value")
{
  LogCapture capture;

  Value<int> error_val = std::unexpected<std::string>("Test error");

  // discard should log but not propagate
  error_val.discard("W::Discarding error");

  // Value should still be in error state
  REQUIRE_FALSE(error_val.has_value());
  CHECK(error_val.error() == "Test error");

  // Verify log output: both original error (DEBUG) and discard message (WARN)
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 2);
  CHECK(LogCapture::extract_log_message(logs[0]) == "W::Test error");
  CHECK(LogCapture::extract_log_message(logs[1]) == "W::Discarding error");
}

TEST_CASE("discard macro on success value")
{
  LogCapture capture;

  Value<int> success_val = 42;

  // discard on success should do nothing
  success_val.discard("W::This won't log");

  // Value should still be successful
  REQUIRE(success_val.has_value());
  CHECK(success_val.value() == 42);

  // No logs should be generated for successful values
  auto logs = capture.read_logs();
  CHECK(logs.empty());
}

TEST_CASE("forward macro adds context to error")
{
  LogCapture capture;

  auto test_func = []() -> Value<int>
  {
    auto result = may_fail(true);
    if (!result)
    {
      return result.forward("E::Additional context");
    }
    return result;
  };

  auto final_result = test_func();
  REQUIRE_FALSE(final_result.has_value());
  // forward doesn't replace error, just logs additional context
  CHECK(final_result.error() == "Operation failed");

  // Verify log output: original error + additional context
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 2);
  CHECK(LogCapture::extract_log_message(logs[0]) == "E::Operation failed");
  CHECK(LogCapture::extract_log_message(logs[1]) == "E::Additional context");
}

TEST_CASE("forward macro with format arguments")
{
  LogCapture capture;

  Value<int> error_val = std::unexpected<std::string>("Original error");

  auto forwarded = error_val.forward("E::Step {}: {}", 2, "processing");

  REQUIRE_FALSE(forwarded.has_value());
  // forward preserves original error
  CHECK(forwarded.error() == "Original error");

  // Verify log output
  auto logs = capture.read_logs();
  REQUIRE(logs.size() == 2);
  CHECK(LogCapture::extract_log_message(logs[0]) == "E::Original error");
  CHECK(LogCapture::extract_log_message(logs[1]) == "E::Step 2: processing");
}
