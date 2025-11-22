/**
 * @file filesystem.hpp
 * @author Ruan Formigoni
 * @brief The base class for filesystems
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <unistd.h>

#include "../lib/fuse.hpp"

/**
 * @namespace ns_filesystem
 * @brief Base filesystem interface and abstraction
 *
 * Defines the abstract base class for all FlatImage filesystem implementations. Provides
 * common interface for mounting, child process management, logging, and automatic cleanup
 * through RAII. All specific filesystem types (DwarFS, UnionFS, OverlayFS, CIOPFS) inherit
 * from this base.
 */
namespace ns_filesystem
{


class Filesystem
{
  protected:
    pid_t m_pid_to_die_for;
    std::filesystem::path m_path_dir_mount;
    std::filesystem::path m_path_file_log;
    std::unique_ptr<ns_subprocess::Child> m_child;
    Filesystem(pid_t pid_to_die_for
      , std::filesystem::path const& path_dir_mount
      , std::filesystem::path const& path_file_log
    );

  public:
    virtual ~Filesystem();
    [[nodiscard]] virtual Value<void> mount() = 0;
    Filesystem(Filesystem&&) = default;
    Filesystem(Filesystem const&) = delete;
    Filesystem& operator=(Filesystem&&) = default;
    Filesystem& operator=(Filesystem const&) = delete;
};

/**
 * @brief Construct a new Filesystem object
 *
 * @param pid_to_die_for Pid the mount process should die with
 * @param path_dir_mount Path to the mount directory
 * @param path_file_log Path to the log file for filesystem operations
 */
inline Filesystem::Filesystem(pid_t pid_to_die_for
  , std::filesystem::path const& path_dir_mount
  , std::filesystem::path const& path_file_log
)
  : m_pid_to_die_for(pid_to_die_for)
  , m_path_dir_mount(path_dir_mount)
  , m_path_file_log(path_file_log)
  , m_child(nullptr)
{
}

/**
 * @brief Destroy the Filesystem:: Filesystem object, it
 * un-mounts the filesystem and sends a termination signal
 */
inline Filesystem::~Filesystem()
{
  // Un-mount the fuse file system
  ns_fuse::unmount(m_path_dir_mount).discard("E::Could not un-mount filesystem '{}'", m_path_dir_mount);
  // Check for subprocess
  return_if(not m_child,,"E::No fuse sub-process for '{}'", m_path_dir_mount.string());
  // Tell process to exit with SIGTERM
  if (pid_t pid = m_child->get_pid().value_or(-1); pid > 0)
  {
    kill(pid, SIGTERM);
  }
  // Wait for process to exit and log result
  std::ignore = m_child->wait();
}

} // namespace ns_filesystem

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
