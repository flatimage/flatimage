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

#include "../lib/subprocess.hpp"
#include "../lib/fuse.hpp"

namespace ns_filesystem
{


class Filesystem
{
  protected:
    pid_t m_pid_to_die_for;
    std::filesystem::path m_path_dir_mount;
    std::unique_ptr<ns_subprocess::Subprocess> m_subprocess;
    Filesystem(pid_t pid_to_die_for, std::filesystem::path const& path_dir_mount);
    
  public:
    virtual ~Filesystem();
    [[nodiscard]] virtual Value<void> mount() = 0;
    Filesystem(Filesystem&&) = default;
    Filesystem(Filesystem const&) = delete;
    Filesystem& operator=(Filesystem&&) = default;
    Filesystem& operator=(Filesystem const&) = delete;
};

/**
 * @brief Construct a new Filesystem:: Filesystem object
 * 
 * @param pid_to_die_for Pid the mount process should die with
 * @param path_dir_mount Path to the mount directory
 */
inline Filesystem::Filesystem(pid_t pid_to_die_for, std::filesystem::path const& path_dir_mount)
  : m_pid_to_die_for(pid_to_die_for)
  , m_path_dir_mount(path_dir_mount)
  , m_subprocess(nullptr)
{
}

/**
 * @brief Destroy the Filesystem:: Filesystem object, it
 * un-mounts the filesystem and sends a termination signal
 */
inline Filesystem::~Filesystem()
{
  if(auto ret = ns_fuse::unmount(m_path_dir_mount); not ret)
  {
    ns_log::error()("Could not un-mount filesystem '{}'", m_path_dir_mount);
  }
  // Check for subprocess
  if(not m_subprocess)
  {
    ns_log::error()("No fuse sub-process for '{}'", m_path_dir_mount);
    return;
  }
  // Tell process to exit with SIGTERM
  if (auto opt_pid = m_subprocess->get_pid())
  {
    kill(*opt_pid, SIGTERM);
  }
  // Wait for process to exit
  auto ret = m_subprocess->wait();
  dreturn_if(not ret, "Mount '{}' exited unexpectedly"_fmt(m_path_dir_mount));
  dreturn_if(ret and *ret != 0, "Mount '{}' exited with non-zero exit code '{}'"_fmt(m_path_dir_mount, *ret));
}

} // namespace ns_filesystem

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
