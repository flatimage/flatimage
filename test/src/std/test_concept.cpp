/**
 * @file test_concept.cpp
 * @brief Unit tests for concept.hpp type constraints
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../../../src/std/concept.hpp"

TEST_CASE("IsInstanceOf concept identifies template instances")
{
  // Test with compile-time concept checks
  CHECK(ns_concept::IsInstanceOf<std::vector<int>, std::vector>);
  CHECK(ns_concept::IsInstanceOf<std::optional<std::string>, std::optional>);
  CHECK_FALSE(ns_concept::IsInstanceOf<int, std::vector>);
}

TEST_CASE("Boolean concept accepts bool-convertible types")
{
  CHECK(ns_concept::Boolean<bool>);
  CHECK(ns_concept::Boolean<int>);
  CHECK(ns_concept::Boolean<void*>);
  CHECK_FALSE(ns_concept::Boolean<std::string>);
}

TEST_CASE("Enum concept identifies enumerations")
{
  enum TestEnum
  {
    A,
    B
  };
  enum class TestEnumClass
  {
    X,
    Y
  };

  CHECK(ns_concept::Enum<TestEnum>);
  CHECK(ns_concept::Enum<TestEnumClass>);
  CHECK_FALSE(ns_concept::Enum<int>);
}

TEST_CASE("ScopedEnum identifies scoped enums only")
{
  enum TestEnum
  {
    A,
    B
  };
  enum class TestEnumClass
  {
    X,
    Y
  };

  CHECK_FALSE(ns_concept::ScopedEnum<TestEnum>);
  CHECK(ns_concept::ScopedEnum<TestEnumClass>);
}

TEST_CASE("Variant concept identifies std::variant")
{
  using Var = std::variant<int, std::string>;

  CHECK(ns_concept::Variant<Var>);
  CHECK_FALSE(ns_concept::Variant<int>);
  CHECK_FALSE(ns_concept::Variant<std::optional<int>>);
}

TEST_CASE("Uniform concept checks type equality")
{
  CHECK(ns_concept::Uniform<int, int>);
  CHECK(ns_concept::Uniform<const int&, int>); // cv-qualifiers removed
  CHECK_FALSE(ns_concept::Uniform<int, long>);
  CHECK_FALSE(ns_concept::Uniform<int, double>);
}

TEST_CASE("Iterable concept identifies ranges")
{
  CHECK(ns_concept::Iterable<std::vector<int>>);
  CHECK(ns_concept::Iterable<std::string>);
  CHECK(ns_concept::Iterable<int[5]>);
  CHECK_FALSE(ns_concept::Iterable<int>);
}

TEST_CASE("IsContainerOf validates container value types")
{
  CHECK(ns_concept::IsContainerOf<std::vector<int>, int>);
  CHECK(ns_concept::IsContainerOf<std::vector<std::string>, std::string>);
  CHECK_FALSE(ns_concept::IsContainerOf<std::vector<int>, double>);
}

TEST_CASE("IsVector identifies std::vector")
{
  CHECK(ns_concept::IsVector<std::vector<int>>);
  CHECK(ns_concept::IsVector<std::vector<std::string>>);
  CHECK_FALSE(ns_concept::IsVector<std::string>);
}

TEST_CASE("IsOptional identifies std::optional")
{
  CHECK(ns_concept::IsOptional<std::optional<int>>);
  CHECK(ns_concept::IsOptional<std::optional<std::string>>);
  CHECK_FALSE(ns_concept::IsOptional<int>);
}

TEST_CASE("Container concept identifies standard containers")
{
  CHECK(ns_concept::Container<std::vector<int>>);
  CHECK(ns_concept::Container<std::deque<double>>);
  CHECK(ns_concept::Container<std::list<int>>);
  CHECK(ns_concept::Container<std::set<int>>);
  CHECK(ns_concept::Container<std::map<std::string, int>>);
  CHECK_FALSE(ns_concept::Container<int>);
  CHECK_FALSE(ns_concept::Container<int[10]>);
}

TEST_CASE("StringConvertible identifies string-convertible types")
{
  CHECK(ns_concept::StringConvertible<std::string>);
  CHECK(ns_concept::StringConvertible<const char*>);
  CHECK_FALSE(ns_concept::StringConvertible<int>);
}

TEST_CASE("StringConstructible identifies types that can construct strings")
{
  CHECK(ns_concept::StringConstructible<const char*>);
  CHECK(ns_concept::StringConstructible<std::string>);
  CHECK_FALSE(ns_concept::StringConstructible<int>);
}

TEST_CASE("Numeric identifies numeric types")
{
  CHECK(ns_concept::Numeric<int>);
  CHECK(ns_concept::Numeric<double>);
  CHECK(ns_concept::Numeric<float>);
  CHECK(ns_concept::Numeric<long long>);
  CHECK_FALSE(ns_concept::Numeric<std::string>);
}

TEST_CASE("StreamInsertable identifies types with operator<<")
{
  CHECK(ns_concept::StreamInsertable<int>);
  CHECK(ns_concept::StreamInsertable<std::string>);
  CHECK(ns_concept::StreamInsertable<double>);
  CHECK_FALSE(ns_concept::StreamInsertable<std::vector<int>>);
}

TEST_CASE("StringRepresentable combines multiple string-related concepts")
{
  CHECK(ns_concept::StringRepresentable<int>);
  CHECK(ns_concept::StringRepresentable<std::string>);
  CHECK(ns_concept::StringRepresentable<const char*>);
  CHECK(ns_concept::StringRepresentable<double>);
  CHECK_FALSE(ns_concept::StringRepresentable<std::vector<int>>);
}
