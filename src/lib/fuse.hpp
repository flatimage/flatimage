/**
 * @file fuse.hpp
 * @author Ruan Formigoni
 * @brief A library for operations on fuse filesystems
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstring>
#include <filesystem>
#include <expected>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <thread>

#include "subprocess.hpp"
#include "env.hpp"
#include "../std/expected.hpp"

// Other codes available here:
// https://man7.org/linux/man-pages/man2/statfs.2.html
#define FUSE_SUPER_MAGIC 0x65735546

namespace ns_fuse
{

namespace
{

namespace fs = std::filesystem;

} // namespace 

/**
 * @brief Checks if a directory is a fuse filesystem mount point
 * 
 * @param path_dir_mount Path to the directory to check
 * @return Value<bool> A boolean with the result, or the respective internal error
 */
inline Value<bool> is_fuse(fs::path const& path_dir_mount)
{
  struct statfs buf;

  if ( statfs(path_dir_mount.c_str(), &buf) < 0 )
  {
    return Error("E::{}", strerror(errno));
  }

  return buf.f_type == FUSE_SUPER_MAGIC;
}

/**
 * @brief Waits for the given directory to not be fuse
 * 
 * @param path_dir_filesystem Path to the directory to wait for
 */
inline void wait_fuse(fs::path const& path_dir_filesystem)
{
  using namespace std::chrono_literals;
  auto time_beg = std::chrono::system_clock::now();
  while ( true )
  {
    auto expected_is_fuse = ns_fuse::is_fuse(path_dir_filesystem);
    ebreak_if(not expected_is_fuse, "Could not check if filesystem is fuse");
    dbreak_if( *expected_is_fuse, std::format("Filesystem '{}' is fuse", path_dir_filesystem.string()));
    auto time_cur = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(time_cur - time_beg);
    ebreak_if(elapsed.count() > 60, "Reached timeout to wait for fuse filesystems");
  } // while
} // function: wait_fuse


/**
 * @brief Un-mounts the given fuse mount point
 * 
 * @param path_dir_mount Path to the mount point to un-mount
 * @return Value<void> Nothing on success, or the respective error
 */
inline Value<void> unmount(fs::path const& path_dir_mount)
{
  using namespace std::chrono_literals;

  // Find fusermount
  auto path_file_fusermount = Pop(ns_env::search_path("fusermount"));

  // Un-mount filesystem
  using enum ns_subprocess::Stream;
  int code = ns_subprocess::Subprocess(path_file_fusermount)
    .with_args("-zu", path_dir_mount)
    .with_log_stdio()
    .wait()
    .forward("E::")
    .value_or(-1);


  // Check for successful un-mount
  if(code == 0)
  {
    logger("D::Un-mounted filesystem '{}'", path_dir_mount.string());
  }
  else
  {
    logger("D::Failed to un-mount filesystem '{}'", path_dir_mount.string());
  }

  // Filesystem could be busy for a bit after un-mount
  while(Pop(ns_fuse::is_fuse(path_dir_mount)))
  {
    std::this_thread::sleep_for(100ms);
  }

  return {};
} // function: unmount

} // namespace ns_fuse

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
