/**
 * @file ciopfs.hpp
 * @author Ruan Formigoni
 * @brief Case-insensitive filesystem management using CIOPFS
 *
 * This module provides a C++ wrapper around CIOPFS (Case Insensitive On Purpose File System),
 * a FUSE filesystem that provides case-insensitive access to an underlying case-sensitive
 * filesystem. This is particularly useful for running Windows applications under Wine or
 * for compatibility with case-insensitive filesystems.
 *
 * CIOPFS works by maintaining a mapping between the requested filename and the actual
 * filename on disk, allowing applications to access files regardless of case differences.
 *
 * @see https://www.brain-dump.org/projects/ciopfs/
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <unistd.h>

#include "../lib/subprocess.hpp"
#include "../lib/env.hpp"
#include "../std/filesystem.hpp"
#include "filesystem.hpp"

/**
 * @namespace ns_filesystems::ns_ciopfs
 * @brief Case-insensitive filesystem implementation
 *
 * This namespace contains the CIOPFS filesystem wrapper that allows mounting
 * a directory with case-insensitive semantics over a case-sensitive filesystem.
 */
namespace ns_filesystems::ns_ciopfs
{


namespace
{

namespace fs = std::filesystem;

} // namespace

/**
 * @class Ciopfs
 * @brief FUSE-based case-insensitive filesystem wrapper
 *
 * The Ciopfs class provides a managed interface to the CIOPFS filesystem.
 * It handles the lifecycle of the CIOPFS process, including mounting,
 * monitoring, and cleanup.
 *
 * The filesystem creates a case-insensitive view of a source directory,
 * allowing files to be accessed regardless of case. For example, "File.txt",
 * "file.txt", and "FILE.TXT" would all refer to the same file.
 *
 * @warning CIOPFS requires FUSE support in the kernel
 * @note This is a final class and cannot be inherited
 */
class Ciopfs final : public ns_filesystem::Filesystem
{
  private:
    /**
     * @brief Path to the upper (mount) directory
     *
     * This is where the case-insensitive filesystem will be mounted.
     * Applications access files through this directory.
     */
    fs::path m_path_dir_upper;

    /**
     * @brief Path to the lower (source) directory
     *
     * This is the underlying case-sensitive directory that contains
     * the actual files. CIOPFS reads from and writes to this directory.
     */
    fs::path m_path_dir_lower;

  public:
    Ciopfs(pid_t pid_to_die_for
      , fs::path const& path_dir_lower
      , fs::path const& path_dir_upper
      , fs::path const& path_file_log);
    Value<void> mount() override;
};

    /**
     * @brief Constructs and mounts a CIOPFS filesystem
     *
     * Creates a new CIOPFS instance and immediately attempts to mount it.
     * The constructor will log an error if mounting fails but will not throw.
     *
     * @param pid_to_die_for Process ID that this filesystem should monitor.
     *                       When this process dies, the filesystem will unmount.
     * @param path_dir_lower Source directory containing the actual files (case-sensitive)
     * @param path_dir_upper Mount point where files will be accessible (case-insensitive)
     *
     * @note The directories will be created if they don't exist
     */
inline Ciopfs::Ciopfs(pid_t pid_to_die_for
  , fs::path const& path_dir_lower
  , fs::path const& path_dir_upper
  , fs::path const& path_file_log
)
  : ns_filesystem::Filesystem(pid_to_die_for, path_dir_upper, path_file_log)
  , m_path_dir_upper(path_dir_upper)
  , m_path_dir_lower(path_dir_lower)
{
  this->mount().discard("E::Could not mount ciopfs filesystem from '{}' to '{}'", path_dir_lower, path_dir_upper);
}

/**
 * @brief Mounts the CIOPFS filesystem
 *
 * Performs the following steps:
 * 1. Creates source and mount directories if they don't exist
 * 2. Locates the CIOPFS binary in the system PATH
 * 3. Spawns the CIOPFS process with appropriate arguments
 * 4. Waits for FUSE to confirm the mount is ready
 *
 * The spawned CIOPFS process will automatically terminate when the
 * process specified by m_pid_to_die_for exits.
 *
 * @return Value<void> Success or error with description
 *
 * @note This is a synchronous operation that blocks until mount is ready
 */
inline Value<void> Ciopfs::mount()
{
  // Validate directories
  Pop(ns_fs::create_directories(m_path_dir_lower), "E::Failed to create lower directory");
  Pop(ns_fs::create_directories(m_path_dir_upper), "E::Failed to create upper directory");
  // Find Ciopfs
  auto path_file_ciopfs = Pop(ns_env::search_path("ciopfs"), "E::Could not find ciopfs in PATH");
  // Include arguments and spawn process
  m_child = ns_subprocess::Subprocess(path_file_ciopfs)
    .with_args(m_path_dir_lower, m_path_dir_upper)
    .with_die_on_pid(m_pid_to_die_for)
    .with_stdio(ns_subprocess::Stream::Pipe)
    .with_log_file(m_path_file_log)
    .spawn();
  // Wait for mount
  ns_fuse::wait_fuse(m_path_dir_upper);
  return {};
}

} // namespace ns_filesystems::ns_ciopfs

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
