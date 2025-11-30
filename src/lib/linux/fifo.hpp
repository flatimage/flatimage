/**
 * @file fifo.hpp
 * @author Ruan Formigoni
 * @brief Linux FIFO related operation wrappers
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <sys/stat.h>
#include <cstring>
#include <filesystem>

#include "../../std/expected.hpp"
#include "../../macro.hpp"

namespace ns_linux::ns_fifo
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @brief Create a fifo object
 *
 * @param path_file_fifo Where to saved the fifo to
 * @return Value<fs::path> The path to the created fifo, or the respective error
 */
[[nodiscard]] inline Value<fs::path> create(fs::path const& path_file_fifo)
{
  fs::path path_dir_parent = path_file_fifo.parent_path();
  // Create parent directory(ies)
  return_if(not Try(fs::exists(path_dir_parent)) and not Try(fs::create_directories(path_dir_parent))
    , Error("E::Failed to create upper directories '{}' for fifo '{}'", path_dir_parent, path_file_fifo)
  );
  // Replace old fifo if exists
  if (Try(fs::exists(path_file_fifo)))
  {
    Try(fs::remove(path_file_fifo));
  }
  // Create fifo
  return_if(::mkfifo(path_file_fifo.c_str(), 0666) < 0
    , Error("E::Failed to create fifo '{}' with error '{}'", path_file_fifo, strerror(errno))
  );
  return path_file_fifo;
}


} // namespace ns_linux::ns_fifo