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
#include <format>

#include "env.hpp"
#include "log.hpp"
#include "match.hpp"
#include "subprocess.hpp"

namespace ns_image
{

namespace
{

namespace fs = std::filesystem;

inline Expected<void> resize_impl(fs::path const& path_file_src
  , fs::path const& path_file_dst
  , uint32_t width
  , uint32_t height)
{
  namespace gil = boost::gil;

  // Create icon directory and set file name
  ns_log::info()("Reading image {}", path_file_src);
  qreturn_if(not fs::is_regular_file(path_file_src), std::unexpected("File '{}' does not exist or is not a regular file"_fmt(path_file_src)));
  // Read image into memory
  gil::rgba8_image_t img;
  Expect(ns_match::match(path_file_src.extension()
      , ns_match::equal(".jpg", ".jpeg") >>= [&]{ gil::read_and_convert_image(path_file_src, img, gil::jpeg_tag()); }
      , ns_match::equal(".png") >>= [&]{ gil::read_and_convert_image(path_file_src, img, gil::png_tag()); }
    ).transform_error([](auto&& e){ return std::vformat("Input image of invalid format '{}': '{}'", std::make_format_args(e)); })
  );
  ns_log::info()("Image size is {}x{}", std::to_string(img.width()), std::to_string(img.height()));
  ns_log::info()("Saving image to {}", path_file_dst);
  // Search for imagemagick
  fs::path path_bin_magick = Expect(ns_env::search_path("magick"));
  // Resize
  int code = Expect(ns_subprocess::Subprocess(path_bin_magick)
    .with_args(path_file_src)
    .with_args("-resize", (img.width() > img.height())? "{}x"_fmt(width) : "x{}"_fmt(height))
    .with_args(path_file_dst)
    .spawn()
    .wait()
  );
  qreturn_if(code != 0, std::unexpected("Failure to resize image with code {}"_fmt(code)))
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
 * @param is_preserve_aspect_ratio Whether to preserve the aspect ratio or not in the output
 * @return Expected<void> Nothing on success, or the respective error
 */
inline Expected<void> resize(fs::path const& path_file_src
  , fs::path const& path_file_dst
  , uint32_t width
  , uint32_t height)
{
  return resize_impl(path_file_src, path_file_dst, width, height);
}

} // namespace ns_image

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
