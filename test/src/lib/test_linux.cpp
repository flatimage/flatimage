/**
 * @file test_linux.cpp
 * @brief Unit tests for linux.hpp Linux system utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <fstream>

#include "../../../src/lib/linux.hpp"

namespace fs = std::filesystem;

TEST_CASE("ns_linux::module_check returns error for invalid module file")
{
  auto result = ns_linux::module_check("definitely_not_loaded_module_12345");
  // Should return a boolean (false if module not found)
  CHECK(result.has_value());
  CHECK_EQ(result.value(), false);
}

TEST_CASE("ns_linux::read_with_timeout reads from file descriptor")
{
  // Create a test file
  std::string_view data{"test data for reading"};
  fs::path test_file = fs::temp_directory_path() / "test_read_timeout.txt";
  std::ofstream(test_file) << data;

  // Open file descriptor
  int fd = open(test_file.c_str(), O_RDONLY);
  REQUIRE(fd >= 0);

  // Read with timeout
  char buffer[100];
  std::span<char> buf_span(buffer, 100);
  auto bytes_read = ns_linux::read_with_timeout(fd, std::chrono::milliseconds(100), buf_span);

  CHECK_EQ(bytes_read, data.size());
  close(fd);

  // Cleanup
  fs::remove(test_file);
}

TEST_CASE("ns_linux::write_with_timeout writes to file descriptor")
{
  fs::path test_file = fs::temp_directory_path() / "test_write_timeout.txt";

  // Open file descriptor for writing
  int fd = open(test_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  REQUIRE(fd >= 0);

  // Write with timeout
  std::string_view data = "test write data";
  std::span<const char> buf_span(data.data(), strlen(data.data()));
  auto bytes_written = ns_linux::write_with_timeout(fd, std::chrono::milliseconds(100), buf_span);

  CHECK_EQ(bytes_written, data.size());
  close(fd);

  // Verify file has content
  REQUIRE(fs::exists(test_file));
  CHECK_EQ(fs::file_size(test_file), data.size());

  // Cleanup
  fs::remove(test_file);
}

TEST_CASE("ns_linux::open_with_timeout opens file")
{
  fs::path test_file = fs::temp_directory_path() / "test_open_timeout.txt";
  std::ofstream(test_file) << "test";

  // Open with timeout
  int fd = ns_linux::open_with_timeout(test_file, std::chrono::milliseconds(100), O_RDONLY);

  REQUIRE(fd >= 0);
  close(fd);

  // Cleanup
  fs::remove(test_file);
}

TEST_CASE("ns_linux::open_with_timeout returns error for non-existent file")
{
  fs::path non_existent = fs::temp_directory_path() / std::format("this_does_not_exist_{}.txt", getpid());

  int fd = ns_linux::open_with_timeout(non_existent, std::chrono::milliseconds(100), O_RDONLY);

  CHECK(fd < 0);
  CHECK_EQ(errno, ENOENT);
}

TEST_CASE("ns_linux::open_read_with_timeout reads file content")
{
  fs::path test_file = fs::temp_directory_path() / "test_open_read.txt";
  std::string_view data = "test content for reading";
  std::ofstream(test_file) << data;

  // Read with timeout
  char buffer[100] = {0};
  std::span<char> buf_span(buffer, 100);
  auto bytes_read =
      ns_linux::open_read_with_timeout(test_file, std::chrono::milliseconds(100), buf_span);

  REQUIRE_EQ(bytes_read, data.size());
  CHECK_EQ(std::string(buffer, bytes_read), data);

  // Cleanup
  fs::remove(test_file);
}

TEST_CASE("ns_linux::open_write_with_timeout writes file content")
{
  fs::path test_file = fs::temp_directory_path() / "test_open_write.txt";
  // Create and close file
  std::ofstream(test_file) << "foo";
  // Write with timeout
  std::string_view data = "write test data";
  std::span<const char> buf_span(data.data(), strlen(data.data()));
  auto bytes_written =
      ns_linux::open_write_with_timeout(test_file, std::chrono::seconds(100), buf_span);
  REQUIRE_EQ(bytes_written, data.size());
  // Verify content
  std::ifstream in(test_file);
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  CHECK_EQ(content, "write test data");
  // Cleanup
  fs::remove(test_file);
}

TEST_CASE("ns_linux::read_with_timeout handles empty files")
{
  fs::path test_file = fs::temp_directory_path() / "test_empty.txt";
  {
    std::ofstream ofs(test_file);
  } // Create empty file

  int fd = open(test_file.c_str(), O_RDONLY);
  REQUIRE(fd >= 0);

  char buffer[100];
  std::span<char> buf_span(buffer, 100);
  auto bytes_read = ns_linux::read_with_timeout(fd, std::chrono::milliseconds(100), buf_span);

  CHECK_EQ(bytes_read, 0);
  close(fd);

  // Cleanup
  fs::remove(test_file);
}
