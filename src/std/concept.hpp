/**
 * @file concept.hpp
 * @author Ruan Formigoni
 * @brief Custom C++ concepts for type constraints and compile-time validation
 *
 * This file provides a comprehensive set of C++20 concepts for common type requirements,
 * including container checks, type conversions, iterability, and more. All concepts follow
 * the standard library naming conventions and semantics where applicable.
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <optional>
#include <ranges>
#include <deque>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>

namespace ns_concept
{

// ============================================================================
// Template Metaprogramming Utilities
// ============================================================================

/**
 * @brief Type trait to check if a type is an instance of a template
 * @tparam T The type to check
 * @tparam U The template to match against
 */
template<typename T, template<typename...> typename U>
inline constexpr bool is_instance_of_v = std::false_type {};

/**
 * @brief Specialization for matching template instances
 * @tparam U The template type
 * @tparam Args Template arguments
 */
template<template<typename...> typename U, typename... Args>
inline constexpr bool is_instance_of_v<U<Args...>,U> = std::true_type {};

/**
 * @brief Concept to check if a type is an instance of a specific template
 * @tparam T The type to check
 * @tparam U The template to match against
 *
 * Example:
 * @code
 * static_assert(IsInstanceOf<std::vector<int>, std::vector>);
 * static_assert(!IsInstanceOf<std::list<int>, std::vector>);
 * @endcode
 */
template<typename T, template<typename...> typename U>
concept IsInstanceOf = is_instance_of_v<std::remove_cvref_t<T>, U>;

static_assert(!IsInstanceOf<std::vector<int>*, std::vector>);
static_assert( IsInstanceOf<std::vector<int>&, std::vector>);
static_assert( IsInstanceOf<std::vector<int> const&, std::vector>);
static_assert( IsInstanceOf<std::vector<int>, std::vector>);
static_assert( IsInstanceOf<std::vector<std::string>, std::vector>);
static_assert(!IsInstanceOf<std::string, std::vector>);
static_assert( IsInstanceOf<std::optional<int>, std::optional>);

// ============================================================================
// Boolean and Logical Concepts
// ============================================================================

/**
 * @brief Concept for types that can be contextually converted to bool
 * @tparam T The type to check
 *
 * This concept matches the standard library's boolean-testable requirements.
 * Types satisfying this concept can be used in boolean contexts (if, while, etc.)
 * and support logical negation.
 *
 * Accepts: bool, int, pointers, std::optional, std::unique_ptr, etc.
 *
 * Example:
 * @code
 * template<Boolean T>
 * void do_if_true(T condition) { if (condition) { } }
 * @endcode
 */
template<typename T>
concept Boolean = std::convertible_to<std::remove_cvref_t<T>, bool>
&& requires(T t)
{
  { !t } -> std::convertible_to<bool>;
};

static_assert( Boolean<bool const&>);
static_assert( Boolean<bool &>);
static_assert( Boolean<int const&>);
static_assert( Boolean<int &>);
static_assert( Boolean<void*>);
static_assert(!Boolean<std::string>);
static_assert(!Boolean<std::vector<int>>);

// ============================================================================
// Enum Concepts
// ============================================================================

/**
 * @brief Concept for enumeration types
 * @tparam T The type to check
 *
 * Matches both scoped (enum class) and unscoped enums.
 *
 * Example:
 * @code
 * enum class Color { Red, Green, Blue };
 * static_assert(Enum<Color>);
 * @endcode
 */
template<typename T>
concept Enum = std::is_enum_v<T>;

enum TestEnum { VALUE1, VALUE2 };
enum class TestEnumClass { VALUE1, VALUE2 };
static_assert( Enum<TestEnum>);
static_assert( Enum<TestEnumClass>);
static_assert(!Enum<int>);
static_assert(!Enum<std::string>);

/**
 * @brief Concept for scoped enumeration types (enum class)
 * @tparam T The type to check
 *
 * Only matches scoped enums (enum class), not unscoped enums.
 */
template<typename T>
concept ScopedEnum = Enum<T> && !std::is_convertible_v<T, std::underlying_type_t<T>>;

static_assert(!ScopedEnum<TestEnum>);
static_assert( ScopedEnum<TestEnumClass>);

// ============================================================================
// Variant Concepts
// ============================================================================

/**
 * @brief Concept for std::variant types
 * @tparam T The type to check
 *
 * Checks if a type is a std::variant instantiation.
 *
 * Example:
 * @code
 * using MyVariant = std::variant<int, std::string>;
 * static_assert(Variant<MyVariant>);
 * @endcode
 */
template <typename T, typename... Args>
concept Variant = IsInstanceOf<T, std::variant>;

static_assert( Variant<std::variant<int, std::string>>);
static_assert( Variant<std::variant<int>>);
static_assert(!Variant<int>);
static_assert(!Variant<std::optional<int>>);

// ============================================================================
// Type Equality Concepts
// ============================================================================

/**
 * @brief Concept to check if all types are the same
 * @tparam T The type to check
 * @tparam U The types to match against
 *
 * Automatically removes cv-qualifiers and references before comparison.
 *
 * Example:
 * @code
 * static_assert( Uniform<int, int>);
 * static_assert(!Uniform<int, long>);
 * static_assert(!Uniform<int, double, float, std::string>)
 * @endcode
 */
template<typename T, typename... U>
concept Uniform = ( std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>> and ... );

static_assert( Uniform<int, int>);
static_assert(!Uniform<int, long>);
static_assert(!Uniform<int, int, long, short>);
static_assert( Uniform<const int&, int>);
static_assert(!Uniform<int, std::string>);
static_assert(!Uniform<int, double, float, std::string>);

// ============================================================================
// Iterator and Range Concepts
// ============================================================================

/**
 * @brief Concept for iterable containers (has begin() and end())
 * @tparam T The type to check
 *
 * Uses std::ranges::range from C++20, which properly checks for begin()/end()
 * compatibility and sentinel types.
 *
 * Example:
 * @code
 * template<Iterable Container>
 * void print_all(const Container& c) {
 *   for (const auto& item : c) { std::cout << item; }
 * }
 * @endcode
 */
template<typename T>
concept Iterable = std::ranges::range<T>;

static_assert( Iterable<std::vector<int>>);
static_assert( Iterable<std::string>);
static_assert( Iterable<int[5]>);
static_assert(!Iterable<int>);
static_assert(!Iterable<void*>);

// ============================================================================
// Container Concepts
// ============================================================================

/**
 * @brief Concept for containers holding a specific value type
 * @tparam T The container type
 * @tparam U The expected value type
 *
 * Checks if a container's value_type matches the specified type and is iterable.
 *
 * Example:
 * @code
 * template<IsContainerOf<int> Container>
 * int sum(const Container& c) {
 *   int total = 0;
 *   for (int val : c) total += val;
 *   return total;
 * }
 * @endcode
 */
template <typename T, typename U>
concept IsContainerOf = requires
{
  typename T::value_type;
  requires std::same_as<typename T::value_type, U>;
  requires Iterable<T>;
};

static_assert( IsContainerOf<std::vector<int>, int>);
static_assert( IsContainerOf<std::vector<std::string>, std::string>);
static_assert(!IsContainerOf<std::vector<int>, double>);
static_assert(!IsContainerOf<int, int>);

/**
 * @brief Concept for std::vector types
 * @tparam T The type to check
 *
 * Checks if a type is any instantiation of std::vector.
 */
template <typename T>
concept IsVector = IsInstanceOf<T, std::vector>;

static_assert( IsVector<std::vector<int>>);
static_assert( IsVector<std::vector<std::string>>);
static_assert(!IsVector<std::string>);
static_assert(!IsVector<int>);

/**
 * @brief Concept for std::optional types
 * @tparam T The type to check
 */
template <typename T>
concept IsOptional = IsInstanceOf<T, std::optional>;

static_assert( IsOptional<std::optional<int>>);
static_assert( IsOptional<std::optional<std::string>>);
static_assert(!IsOptional<int>);
static_assert(!IsOptional<std::vector<int>>);

/**
 * @brief Concept for standard library container types
 * @tparam T The type to check
 *
 * Matches any instantiation of std::vector, std::deque, std::list, std::set,
 * std::multiset, std::map, std::multimap, std::unordered_set, std::unordered_multiset,
 * std::unordered_map, or std::unordered_multimap.
 *
 * Containers must have:
 * - value_type member type
 * - Iterable via begin()/end()
 *
 * Example:
 * @code
 * template<Container T>
 * void process(const T& c) { for (const auto& item : c) { } }
 *
 * static_assert(Container<std::vector<int>>);
 * static_assert(Container<std::map<std::string, int>>);
 * static_assert(!Container<int>);
 * static_assert(!Container<int[10]>);  // C arrays are not containers
 * @endcode
 */
template <typename T>
concept Container = IsInstanceOf<T, std::vector>
  or IsInstanceOf<T, std::deque>
  or IsInstanceOf<T, std::list>
  or IsInstanceOf<T, std::set>
  or IsInstanceOf<T, std::map>
  or IsInstanceOf<T, std::unordered_set>
  or IsInstanceOf<T, std::unordered_map>;

static_assert( Container<std::vector<int>>);
static_assert( Container<std::vector<std::string>>);
static_assert( Container<std::deque<double>>);
static_assert( Container<std::list<int>>);
static_assert( Container<std::set<int>>);
static_assert( Container<std::map<std::string, int>>);
static_assert( Container<std::unordered_set<int>>);
static_assert( Container<std::unordered_map<std::string, int>>);
static_assert(!Container<int>);
static_assert(!Container<int[10]>);
static_assert(!Container<std::optional<int>>);
static_assert(!Container<std::string>);

// ============================================================================
// String Concepts
// ============================================================================

/**
 * @brief Concept for types implicitly convertible to std::string
 * @tparam T The type to check
 *
 * Example: const char*, std::string_view (with conversion operator)
 */
template<typename T>
concept StringConvertible = std::is_convertible_v<std::remove_cvref_t<T>, std::string>;

static_assert( StringConvertible<std::string>);
static_assert( StringConvertible<const char*>);
static_assert(!StringConvertible<int>);
static_assert(!StringConvertible<std::vector<char>>);

/**
 * @brief Concept for types that can construct a std::string
 * @tparam T The type to check
 *
 * Example: const char*, std::string_view, iterators
 */
template<typename T>
concept StringConstructible = std::constructible_from<std::string, std::remove_cvref_t<T>>;

static_assert( StringConstructible<const char*>);
static_assert( StringConstructible<std::string>);
static_assert(!StringConstructible<int>);
static_assert(!StringConstructible<void*>);

// ============================================================================
// Numeric Concepts
// ============================================================================

/**
 * @brief Concept for numeric types (integral or floating-point)
 * @tparam T The type to check
 *
 * Matches: int, long, float, double, etc. (not char by default in some contexts)
 *
 * Example:
 * @code
 * template<Numeric T>
 * T square(T value) { return value * value; }
 * @endcode
 */
template<typename T>
concept Numeric = std::integral<std::remove_cvref_t<T>> or std::floating_point<std::remove_cvref_t<T>>;

// Static assertions for Numeric
static_assert( Numeric<int>);
static_assert( Numeric<double>);
static_assert( Numeric<float>);
static_assert( Numeric<long long>);
static_assert( Numeric<unsigned int>);
static_assert(!Numeric<std::string>);
static_assert(!Numeric<void*>);

// ============================================================================
// Stream and Formatting Concepts
// ============================================================================

/**
 * @brief Concept for types that support stream insertion (operator<<)
 * @tparam T The type to check
 *
 * Types satisfying this concept can be written to std::ostream.
 *
 * Example:
 * @code
 * template<StreamInsertable T>
 * void print(const T& value) { std::cout << value; }
 * @endcode
 */
template<typename T>
concept StreamInsertable = requires(T t, std::ostream& os)
{
  { os << t } -> std::same_as<std::ostream&>;
};

// Static assertions for StreamInsertable
static_assert( StreamInsertable<int>);
static_assert( StreamInsertable<std::string>);
static_assert( StreamInsertable<double>);
static_assert(!StreamInsertable<std::vector<int>>);

/**
 * @brief Concept for types that can be represented as a string
 * @tparam T The type to check
 *
 * Combines multiple string-representable properties: convertible to string,
 * constructible as string, numeric, or stream insertable.
 *
 * Example:
 * @code
 * template<StringRepresentable T>
 * std::string to_string(const T& value) { ... }
 * @endcode
 */
template<typename T>
concept StringRepresentable = StringConvertible<T>
  or StringConstructible<T>
  or Numeric<T>
  or StreamInsertable<T>;

static_assert( StringRepresentable<int>);
static_assert( StringRepresentable<std::string>);
static_assert( StringRepresentable<const char*>);
static_assert( StringRepresentable<double>);
static_assert(!StringRepresentable<std::vector<int>>);

} // namespace ns_concept

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
