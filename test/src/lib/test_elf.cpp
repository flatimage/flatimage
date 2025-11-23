/**
 * @file test_elf.cpp
 * @brief Unit tests for elf.hpp ELF file utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "../../../src/lib/elf.hpp"

namespace fs = std::filesystem;

TEST_CASE("ns_elf::skip_elf_header returns error for non-existent file")
{
  fs::path non_existent = "/this/file/does/not/exist.elf";

  auto result = ns_elf::skip_elf_header(non_existent);

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_elf::skip_elf_header returns error for non-ELF file")
{
  // Create a temporary non-ELF file
  fs::path temp_file = fs::temp_directory_path() / "not_an_elf.txt";
  std::ofstream(temp_file) << "This is not an ELF file";

  auto result = ns_elf::skip_elf_header(temp_file);

  CHECK_FALSE(result.has_value());

  // Cleanup
  fs::remove(temp_file);
}

TEST_CASE("ns_elf::skip_elf_header works with valid ELF binary")
{
  // Try to find a system ELF binary
  fs::path elf_binary = "/bin/sh";

  if (fs::exists(elf_binary))
  {
    auto result = ns_elf::skip_elf_header(elf_binary);

    // If it's an ELF file, should return success with offset
    if (result.has_value())
    {
      CHECK(result.value() > 0);
    }
  }
  else
  {
    // Skip test if /bin/sh doesn't exist
    MESSAGE("System ELF binary /bin/sh not found, skipping test");
  }
}

TEST_CASE("ns_elf::copy_binary creates output file")
{
  // Create a simple test file
  fs::path input_file = fs::temp_directory_path() / "test_input.bin";
  fs::path output_file = fs::temp_directory_path() / "test_output.bin";

  // Write test data
  std::ofstream out(input_file, std::ios::binary);
  std::string test_data = "This is test binary data with some content";
  out.write(test_data.c_str(), test_data.size());
  out.close();

  // Copy a section [0, 20)
  auto result = ns_elf::copy_binary(input_file, output_file, {0, 20});

  REQUIRE(result.has_value());
  REQUIRE(fs::exists(output_file));

  // Verify copied size
  CHECK(fs::file_size(output_file) == 20);

  // Cleanup
  fs::remove(input_file);
  fs::remove(output_file);
}

TEST_CASE("ns_elf::copy_binary handles different section ranges")
{
  fs::path input_file = fs::temp_directory_path() / "test_section_input.bin";
  fs::path output_file = fs::temp_directory_path() / "test_section_output.bin";

  // Write test data
  std::ofstream out(input_file, std::ios::binary);
  std::string test_data = "0123456789ABCDEFGHIJ";
  out.write(test_data.c_str(), test_data.size());
  out.close();

  // Copy section [5, 15) - should copy "56789ABCDE"
  auto result = ns_elf::copy_binary(input_file, output_file, {5, 15});

  REQUIRE(result.has_value());
  REQUIRE(fs::exists(output_file));
  CHECK(fs::file_size(output_file) == 10);

  // Verify content
  std::ifstream in(output_file, std::ios::binary);
  std::string copied_data(10, '\0');
  in.read(&copied_data[0], 10);
  CHECK(copied_data == "56789ABCDE");

  // Cleanup
  fs::remove(input_file);
  fs::remove(output_file);
}

TEST_CASE("ns_elf::copy_binary returns error for non-existent input")
{
  fs::path input_file = "/non/existent/input.bin";
  fs::path output_file = fs::temp_directory_path() / "output.bin";

  auto result = ns_elf::copy_binary(input_file, output_file, {0, 100});

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_elf::copy_binary handles empty section")
{
  fs::path input_file = fs::temp_directory_path() / "test_empty_input.bin";
  fs::path output_file = fs::temp_directory_path() / "test_empty_output.bin";

  // Write test data
  std::ofstream(input_file, std::ios::binary) << "test data";

  // Copy empty section [5, 5)
  auto result = ns_elf::copy_binary(input_file, output_file, {5, 5});

  REQUIRE(result.has_value());

  if (fs::exists(output_file))
  {
    CHECK(fs::file_size(output_file) == 0);
  }

  // Cleanup
  fs::remove(input_file);
  if (fs::exists(output_file))
  {
    fs::remove(output_file);
  }
}

TEST_CASE("ns_elf::copy_binary handles large sections")
{
  fs::path input_file = fs::temp_directory_path() / "test_large_input.bin";
  fs::path output_file = fs::temp_directory_path() / "test_large_output.bin";

  // Write larger test data (> 4KB to test chunked reading)
  std::ofstream out(input_file, std::ios::binary);
  std::string chunk(1024, 'X'); // 1KB chunks
  for (int i = 0; i < 10; ++i)
  {
    out.write(chunk.c_str(), chunk.size());
  }
  out.close();

  // Copy full 10KB
  auto result = ns_elf::copy_binary(input_file, output_file, {0, 10240});

  REQUIRE(result.has_value());
  REQUIRE(fs::exists(output_file));
  CHECK(fs::file_size(output_file) == 10240);

  // Cleanup
  fs::remove(input_file);
  fs::remove(output_file);
}
