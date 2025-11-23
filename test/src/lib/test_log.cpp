/**
 * @file test_log.cpp
 * @brief Unit tests for log.hpp logging utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#include "../../../src/lib/log.hpp"

namespace fs = std::filesystem;

TEST_CASE("ns_log::Level enum has all expected values")
{
  using enum ns_log::Level;

  // Just verify enum values exist
  [[maybe_unused]] auto critical = CRITICAL;
  [[maybe_unused]] auto error = ERROR;
  [[maybe_unused]] auto warn = WARN;
  [[maybe_unused]] auto info = INFO;
  [[maybe_unused]] auto debug = DEBUG;

  CHECK(critical == ns_log::Level::CRITICAL);
  CHECK(error == ns_log::Level::ERROR);
}

TEST_CASE("ns_log::set_level changes logging level")
{
  ns_log::set_level(ns_log::Level::DEBUG);

  CHECK(ns_log::get_level() == ns_log::Level::DEBUG);

  // Reset to default
  ns_log::set_level(ns_log::Level::CRITICAL);
}

TEST_CASE("ns_log::get_level returns current level")
{
  ns_log::set_level(ns_log::Level::INFO);

  CHECK(ns_log::get_level() == ns_log::Level::INFO);

  // Reset to default
  ns_log::set_level(ns_log::Level::CRITICAL);
}

TEST_CASE("ns_log::set_sink_file configures log output")
{
  fs::path log_file = fs::temp_directory_path() / "test_log.txt";
  // This should not throw
  ns_log::set_sink_file(log_file);
  // Cleanup
  REQUIRE(fs::exists(log_file));
  fs::remove(log_file);
}

TEST_CASE("ns_log::Location captures file and line")
{
  ns_log::Location loc;

  // Location should have captured something
  auto str = loc.get();
  CHECK_FALSE(str.empty());
  CHECK(str.find("::") != std::string::npos);
}

TEST_CASE("ns_log::Location formats as file::line")
{
  ns_log::Location loc;
  auto formatted = loc.get();

  // Should contain :: separator
  CHECK(formatted.find("::") != std::string::npos);
}

TEST_CASE("Logger writes to file when configured")
{
  fs::path log_file = fs::temp_directory_path() / "test_logger_write.log";

  // Configure logger
  ns_log::set_sink_file(log_file);
  ns_log::set_level(ns_log::Level::INFO);

  // Write a log message using the logger macro
  logger("I::Test log message");

  // Give it a moment to flush
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Check file exists and has content
  REQUIRE(fs::exists(log_file));
  CHECK(fs::file_size(log_file) > 0);

  // Cleanup
  fs::remove(log_file);
}

TEST_CASE("Logger supports different log level prefixes")
{
  fs::path log_file = fs::temp_directory_path() / "test_logger_prefixes.log";

  ns_log::set_sink_file(log_file);
  ns_log::set_level(ns_log::Level::DEBUG);

  // Test different log levels
  logger("D::Debug message");
  logger("I::Info message");
  logger("W::Warning message");
  logger("E::Error message");
  logger("C::Critical message");

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Check file has content
  REQUIRE(fs::exists(log_file));
  CHECK(fs::file_size(log_file) > 0);

  // Cleanup
  fs::remove(log_file);
  ns_log::set_level(ns_log::Level::CRITICAL);
}

TEST_CASE("Logger supports format arguments")
{
  fs::path log_file = fs::temp_directory_path() / "test_logger_format.log";

  ns_log::set_sink_file(log_file);

  // Log with format arguments
  int value = 42;
  std::string text = "test";
  logger("I::Value is {} and text is {}", value, text);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Verify file exists
  REQUIRE(fs::exists(log_file));

  // Read file content
  std::ifstream file(log_file);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  // Check that formatted values appear in log
  CHECK(content.find("42") != std::string::npos);
  CHECK(content.find("test") != std::string::npos);

  // Cleanup
  fs::remove(log_file);
}

TEST_CASE("Logger quiet mode discards messages")
{
  fs::path log_file = fs::temp_directory_path() / "test_logger_quiet.log";

  ns_log::set_sink_file(log_file);

  // Q:: prefix means quiet/discard
  logger("Q::This should be discarded");

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  CHECK_EQ(fs::file_size(log_file), 0);

  // Cleanup if it exists
  if (fs::exists(log_file))
  {
    fs::remove(log_file);
  }
}