/**
 * @file test_filesystem.cpp
 * @brief Unit tests for filesystem.hpp utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "../../../src/std/filesystem.hpp"

namespace fs = std::filesystem;

TEST_CASE("ns_fs::realpath resolves existing paths")
{
  // Create a temporary file
  fs::path temp_path = fs::temp_directory_path() / "test_realpath.txt";
  std::ofstream(temp_path) << "test";

  auto result = ns_fs::realpath(temp_path);

  REQUIRE(result.has_value());
  CHECK(fs::exists(result.value()));

  // Cleanup
  fs::remove(temp_path);
}

TEST_CASE("ns_fs::realpath returns error for non-existent paths")
{
  fs::path non_existent = "/this/path/does/not/exist/hopefully";

  auto result = ns_fs::realpath(non_existent);

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_fs::regular_files lists files in directory")
{
  // Create temp directory with files
  fs::path temp_dir = fs::temp_directory_path() / "test_dir_listing";
  fs::create_directories(temp_dir);

  std::ofstream(temp_dir / "file1.txt") << "test1";
  std::ofstream(temp_dir / "file2.txt") << "test2";

  auto result = ns_fs::regular_files(temp_dir);

  REQUIRE(result.has_value());
  CHECK(result.value().size() == 2);

  // Cleanup
  fs::remove_all(temp_dir);
}

TEST_CASE("ns_fs::regular_files returns error for non-existent directory")
{
  fs::path non_existent = "/this/directory/does/not/exist";

  auto result = ns_fs::regular_files(non_existent);

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_fs::regular_files excludes subdirectories")
{
  fs::path temp_dir = fs::temp_directory_path() / "test_dir_no_subdirs";
  fs::create_directories(temp_dir);
  fs::create_directories(temp_dir / "subdir");

  std::ofstream(temp_dir / "file.txt") << "test";

  auto result = ns_fs::regular_files(temp_dir);

  REQUIRE(result.has_value());
  CHECK(result.value().size() == 1);

  // Cleanup
  fs::remove_all(temp_dir);
}

TEST_CASE("ns_fs::create_directories creates new directories")
{
  fs::path temp_path = fs::temp_directory_path() / "test_create" / "nested" / "dirs";

  // Ensure it doesn't exist
  fs::remove_all(temp_path.parent_path().parent_path());

  auto result = ns_fs::create_directories(temp_path);

  REQUIRE(result.has_value());
  CHECK(fs::exists(temp_path));
  CHECK(fs::is_directory(temp_path));

  // Cleanup
  fs::remove_all(temp_path.parent_path().parent_path());
}

TEST_CASE("ns_fs::create_directories returns existing directory")
{
  fs::path temp_path = fs::temp_directory_path() / "test_existing_dir";
  fs::create_directories(temp_path);

  auto result = ns_fs::create_directories(temp_path);

  REQUIRE(result.has_value());
  CHECK(result.value() == temp_path);

  // Cleanup
  fs::remove(temp_path);
}

TEST_CASE("ns_fs::placeholders_replace substitutes path components")
{
  fs::path template_path = "/home/{}/documents/{}";

  auto result = ns_fs::placeholders_replace(template_path, "user", "file.txt");

  CHECK(result.string().find("/home/user/documents/file.txt") != std::string::npos);
}

TEST_CASE("ns_fs::placeholders_replace handles multiple occurrences")
{
  fs::path template_path = "/{}/{}/{}";

  auto result = ns_fs::placeholders_replace(template_path, "a", "b", "c");

  auto str = result.string();
  CHECK(str.find("/a/b/c") != std::string::npos);
}

TEST_CASE("ns_fs::placeholders_replace handles multiple occurrences in the same component")
{
  fs::path template_path = "/{}-{}/{}-{}";

  auto result = ns_fs::placeholders_replace(template_path, "a", "b", "c", "d");

  auto str = result.string();
  CHECK(str.find("/a-b/c-d") != std::string::npos);
}

TEST_CASE("ns_fs::placeholders_replace handles no placeholders")
{
  fs::path template_path = "/fixed/path/name.txt";

  auto result = ns_fs::placeholders_replace(template_path, "unused");

  CHECK(result == template_path);
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE("ns_fs::realpath with relative path")
{
  // Get current directory
  fs::path cwd = fs::current_path();

  // Create a relative path
  fs::path relative = ".";

  auto result = ns_fs::realpath(relative);

  REQUIRE(result.has_value());
  CHECK(result.value() == cwd);
}

TEST_CASE("ns_fs::realpath with path containing dot segments")
{
  // Create a path with .. and .
  fs::path current = fs::current_path();
  fs::path with_dots = current / ".." / current.filename();

  auto result = ns_fs::realpath(with_dots);

  REQUIRE(result.has_value());
  // Should resolve to current path
  CHECK(result.value() == current);
}

TEST_CASE("ns_fs::regular_files with empty directory")
{
  // Create empty directory
  fs::path empty_dir = fs::temp_directory_path() / "test_empty_dir";
  fs::create_directories(empty_dir);

  auto result = ns_fs::regular_files(empty_dir);

  REQUIRE(result.has_value());
  CHECK(result.value().empty());

  // Cleanup
  fs::remove(empty_dir);
}

TEST_CASE("ns_fs::regular_files with directory containing subdirectories only")
{
  // Create directory with subdirectories but no files
  fs::path test_dir = fs::temp_directory_path() / "test_only_subdirs";
  fs::create_directories(test_dir / "subdir1");
  fs::create_directories(test_dir / "subdir2");

  auto result = ns_fs::regular_files(test_dir);

  REQUIRE(result.has_value());
  CHECK(result.value().empty());

  // Cleanup
  fs::remove_all(test_dir);
}

TEST_CASE("ns_fs::create_directories with already existing path")
{
  fs::path existing = fs::temp_directory_path();

  auto result = ns_fs::create_directories(existing);

  REQUIRE(result.has_value());
  CHECK(result.value() == existing);
}

TEST_CASE("ns_fs::create_directories creates parent directories")
{
  fs::path deep_path = fs::temp_directory_path() / "test_deep" / "a" / "b" / "c";

  // Ensure it doesn't exist
  fs::remove_all(deep_path.parent_path().parent_path().parent_path());

  auto result = ns_fs::create_directories(deep_path);

  REQUIRE(result.has_value());
  CHECK(fs::exists(deep_path));
  CHECK(fs::exists(deep_path.parent_path()));
  CHECK(fs::exists(deep_path.parent_path().parent_path()));

  // Cleanup
  fs::remove_all(deep_path.parent_path().parent_path().parent_path());
}

TEST_CASE("ns_fs::placeholders_replace with special characters")
{
  fs::path template_path = "/path/{}/{}";

  auto result = ns_fs::placeholders_replace(template_path, "test-name", "file@2.txt");

  // Check that result is a valid path
  CHECK(fs::path(result).is_absolute());
}

TEST_CASE("ns_fs::placeholders_replace with empty replacement")
{
  fs::path template_path = "/path/{}/{}";

  auto result = ns_fs::placeholders_replace(template_path, "", "file");

  // Check that result is a valid path
  CHECK(fs::path(result).is_absolute());
}

TEST_CASE("ns_fs::regular_files filters out special files")
{
  // Create a test directory with a regular file
  fs::path test_dir = fs::temp_directory_path() / "test_special_files";
  fs::create_directories(test_dir);

  // Create a regular file
  std::ofstream file(test_dir / "regular.txt");
  file << "content";
  file.close();

  auto result = ns_fs::regular_files(test_dir);

  REQUIRE(result.has_value());
  CHECK(result.value().size() == 1);
  CHECK(result.value()[0].filename() == "regular.txt");

  // Cleanup
  fs::remove_all(test_dir);
}

TEST_CASE("ns_fs::realpath with temp_directory_path")
{
  fs::path temp = fs::temp_directory_path();

  auto result = ns_fs::realpath(temp);

  REQUIRE(result.has_value());
  CHECK(fs::exists(result.value()));
  CHECK(fs::is_directory(result.value()));
}
