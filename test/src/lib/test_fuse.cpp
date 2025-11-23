/**
 * @file test_fuse.cpp
 * @brief Unit tests for fuse.hpp FUSE filesystem utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "../../../src/lib/fuse.hpp"
#include "../test.hpp"

namespace fs = std::filesystem;

TEST_CASE("ns_fuse::is_fuse checks non-fuse type")
{
  // Test with root filesystem (should not be FUSE)
  auto result = ns_fuse::is_fuse("/");
  CHECK(result.has_value());
  // Root filesystem is typically not FUSE
  CHECK_FALSE(result.value());
}

TEST_CASE("ns_fuse::is_fuse handles non-existent paths")
{
  fs::path non_existent = "/this/path/definitely/does/not/exist/12345";

  auto result = ns_fuse::is_fuse(non_existent);

  // Should return error for non-existent path
  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_fuse::is_fuse detects DwarFS FUSE mount")
{
  // Create a temporary source directory with test files
  auto unique_suffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
  fs::path source_dir = fs::temp_directory_path() / ("test_source_" + unique_suffix);
  fs::path image_file = fs::temp_directory_path() / ("test_image_" + unique_suffix + ".dwarfs");

  fs::create_directories(source_dir);

  // Create some test files
  std::ofstream(source_dir / "test.txt") << "Hello, DwarFS!\n";
  std::ofstream(source_dir / "file2.txt") << "Another file\n";

  // Create the DwarFS image
  auto create_result = ns_test::create_dwarfs(source_dir, image_file);
  REQUIRE(create_result.has_value());
  REQUIRE(fs::exists(image_file));

  // Mount the filesystem
  auto mount_result = ns_test::mount_dwarfs(image_file);
  REQUIRE(mount_result.has_value());
  fs::path mount_point = mount_result.value();

  // Check if it's detected as FUSE
  auto is_fuse_result = ns_fuse::is_fuse(mount_point);
  REQUIRE(is_fuse_result.has_value());
  CHECK(is_fuse_result.value());

  // Verify we can access files
  CHECK(fs::exists(mount_point / "test.txt"));
  CHECK(fs::exists(mount_point / "file2.txt"));

  // Cleanup: unmount and remove files
  ns_fuse::unmount(mount_point);
  fs::remove_all(mount_point);
  fs::remove(image_file);
  fs::remove_all(source_dir);
}

TEST_CASE("ns_fuse::unmount successfully unmounts DwarFS filesystem")
{
  // Create test filesystem
  auto unique_suffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
  fs::path source_dir = fs::temp_directory_path() / ("test_source_" + unique_suffix);
  fs::path image_file = fs::temp_directory_path() / ("test_image_" + unique_suffix + ".dwarfs");

  fs::create_directories(source_dir);
  std::ofstream(source_dir / "test.txt") << "Test content\n";

  // Create and mount
  auto create_result = ns_test::create_dwarfs(source_dir, image_file);
  REQUIRE(create_result.has_value());

  auto mount_result = ns_test::mount_dwarfs(image_file);
  REQUIRE(mount_result.has_value());

  fs::path mount_point = mount_result.value();

  // Verify it's mounted
  auto is_fuse_before = ns_fuse::is_fuse(mount_point);
  REQUIRE(is_fuse_before.has_value());
  REQUIRE(is_fuse_before.value());

  // Unmount it
  auto unmount_result = ns_fuse::unmount(mount_point);
  CHECK(unmount_result.has_value());
  CHECK_EQ(unmount_result.value(), 0);

  // Verify it's no longer FUSE
  auto is_fuse_after = ns_fuse::is_fuse(mount_point);
  REQUIRE(is_fuse_after.has_value());
  CHECK_FALSE(is_fuse_after.value());

  // Cleanup
  fs::remove_all(mount_point);
  fs::remove(image_file);
  fs::remove_all(source_dir);
}

TEST_CASE("ns_fuse::unmount handles non-fuse")
{
  fs::path non_fuse = "/usr";

  // Should handle gracefully (might return error or success depending on fusermount behavior)
  auto result = ns_fuse::unmount(non_fuse);
  // Returns a code
  CHECK(result.has_value());
  // The code should be non-zero (failure)
  CHECK_NE(result.value(), 0);
}

TEST_CASE("DwarFS filesystem preserves file contents and structure")
{
  // Create source directory with various files
  auto unique_suffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
  fs::path source_dir = fs::temp_directory_path() / ("test_source_" + unique_suffix);
  fs::path image_file = fs::temp_directory_path() / ("test_image_" + unique_suffix + ".dwarfs");

  fs::create_directories(source_dir);
  fs::create_directories(source_dir / "subdir");

  std::ofstream(source_dir / "file1.txt") << "Content 1\n";
  std::ofstream(source_dir / "file2.txt") << "Content 2\n";
  std::ofstream(source_dir / "subdir" / "nested.txt") << "Nested content\n";

  // Create and mount
  auto create_result = ns_test::create_dwarfs(source_dir, image_file);
  REQUIRE(create_result.has_value());

  auto mount_result = ns_test::mount_dwarfs(image_file);
  REQUIRE(mount_result.has_value());

  fs::path mount_point = mount_result.value();

  // Verify all files exist and have correct content
  SUBCASE("Root level files")
  {
    REQUIRE(fs::exists(mount_point / "file1.txt"));
    REQUIRE(fs::exists(mount_point / "file2.txt"));

    std::ifstream f1(mount_point / "file1.txt");
    std::string content1;
    std::getline(f1, content1);
    CHECK_EQ(content1, "Content 1");

    std::ifstream f2(mount_point / "file2.txt");
    std::string content2;
    std::getline(f2, content2);
    CHECK_EQ(content2, "Content 2");
  }

  SUBCASE("Nested files")
  {
    REQUIRE(fs::exists(mount_point / "subdir"));
    REQUIRE(fs::is_directory(mount_point / "subdir"));
    REQUIRE(fs::exists(mount_point / "subdir" / "nested.txt"));

    std::ifstream f(mount_point / "subdir" / "nested.txt");
    std::string content;
    std::getline(f, content);
    CHECK_EQ(content, "Nested content");
  }

  // Cleanup
  ns_fuse::unmount(mount_point);
  fs::remove_all(mount_point);
  fs::remove(image_file);
  fs::remove_all(source_dir);
}

TEST_CASE("ns_test::create_dwarfs validates inputs")
{
  auto unique_suffix = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

  SUBCASE("Fails with non-existent source directory")
  {
    fs::path non_existent = "/tmp/does_not_exist_" + unique_suffix;
    fs::path output = fs::temp_directory_path() / ("output_" + unique_suffix + ".dwarfs");

    auto result = ns_test::create_dwarfs(non_existent, output);
    CHECK_FALSE(result.has_value());
  }

  SUBCASE("Fails when source is a file, not directory")
  {
    fs::path file_path = fs::temp_directory_path() / ("not_a_dir_" + unique_suffix);
    fs::path output = fs::temp_directory_path() / ("output_" + unique_suffix + ".dwarfs");

    std::ofstream(file_path) << "I am a file\n";

    auto result = ns_test::create_dwarfs(file_path, output);
    CHECK_FALSE(result.has_value());

    fs::remove(file_path);
  }
}

TEST_CASE("ns_test::mount_dwarfs validates inputs")
{
  SUBCASE("Fails with non-existent image file")
  {
    fs::path non_existent = "/tmp/does_not_exist.dwarfs";

    auto result = ns_test::mount_dwarfs(non_existent);
    CHECK_FALSE(result.has_value());
  }
}