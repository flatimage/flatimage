/**
 * @file ciopfs.hpp
 * @author Ruan Formigoni
 * @brief Manage ciopfs filesystems
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <unistd.h>

#include "../lib/subprocess.hpp"
#include "../lib/env.hpp"
#include "../std/filesystem.hpp"
#include "filesystem.hpp"

namespace ns_filesystems::ns_ciopfs
{


namespace
{

namespace fs = std::filesystem;

} // namespace

class Ciopfs final : public ns_filesystem::Filesystem
{
  private:
    fs::path m_path_dir_upper;
    fs::path m_path_dir_lower;

  public:
    Ciopfs(pid_t pid_to_die_for, fs::path const& path_dir_lower, fs::path const& path_dir_upper);
    Value<void> mount() override;
};

inline Ciopfs::Ciopfs(pid_t pid_to_die_for, fs::path const& path_dir_lower, fs::path const& path_dir_upper)
  : ns_filesystem::Filesystem(pid_to_die_for, path_dir_upper)
  , m_path_dir_upper(path_dir_upper)
  , m_path_dir_lower(path_dir_lower)
{
  if(auto ret = this->mount(); not ret)
  {
    ns_log::error()("Could u mount ciopfs filesystem from '{}' to '{}': {}", path_dir_lower, path_dir_upper, ret.error());
  }
}

/**
 * @brief Mounts the filesystem
 * 
 * @return Value<void> Nothing on success or the respective error
 */
inline Value<void> Ciopfs::mount()
{
  // Validate directories
  Pop(ns_fs::create_directories(m_path_dir_lower), "E::Failed to create lower directory");
  Pop(ns_fs::create_directories(m_path_dir_upper), "E::Failed to create upper directory");
  // Find Ciopfs
  auto path_file_ciopfs = Pop(ns_env::search_path("ciopfs"), "E::Could not find ciopfs in PATH");
  // Create subprocess
  m_subprocess = std::make_unique<ns_subprocess::Subprocess>(path_file_ciopfs);
  // Include arguments and spawn process
  std::ignore = m_subprocess->
    with_args(m_path_dir_lower, m_path_dir_upper)
    .with_die_on_pid(m_pid_to_die_for)
    .spawn();
  // Wait for mount
  ns_fuse::wait_fuse(m_path_dir_upper);
  return {};
}

} // namespace ns_filesystems::ns_ciopfs

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
