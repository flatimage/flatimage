/**
 * @file test_vector.cpp
 * @brief Unit tests for vector.hpp utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "../../../src/std/vector.hpp"

TEST_CASE("ns_vector::append_range appends elements from one container to another")
{
  std::vector<int> dest = {1, 2, 3};
  std::vector<int> src = {4, 5, 6};

  ns_vector::append_range(dest, src);

  CHECK(dest == std::vector<int>{1, 2, 3, 4, 5, 6});
}

TEST_CASE("ns_vector::append_range works with strings")
{
  std::vector<std::string> dest = {"hello"};
  std::vector<std::string> src = {"world", "test"};

  ns_vector::append_range(dest, src);

  CHECK(dest == std::vector<std::string>{"hello", "world", "test"});
}

TEST_CASE("ns_vector::push_back adds multiple elements to vector")
{
  std::vector<int> vec = {1, 2};

  ns_vector::push_back(vec, 3, 4, 5);

  CHECK(vec == std::vector<int>{1, 2, 3, 4, 5});
}

TEST_CASE("ns_vector::push_back works with single element")
{
  std::vector<std::string> vec;

  ns_vector::push_back(vec, "single");

  CHECK(vec == std::vector<std::string>{"single"});
}

TEST_CASE("ns_vector::push_front prepends multiple elements to vector")
{
  std::vector<int> vec = {4, 5};

  ns_vector::push_front(vec, 1, 2, 3);

  CHECK(vec == std::vector<int>{1, 2, 3, 4, 5});
}

TEST_CASE("ns_vector::push_front works with empty vector")
{
  std::vector<std::string> vec;

  ns_vector::push_front(vec, "first", "second");

  CHECK(vec == std::vector<std::string>{"first", "second"});
}

TEST_CASE("ns_vector::from_string splits string by delimiter")
{
  std::string input = "one,two,three";

  auto result = ns_vector::from_string(input, ',');

  CHECK(result == std::vector<std::string>{"one", "two", "three"});
}

TEST_CASE("ns_vector::from_string handles empty tokens")
{
  std::string input = "a,,b";

  auto result = ns_vector::from_string(input, ',');

  CHECK(result == std::vector<std::string>{"a", "", "b"});
}

TEST_CASE("ns_vector::from_string works with different delimiters")
{
  std::string input = "foo:bar:baz";

  auto result = ns_vector::from_string(input, ':');

  CHECK(result == std::vector<std::string>{"foo", "bar", "baz"});
}

TEST_CASE("ns_vector::from_string handles single element")
{
  std::string input = "single";

  auto result = ns_vector::from_string(input, ',');

  CHECK(result == std::vector<std::string>{"single"});
}

TEST_CASE("ns_vector::from_string handles trailing delimiter")
{
  std::string input = "a,b,";

  auto result = ns_vector::from_string(input, ',');

  // std::getline with delimiter doesn't produce empty string at end
  CHECK(result == std::vector<std::string>{"a", "b"});
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE("ns_vector::append_range with empty source container")
{
  std::vector<int> dest = {1, 2, 3};
  std::vector<int> src; // Empty

  ns_vector::append_range(dest, src);

  // Destination should be unchanged
  CHECK(dest == std::vector<int>{1, 2, 3});
}

TEST_CASE("ns_vector::append_range with empty destination container")
{
  std::vector<int> dest; // Empty
  std::vector<int> src = {1, 2, 3};

  ns_vector::append_range(dest, src);

  CHECK(dest == std::vector<int>{1, 2, 3});
}

TEST_CASE("ns_vector::append_range with both containers empty")
{
  std::vector<std::string> dest;
  std::vector<std::string> src;

  ns_vector::append_range(dest, src);

  CHECK(dest.empty());
}

TEST_CASE("ns_vector::push_back with no arguments compiles")
{
  std::vector<int> vec = {1, 2};

  // This should compile and do nothing
  ns_vector::push_back(vec);

  // Vector should be unchanged
  CHECK(vec == std::vector<int>{1, 2});
}

TEST_CASE("ns_vector::push_front with single element")
{
  std::vector<int> vec = {2, 3};

  ns_vector::push_front(vec, 1);

  CHECK(vec == std::vector<int>{1, 2, 3});
}

TEST_CASE("ns_vector::from_string with empty string")
{
  std::string input = "";

  auto result = ns_vector::from_string(input, ',');

  // Empty string produces zero elements (getline on empty stream)
  REQUIRE(result.size() == 0);
}

TEST_CASE("ns_vector::from_string with leading delimiter")
{
  std::string input = ",a,b";

  auto result = ns_vector::from_string(input, ',');

  CHECK(result == std::vector<std::string>{"", "a", "b"});
}

TEST_CASE("ns_vector::from_string with multiple consecutive delimiters")
{
  std::string input = "a,,,b";

  auto result = ns_vector::from_string(input, ',');

  CHECK(result == std::vector<std::string>{"a", "", "", "b"});
}

TEST_CASE("ns_vector::from_string with delimiter at start and end")
{
  std::string input = "|start|end|";

  auto result = ns_vector::from_string(input, '|');

  CHECK(result == std::vector<std::string>{"", "start", "end"});
}

TEST_CASE("ns_vector::from_string with only delimiter")
{
  std::string input = ";;;";

  auto result = ns_vector::from_string(input, ';');

  CHECK(result == std::vector<std::string>{"", "", ""});
}

TEST_CASE("ns_vector::push_back with large number of elements")
{
  std::vector<int> vec;

  ns_vector::push_back(vec, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

  CHECK(vec == std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
}

TEST_CASE("ns_vector::push_front maintains order of prepended elements")
{
  std::vector<int> vec = {10};

  ns_vector::push_front(vec, 1, 2, 3, 4, 5);

  CHECK(vec == std::vector<int>{1, 2, 3, 4, 5, 10});
}
