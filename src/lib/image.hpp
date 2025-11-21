/**
 * @file image.hpp
 * @author Ruan Formigoni
 * @brief A library for operations on image files
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <boost/gil.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>

#include "env.hpp"
#include "log.hpp"
#include "subprocess.hpp"
#include "../std/enum.hpp"

namespace ns_image
{

namespace
{

namespace fs = std::filesystem;

ENUM(ImageFormat, JPG, PNG);

/**
 * @brief Internal implementation for resizing an image
 *
 * Reads an image file, determines its format (JPG or PNG), and resizes it
 * using ImageMagick to the specified dimensions. The resize maintains aspect
 * ratio by using the larger dimension (width or height) as the constraint.
 *
 * @param path_file_src Path to the source image file
 * @param path_file_dst Path to the destination image file
 * @param width Target width for the resized image
 * @param height Target height for the resized image
 * @return Value<void> Nothing on success, or error if file doesn't exist,
 *         format is invalid, or ImageMagick resize fails
 */
inline Value<void> resize_impl(fs::path const& path_file_src
  , fs::path const& path_file_dst
  , uint32_t width
  , uint32_t height)
{
  namespace gil = boost::gil;

  // Create icon directory and set file name
  logger("I::Reading image {}", path_file_src);
  return_if(not fs::is_regular_file(path_file_src)
    , Error("E::File '{}' does not exist or is not a regular file", path_file_src)
  );

  // Determine image format
  std::string ext = path_file_src.extension().string();
  std::ranges::transform(ext, ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

  ImageFormat format = Pop(
    (ext == ".jpg" || ext == ".jpeg") ? Value<ImageFormat>(ImageFormat::JPG) :
    (ext == ".png") ? Value<ImageFormat>(ImageFormat::PNG) :
    Error("E::Input image of invalid format: '{}'", ext)
  );

  // Read image into memory
  gil::rgba8_image_t img;
  if (format == ImageFormat::JPG)
  {
    gil::read_and_convert_image(path_file_src, img, gil::jpeg_tag());
  }
  else if (format == ImageFormat::PNG)
  {
    gil::read_and_convert_image(path_file_src, img, gil::png_tag());
  }
  logger("I::Image size is {}x{}", std::to_string(img.width()), std::to_string(img.height()));
  logger("I::Saving image to {}", path_file_dst);
  // Search for imagemagick
  fs::path path_bin_magick = Pop(ns_env::search_path("magick"));
  // Resize
  Pop(ns_subprocess::Subprocess(path_bin_magick)
    .with_args(path_file_src)
    .with_args("-resize", (img.width() > img.height())? std::format("{}x", width) : std::format("x{}", height))
    .with_args(path_file_dst)
    .spawn()->wait());
  return {};
}

} // namespace




/**
 * @brief Resizes an input image to the specified width and height
 *
 * @param path_file_src Path to the input image file
 * @param path_file_dst Path to the output image file
 * @param width Target width of the output image
 * @param height Target height of the output image
 * @return Value<void> Nothing on success, or the respective error
 */
inline Value<void> resize(fs::path const& path_file_src
  , fs::path const& path_file_dst
  , uint32_t width
  , uint32_t height)
{
  return resize_impl(path_file_src, path_file_dst, width, height);
}

} // namespace ns_image

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
