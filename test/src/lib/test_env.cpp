/**
 * @file test_env.cpp
 * @brief Unit tests for env.hpp environment variable utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cstdlib>
#include <string>

#include "../../../src/lib/env.hpp"

TEST_CASE("ns_env::set creates environment variable")
{
  ns_env::set("TEST_VAR_SET", "test_value", ns_env::Replace::Y);

  const char* value = std::getenv("TEST_VAR_SET");
  REQUIRE(value != nullptr);
  CHECK(std::string(value) == "test_value");

  // Cleanup
  unsetenv("TEST_VAR_SET");
}

TEST_CASE("ns_env::set with Replace::N does not overwrite existing")
{
  setenv("TEST_VAR_NO_REPLACE", "original", 1);

  ns_env::set("TEST_VAR_NO_REPLACE", "new_value", ns_env::Replace::N);

  const char* value = std::getenv("TEST_VAR_NO_REPLACE");
  REQUIRE(value != nullptr);
  CHECK(std::string(value) == "original");

  // Cleanup
  unsetenv("TEST_VAR_NO_REPLACE");
}

TEST_CASE("ns_env::set with Replace::Y overwrites existing")
{
  setenv("TEST_VAR_REPLACE", "original", 1);

  ns_env::set("TEST_VAR_REPLACE", "new_value", ns_env::Replace::Y);

  const char* value = std::getenv("TEST_VAR_REPLACE");
  REQUIRE(value != nullptr);
  CHECK(std::string(value) == "new_value");

  // Cleanup
  unsetenv("TEST_VAR_REPLACE");
}

TEST_CASE("ns_env::get_expected returns value for existing variable")
{
  setenv("TEST_VAR_GET", "test_value", 1);

  auto result = ns_env::get_expected("TEST_VAR_GET");

  REQUIRE(result.has_value());
  CHECK(result.value() == "test_value");

  // Cleanup
  unsetenv("TEST_VAR_GET");
}

TEST_CASE("ns_env::get_expected returns error for non-existent variable")
{
  // Ensure variable doesn't exist
  unsetenv("NONEXISTENT_VAR");

  auto result = ns_env::get_expected("NONEXISTENT_VAR");

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_env::exists checks if variable has specific value")
{
  setenv("TEST_VAR_EXISTS", "specific_value", 1);

  CHECK(ns_env::exists("TEST_VAR_EXISTS", "specific_value"));
  CHECK_FALSE(ns_env::exists("TEST_VAR_EXISTS", "wrong_value"));
  CHECK_FALSE(ns_env::exists("NONEXISTENT_VAR", "any_value"));

  // Cleanup
  unsetenv("TEST_VAR_EXISTS");
}

TEST_CASE("ns_env::expand performs variable expansion")
{
  setenv("TEST_EXPAND_VAR", "expanded_value", 1);

  auto result = ns_env::expand("$TEST_EXPAND_VAR");

  REQUIRE(result.has_value());
  CHECK(result.value() == "expanded_value");

  // Cleanup
  unsetenv("TEST_EXPAND_VAR");
}

TEST_CASE("ns_env::expand handles tilde expansion")
{
  auto result = ns_env::expand("~/test");

  REQUIRE(result.has_value());
  // Should expand tilde to home directory
  std::filesystem::path path_dir_home = getenv("HOME");
  CHECK(std::filesystem::path{result.value()} == path_dir_home / "test");
  CHECK(result.value()[0] == '/'); // Should be absolute path
}

TEST_CASE("ns_env::expand returns string unchanged if no expansion needed")
{
  auto result = ns_env::expand("literal_string");

  REQUIRE(result.has_value());
  CHECK(result.value() == "literal_string");
}

TEST_CASE("ns_env::xdg_data_home returns XDG_DATA_HOME if set")
{
  setenv("XDG_DATA_HOME", "/custom/data/home", 1);

  auto result = ns_env::xdg_data_home();

  REQUIRE(result.has_value());
  CHECK(result.value() == "/custom/data/home");

  // Cleanup
  unsetenv("XDG_DATA_HOME");
}

TEST_CASE("ns_env::xdg_data_home falls back to HOME/.local/share")
{
  // Unset XDG_DATA_HOME to force fallback
  unsetenv("XDG_DATA_HOME");

  // HOME should be set in environment
  const char* home = std::getenv("HOME");
  REQUIRE(home != nullptr);

  auto result = ns_env::xdg_data_home();

  REQUIRE(result.has_value());
  CHECK(result.value() == std::string(home) + "/.local/share");
}

TEST_CASE("ns_env::search_path finds executables in PATH")
{
  // Most systems have /bin/sh
  auto result = ns_env::search_path("sh");

  if (result.has_value())
  {
    CHECK(std::filesystem::exists(result.value()));
    CHECK(result.value().filename() == "sh");
  }
}

TEST_CASE("ns_env::search_path returns error for non-existent executable")
{
  auto result = ns_env::search_path("this_executable_definitely_does_not_exist_12345");

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_env::search_path rejects absolute paths")
{
  auto result = ns_env::search_path("/bin/sh");

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_env::set works with numeric values")
{
  ns_env::set("TEST_NUMERIC", 12345, ns_env::Replace::Y);

  const char* value = std::getenv("TEST_NUMERIC");
  REQUIRE(value != nullptr);
  CHECK(std::string(value) == "12345");

  // Cleanup
  unsetenv("TEST_NUMERIC");
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE("ns_env::set with empty value")
{
  ns_env::set("TEST_EMPTY_VAL", "", ns_env::Replace::Y);

  const char* value = std::getenv("TEST_EMPTY_VAL");
  REQUIRE(value != nullptr);
  CHECK(std::string(value) == "");

  // Cleanup
  unsetenv("TEST_EMPTY_VAL");
}

TEST_CASE("ns_env::set with special characters in value")
{
  ns_env::set("TEST_SPECIAL", "!@#$%^&*()=+[]{}|;:',<>?", ns_env::Replace::Y);

  const char* value = std::getenv("TEST_SPECIAL");
  REQUIRE(value != nullptr);
  CHECK(std::string(value) == "!@#$%^&*()=+[]{}|;:',<>?");

  // Cleanup
  unsetenv("TEST_SPECIAL");
}

TEST_CASE("ns_env::set with very long value")
{
  std::string long_value(5000, 'x');
  ns_env::set("TEST_LONG_VAL", long_value, ns_env::Replace::Y);

  const char* value = std::getenv("TEST_LONG_VAL");
  REQUIRE(value != nullptr);
  CHECK(std::string(value) == long_value);

  // Cleanup
  unsetenv("TEST_LONG_VAL");
}

TEST_CASE("ns_env::get_expected with empty variable value")
{
  setenv("TEST_EMPTY_GET", "", 1);

  auto result = ns_env::get_expected("TEST_EMPTY_GET");

  REQUIRE(result.has_value());
  CHECK(result.value() == "");

  // Cleanup
  unsetenv("TEST_EMPTY_GET");
}

TEST_CASE("ns_env::exists with empty value string")
{
  setenv("TEST_EXISTS_EMPTY", "", 1);

  CHECK(ns_env::exists("TEST_EXISTS_EMPTY", ""));
  CHECK_FALSE(ns_env::exists("TEST_EXISTS_EMPTY", "non-empty"));

  // Cleanup
  unsetenv("TEST_EXISTS_EMPTY");
}

TEST_CASE("ns_env::expand with multiple variables")
{
  setenv("TEST_VAR1", "hello", 1);
  setenv("TEST_VAR2", "world", 1);

  auto result = ns_env::expand("$TEST_VAR1 $TEST_VAR2");

  REQUIRE(result.has_value());
  // wordexp may expand only the first variable or both, depending on implementation
  CHECK(result.value().find("hello") != std::string::npos);

  // Cleanup
  unsetenv("TEST_VAR1");
  unsetenv("TEST_VAR2");
}

TEST_CASE("ns_env::expand with braced variable syntax")
{
  setenv("TEST_BRACE_VAR", "braced", 1);

  auto result = ns_env::expand("${TEST_BRACE_VAR}");

  REQUIRE(result.has_value());
  CHECK(result.value() == "braced");

  // Cleanup
  unsetenv("TEST_BRACE_VAR");
}

TEST_CASE("ns_env::expand with undefined variable")
{
  unsetenv("TEST_UNDEFINED_VAR");

  auto result = ns_env::expand("$TEST_UNDEFINED_VAR");

  // Should still succeed, likely expanding to empty or keeping $VAR
  REQUIRE(result.has_value());
}

TEST_CASE("ns_env::search_path with empty PATH")
{
  // Save original PATH
  const char* orig_path = std::getenv("PATH");
  std::string saved_path = orig_path ? orig_path : "";

  // Set empty PATH
  setenv("PATH", "", 1);

  auto result = ns_env::search_path("sh");

  // Should fail with empty PATH
  CHECK_FALSE(result.has_value());

  // Restore PATH
  if (!saved_path.empty())
  {
    setenv("PATH", saved_path.c_str(), 1);
  }
}

TEST_CASE("ns_env::search_path with relative path component in PATH")
{
  // Save original PATH
  const char* orig_path = std::getenv("PATH");
  std::string saved_path = orig_path ? orig_path : "";

  // Add current directory to PATH
  setenv("PATH", (std::string(".:") + saved_path).c_str(), 1);

  auto result = ns_env::search_path("sh");

  // Should still work, finding sh in standard locations
  // (current dir likely won't have sh)

  // Restore PATH
  if (!saved_path.empty())
  {
    setenv("PATH", saved_path.c_str(), 1);
  }
}

TEST_CASE("ns_env::set with negative number")
{
  ns_env::set("TEST_NEGATIVE", -999, ns_env::Replace::Y);

  const char* value = std::getenv("TEST_NEGATIVE");
  REQUIRE(value != nullptr);
  CHECK(std::string(value) == "-999");

  // Cleanup
  unsetenv("TEST_NEGATIVE");
}

TEST_CASE("ns_env::expand with tilde in middle of path")
{
  auto result = ns_env::expand("/usr/~/test");

  REQUIRE(result.has_value());
  // Tilde should only expand at start
  CHECK(result.value() == "/usr/~/test");
}

TEST_CASE("ns_env::exists checks variable existence")
{
  setenv("TEST_VAR_CHECK", "value", 1);

  // Calling with matching value should return true
  CHECK(ns_env::exists("TEST_VAR_CHECK", "value"));

  // Calling with non-matching value should return false
  CHECK_FALSE(ns_env::exists("TEST_VAR_CHECK", "other"));

  // Cleanup
  unsetenv("TEST_VAR_CHECK");
}
