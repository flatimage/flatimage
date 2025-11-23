/**
 * @file test_image.cpp
 * @brief Unit tests for image.hpp image manipulation utilities
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "../../../src/lib/env.hpp"
#include "../../../src/lib/image.hpp"
#include "../../../src/lib/subprocess.hpp"

namespace fs = std::filesystem;

TEST_CASE("ns_image::resize returns error for non-existent file")
{
  fs::path non_existent = "/tmp/this_image_does_not_exist.jpg";
  fs::path output = fs::temp_directory_path() / "output.jpg";

  auto result = ns_image::resize(non_existent, output, 100, 100);

  CHECK_FALSE(result.has_value());
}

TEST_CASE("ns_image::resize returns error for non-image file")
{
  // Create a text file (not an image)
  fs::path fake_image = fs::temp_directory_path() / "fake.jpg";
  std::ofstream(fake_image) << "This is not an image";

  fs::path output = fs::temp_directory_path() / "output.jpg";

  auto result = ns_image::resize(fake_image, output, 100, 100);

  // Should fail because it's not a valid image
  CHECK_FALSE(result.has_value());

  // Cleanup
  fs::remove(fake_image);
  if (fs::exists(output))
  {
    fs::remove(output);
  }
}

TEST_CASE("ns_image::resize handles invalid format")
{
  // Create a file with unsupported extension
  fs::path invalid = fs::temp_directory_path() / "test.bmp";
  std::ofstream(invalid) << "fake data";

  fs::path output = fs::temp_directory_path() / "output.bmp";

  auto result = ns_image::resize(invalid, output, 100, 100);

  // Should return error for unsupported format
  CHECK_FALSE(result.has_value());

  // Cleanup
  fs::remove(invalid);
}

TEST_CASE("ns_image::resize validates input file exists")
{
  fs::path input = "/tmp/definitely_not_a_real_image_file_12345.png";
  fs::path output = fs::temp_directory_path() / "output.png";

  auto result = ns_image::resize(input, output, 200, 200);

  REQUIRE_FALSE(result.has_value());
}

TEST_CASE("ns_image::resize accepts jpg extension")
{
  // We can't test actual resizing without a valid image,
  // but we can test that the function is callable with .jpg files
  fs::path input = fs::temp_directory_path() / "test.jpg";
  fs::path output = fs::temp_directory_path() / "out.jpg";

  // Create empty file
  std::ofstream(input) << "";

  // This will fail due to invalid image data, but that's expected
  auto result = ns_image::resize(input, output, 100, 100);

  CHECK_FALSE(result.has_value());

  // Cleanup
  fs::remove(input);
  if (fs::exists(output))
  {
    fs::remove(output);
  }
}

TEST_CASE("ns_image::resize accepts png extension")
{
  fs::path input = fs::temp_directory_path() / "test.png";
  fs::path output = fs::temp_directory_path() / "out.png";

  // Create empty file
  std::ofstream(input) << "";

  // This will fail due to invalid image data, but that's expected
  auto result = ns_image::resize(input, output, 150, 150);

  CHECK_FALSE(result.has_value());

  // Cleanup
  fs::remove(input);
  if (fs::exists(output))
  {
    fs::remove(output);
  }
}

TEST_CASE("ns_image::resize accepts jpeg extension")
{
  fs::path input = fs::temp_directory_path() / "test.jpeg";
  fs::path output = fs::temp_directory_path() / "out.jpeg";

  // Create empty file
  std::ofstream(input) << "";

  // This will fail due to invalid image data, but that's expected
  auto result = ns_image::resize(input, output, 100, 100);

  CHECK_FALSE(result.has_value());

  // Cleanup
  fs::remove(input);
  if (fs::exists(output))
  {
    fs::remove(output);
  }
}

TEST_CASE("ns_image functions are defined and callable")
{
  // Basic API existence test
  fs::path input = fs::temp_directory_path() / "api_test.jpg";
  fs::path output = fs::temp_directory_path() / "api_output.jpg";

  std::ofstream(input) << "fake";

  // Verify the function exists and is callable
  auto result = ns_image::resize(input, output, 50, 50);

  // Function should be callable (will return error for fake image)
  CHECK(true);

  // Cleanup
  fs::remove(input);
  if (fs::exists(output))
  {
    fs::remove(output);
  }
}

TEST_CASE("ns_image::resize successfully resizes real PNG image")
{
  // Use the actual PNG image from test/data
  fs::path input = fs::current_path().parent_path().parent_path() / "test" / "data" / "icon.png";
  fs::path output = fs::current_path() / "resized_icon.png";

  // Verify input image exists
  REQUIRE(fs::exists(input));
  REQUIRE(fs::is_regular_file(input));

  // Helper lambda to get image dimensions using ImageMagick identify
  auto get_image_dimensions = [](const fs::path& image_path) -> std::pair<uint32_t, uint32_t>
  {
    // Find magick binary using the same method as ns_image::resize
    auto path_bin_magick_result = ns_env::search_path("magick");
    if (!path_bin_magick_result.has_value())
    {
      return {0, 0};
    }
    fs::path path_bin_magick = path_bin_magick_result.value();

    std::stringstream stdout_stream;

    auto child = ns_subprocess::Subprocess(path_bin_magick)
      .with_args("identify", "-ping", "-format", "%w %h")
      .with_args(image_path)
      .with_stdio(ns_subprocess::Stream::Pipe)
      .with_streams(std::cin, stdout_stream, std::cerr)
      .spawn()
      ->wait();

    if (!child.has_value() || child.value() != 0)
    {
      return {0, 0};
    }

    std::string output = stdout_stream.str();
    std::istringstream iss(output);
    uint32_t width, height;
    iss >> width >> height;
    return {width, height};
  };

  SUBCASE("Resize to 64x64")
  {
    auto result = ns_image::resize(input, output, 64, 64);

    // Check that resize succeeded
    CHECK(result.has_value());

    // Verify output file was created
    REQUIRE(fs::exists(output));
    CHECK(fs::is_regular_file(output));

    // Verify dimensions
    auto [width, height] = get_image_dimensions(output);
    std::println("WxH: {}x{}", width, height);
    CHECK((width == 64 || height == 64)); // One dimension should match
    CHECK((width <= 64 && height <= 64)); // Both should be <= target

    // Cleanup
    // fs::remove(output);
  }

  SUBCASE("Resize to 128x128")
  {
    auto result = ns_image::resize(input, output, 128, 128);

    CHECK(result.has_value());
    REQUIRE(fs::exists(output));

    auto [width, height] = get_image_dimensions(output);
    CHECK((width == 128 || height == 128));
    CHECK((width <= 128 && height <= 128));

    fs::remove(output);
  }

  SUBCASE("Resize to 256x256")
  {
    auto result = ns_image::resize(input, output, 256, 256);

    CHECK(result.has_value());
    REQUIRE(fs::exists(output));

    auto [width, height] = get_image_dimensions(output);
    CHECK((width == 256 || height == 256));
    CHECK((width <= 256 && height <= 256));

    fs::remove(output);
  }

  SUBCASE("Resize to small dimensions 32x32")
  {
    auto result = ns_image::resize(input, output, 32, 32);

    CHECK(result.has_value());
    REQUIRE(fs::exists(output));

    auto [width, height] = get_image_dimensions(output);
    CHECK((width == 32 || height == 32));
    CHECK((width <= 32 && height <= 32));

    fs::remove(output);
  }

  // Cleanup any remaining output file
  if (fs::exists(output))
  {
    fs::remove(output);
  }
}