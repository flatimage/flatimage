/**
 * @file env.hpp
 * @author Ruan Formigoni
 * @brief Manages the environment reserved space
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <filesystem>

#include "../std/expected.hpp"
#include "../macro.hpp"
#include "reserved.hpp"

/**
 * @namespace ns_reserved::ns_env
 * @brief Environment variable storage in reserved space
 *
 * This namespace manages environment variables stored as a JSON string in the binary's
 * reserved space. It allows configuration of environment variables
 * that are set when the containerized application is launched, enabling customization
 * of runtime behavior, library paths, configuration options, and other environment-based
 * settings without rebuilding the binary.
 */
namespace ns_reserved::ns_env
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Writes the environment json string to the target binary
 *
 * @param path_file_binary Target binary to write the json string
 * @param json Json string to write to the target file as binary data
 * @return Value<void> Nothing on success, or the respective error message
 */
inline Value<void> write(fs::path const& path_file_binary, std::string_view const& json)
{
  uint64_t space_available = ns_reserved::FIM_RESERVED_OFFSET_ENVIRONMENT_END - ns_reserved::FIM_RESERVED_OFFSET_ENVIRONMENT_BEGIN;
  uint64_t space_required = json.size();
  return_if(space_available <= space_required, Error("E::Not enough space to fit json data"));
  Pop(ns_reserved::write(path_file_binary
    , FIM_RESERVED_OFFSET_ENVIRONMENT_BEGIN
    , FIM_RESERVED_OFFSET_ENVIRONMENT_END
    , json.data()
    , json.size()
  ), "E::Failed to write data to {}", path_file_binary);
  return {};
}

/**
 * @brief Reads the environment json string from the target binary
 *
 * @param path_file_binary Target binary to write the json string
 * @return On success it returns the read data, or the respective error message
 */
inline Value<std::string> read(fs::path const& path_file_binary)
{
  uint64_t offset_begin = ns_reserved::FIM_RESERVED_OFFSET_ENVIRONMENT_BEGIN;
  uint64_t size = ns_reserved::FIM_RESERVED_OFFSET_ENVIRONMENT_END - offset_begin;
  auto buffer = std::make_unique<char[]>(size);
  Pop(ns_reserved::read(path_file_binary, offset_begin, buffer.get(), size)
    , "E::Failed to read binary data from {}", path_file_binary
  );
  return buffer.get();
}

} // namespace ns_reserved::ns_env

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
