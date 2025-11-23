/**
 * @file test_enum.cpp
 * @brief Unit tests for enum.hpp custom enumeration system
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <string>

#include "../../../src/std/enum.hpp"

// Define test enums using the ENUM macro
ENUM(Color, RED, GREEN, BLUE);
ENUM(Status, PENDING, RUNNING, COMPLETED, FAILED);
ENUM(Direction, NORTH, SOUTH, EAST, WEST);

TEST_CASE("ENUM creates enum with values")
{
  Color red = Color::RED;
  Color green = Color::GREEN;

  CHECK(red == Color::enum_t::RED);
  CHECK(green == Color::enum_t::GREEN);
}

TEST_CASE("ENUM includes NONE as default value")
{
  Color default_color;

  CHECK(default_color == Color::enum_t::NONE);
}

TEST_CASE("ENUM converts to string")
{
  Color red = Color::RED;
  std::string str = red;

  CHECK(str == "RED");
}

TEST_CASE("ENUM converts different values to string")
{
  Status pending = Status::PENDING;
  Status running = Status::RUNNING;
  Status completed = Status::COMPLETED;

  CHECK(std::string(pending) == "PENDING");
  CHECK(std::string(running) == "RUNNING");
  CHECK(std::string(completed) == "COMPLETED");
}

TEST_CASE("ENUM lower() returns lowercase string")
{
  Color blue = Color::BLUE;

  CHECK(blue.lower() == "blue");
}

TEST_CASE("ENUM from_string parses valid strings")
{
  auto result = Color::from_string("RED");

  REQUIRE(result.has_value());
  CHECK(result.value() == Color::enum_t::RED);
}

TEST_CASE("ENUM from_string is case-insensitive")
{
  auto lower_result = Color::from_string("green");
  auto upper_result = Color::from_string("GREEN");
  auto mixed_result = Color::from_string("GrEeN");

  REQUIRE(lower_result.has_value());
  REQUIRE(upper_result.has_value());
  REQUIRE(mixed_result.has_value());

  CHECK(lower_result.value() == Color::enum_t::GREEN);
  CHECK(upper_result.value() == Color::enum_t::GREEN);
  CHECK(mixed_result.value() == Color::enum_t::GREEN);
}

TEST_CASE("ENUM from_string returns error for invalid strings")
{
  auto result = Color::from_string("INVALID");

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ENUM supports comparison operators")
{
  Color red = Color::RED;
  Color green = Color::GREEN;
  Color also_red = Color::RED;

  CHECK(red == Color::enum_t::RED);
  CHECK(red == also_red.get());
  CHECK_FALSE(red == green.get());
}

TEST_CASE("ENUM supports less-than comparison")
{
  Direction north = Direction::NORTH;
  Direction south = Direction::SOUTH;

  // Order is based on enum definition order
  CHECK(north < south);
}

TEST_CASE("ENUM copy constructor works")
{
  Status original = Status::RUNNING;
  Status copy = original;

  CHECK(copy == Status::enum_t::RUNNING);
  CHECK(std::string(copy) == "RUNNING");
}

TEST_CASE("ENUM copy assignment works")
{
  Status status1 = Status::PENDING;
  Status status2 = Status::COMPLETED;

  status1 = status2;

  CHECK(status1 == Status::enum_t::COMPLETED);
}

TEST_CASE("ENUM get() returns underlying enum_t")
{
  Color red = Color::RED;

  CHECK(red.get() == Color::enum_t::RED);
}

TEST_CASE("ENUM size member reflects number of values")
{
  Color c;

  // RED, GREEN, BLUE, NONE = 4
  CHECK(c.size == 4);
}

TEST_CASE("ENUM static members are accessible")
{
  // Access static members directly
  Color::enum_t red_val = Color::RED;
  Color::enum_t green_val = Color::GREEN;

  Color red{red_val};
  Color green{green_val};

  CHECK(red == Color::enum_t::RED);
  CHECK(green == Color::enum_t::GREEN);
}

TEST_CASE("ENUM implicit conversion to enum_t")
{
  Color red = Color::RED;
  Color::enum_t underlying = red;

  CHECK(underlying == Color::enum_t::RED);
}

TEST_CASE("ENUM construction from enum_t")
{
  Color color{Color::enum_t::BLUE};

  CHECK(color == Color::enum_t::BLUE);
  CHECK(std::string(color) == "BLUE");
}
