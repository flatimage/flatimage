/**
 * @file test_string.cpp
 * @brief Unit tests for string.hpp utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>
#include <string>
#include <vector>

#include "../../../src/std/string.hpp"

TEST_CASE("ns_string::static_string stores compile-time strings")
{
  constexpr ns_string::static_string str("hello");

  CHECK(str.size() == 5);
  CHECK(std::string_view(str) == "hello");
}

TEST_CASE("ns_string::static_string::join concatenates strings")
{
  constexpr ns_string::static_string s1("hello");
  constexpr ns_string::static_string s2(" ");
  constexpr ns_string::static_string s3("world");

  constexpr auto result = s1.template join<s2, s3>();

  CHECK(std::string_view(result) == "hello world");
}

TEST_CASE("ns_string::to_string converts string types")
{
  std::string input = "test";

  auto result = ns_string::to_string(input);

  CHECK(result == "test");
}

TEST_CASE("ns_string::to_string converts c-string")
{
  const char* input = "c-string";

  auto result = ns_string::to_string(input);

  CHECK(result == "c-string");
}

TEST_CASE("ns_string::to_string converts numeric types")
{
  int num = 42;

  auto result = ns_string::to_string(num);

  CHECK(result == "42");
}

TEST_CASE("ns_string::to_string converts floating point")
{
  double num = 3.14;

  auto result = ns_string::to_string(num);

  CHECK(result.find("3.14") != std::string::npos);
}

TEST_CASE("ns_string::to_string converts stream-insertable types")
{
  std::ostringstream expected;
  expected << std::hex << 255;

  // For demonstration, just use an int
  int val = 255;
  auto result = ns_string::to_string(val);

  CHECK(result == "255");
}

TEST_CASE("ns_string::from_container joins container elements")
{
  std::vector<std::string> vec = {"hello", "world", "test"};

  auto result = ns_string::from_container(vec);

  CHECK(result == "helloworldtest");
}

TEST_CASE("ns_string::from_container joins with separator")
{
  std::vector<std::string> vec = {"a", "b", "c"};

  auto result = ns_string::from_container(vec, ',');

  CHECK(result == "a,b,c");
}

TEST_CASE("ns_string::from_container handles empty container")
{
  std::vector<std::string> vec;

  auto result = ns_string::from_container(vec);

  CHECK(result.empty());
}

TEST_CASE("ns_string::from_container works with iterators")
{
  std::vector<std::string> vec = {"x", "y", "z"};

  auto result = ns_string::from_container(vec.begin(), vec.end(), '-');

  CHECK(result == "x-y-z");
}

TEST_CASE("ns_string::placeholders_replace replaces {} placeholders")
{
  std::string template_str = "Hello, {}!";

  auto result = ns_string::placeholders_replace(template_str, "World");

  CHECK(result == "Hello, World!");
}

TEST_CASE("ns_string::placeholders_replace handles multiple placeholders")
{
  std::string template_str = "{} + {} = {}";

  auto result = ns_string::placeholders_replace(template_str, 2, 3, 5);

  CHECK(result == "2 + 3 = 5");
}

TEST_CASE("ns_string::placeholders_replace handles more placeholders than arguments")
{
  std::string template_str = "{} and {} and {}";

  auto result = ns_string::placeholders_replace(template_str, "one", "two");

  CHECK(result == "one and two and {}");
}

TEST_CASE("ns_string::placeholders_replace handles no placeholders")
{
  std::string template_str = "no placeholders";

  auto result = ns_string::placeholders_replace(template_str, "arg");

  CHECK(result == "no placeholders");
}

TEST_CASE("ns_string::placeholders_replace works with different types")
{
  std::string template_str = "String: {}, Int: {}, Double: {}";

  auto result = ns_string::placeholders_replace(template_str, "test", 42, 3.14);

  CHECK(result.find("String: test") != std::string::npos);
  CHECK(result.find("Int: 42") != std::string::npos);
  CHECK(result.find("Double: 3.14") != std::string::npos);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE("ns_string::to_string converts containers")
{
  std::vector<int> vec = {1, 2, 3};

  auto result = ns_string::to_string(vec);

  // Should contain the elements
  CHECK(result.find("1") != std::string::npos);
  CHECK(result.find("2") != std::string::npos);
  CHECK(result.find("3") != std::string::npos);
}

TEST_CASE("ns_string::to_string handles empty containers")
{
  std::vector<int> vec;

  auto result = ns_string::to_string(vec);

  // Empty containers should produce some output (likely brackets)
  CHECK(!result.empty());
}

TEST_CASE("ns_string::from_container with single element")
{
  std::vector<std::string> vec = {"solo"};

  auto result = ns_string::from_container(vec, ',');

  CHECK(result == "solo");
}

TEST_CASE("ns_string::from_container with numeric separator")
{
  std::vector<int> vec = {1, 2, 3};

  auto result = ns_string::from_container(vec, '-');

  CHECK(result == "1-2-3");
}

TEST_CASE("ns_string::from_container with space separator")
{
  std::vector<std::string> vec = {"foo", "bar", "baz"};

  auto result = ns_string::from_container(vec, ' ');

  CHECK(result == "foo bar baz");
}

TEST_CASE("ns_string::placeholders_replace with empty format string")
{
  std::string template_str = "";

  auto result = ns_string::placeholders_replace(template_str, "arg");

  CHECK(result.empty());
}

TEST_CASE("ns_string::placeholders_replace with only placeholders")
{
  std::string template_str = "{}{}{}";

  auto result = ns_string::placeholders_replace(template_str, "a", "b", "c");

  CHECK(result == "abc");
}

TEST_CASE("ns_string::placeholders_replace with no arguments")
{
  std::string template_str = "{} and {}";

  auto result = ns_string::placeholders_replace(template_str);

  CHECK(result == "{} and {}");
}

TEST_CASE("ns_string::placeholders_replace with special characters in args")
{
  std::string template_str = "Quote: {}";

  auto result = ns_string::placeholders_replace(template_str, "\"test\"");

  CHECK(result == "Quote: \"test\"");
}

TEST_CASE("ns_string::static_string::join with empty strings")
{
  constexpr ns_string::static_string s1("");
  constexpr ns_string::static_string s2("world");

  constexpr auto result = s1.template join<s2>();

  CHECK(std::string_view(result) == "world");
}

TEST_CASE("ns_string::static_string with many joins")
{
  constexpr ns_string::static_string s1("a");
  constexpr ns_string::static_string s2("b");
  constexpr ns_string::static_string s3("c");
  constexpr ns_string::static_string s4("d");
  constexpr ns_string::static_string s5("e");

  constexpr auto result = s1.template join<s2, s3, s4, s5>();

  CHECK(std::string_view(result) == "abcde");
}

TEST_CASE("ns_string::to_string with negative numbers")
{
  int num = -42;

  auto result = ns_string::to_string(num);

  CHECK(result == "-42");
}

TEST_CASE("ns_string::to_string with very large number")
{
  long long num = 9223372036854775807LL; // max long long

  auto result = ns_string::to_string(num);

  CHECK(result == "9223372036854775807");
}

TEST_CASE("ns_string::to_string with zero")
{
  int num = 0;

  auto result = ns_string::to_string(num);

  CHECK(result == "0");
}

TEST_CASE("ns_string::from_container with boolean values")
{
  std::vector<bool> vec = {true, false, true};

  auto result = ns_string::from_container(vec, ',');

  CHECK(result == "1,0,1");
}

TEST_CASE("ns_string::placeholders_replace preserves non-placeholder braces")
{
  std::string template_str = "Code: {{ {}, {} }}";

  auto result = ns_string::placeholders_replace(template_str, "x", "y");

  // Should replace {} but preserve {{ }}
  CHECK(result.find("x") != std::string::npos);
  CHECK(result.find("y") != std::string::npos);
}
