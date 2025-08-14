/**
 * @file overlayfs.hpp
 * @author Ruan Formigoni
 * @brief Manages the fuse-overlayfs filesystem
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <unistd.h>

#include "../lib/subprocess.hpp"
#include "../lib/fuse.hpp"
#include "filesystem.hpp"

namespace
{

namespace fs = std::filesystem;

} // namespace

namespace ns_filesystems::ns_overlayfs
{

class Overlayfs final : public ns_filesystem::Filesystem
{
  private:
    fs::path m_path_dir_upper;
    fs::path m_path_dir_work;
    std::vector<fs::path> m_vec_path_dir_layers;

  public:
    Overlayfs(pid_t pid_to_die_for
        , fs::path const& path_dir_mount
        , fs::path const& path_dir_upperdir
        , fs::path const& path_dir_workdir
        , std::vector<fs::path> const& vec_path_dir_layers
    );
    Expected<void> mount() override;
};


/**
 * @brief Construct a new Overlayfs:: Overlayfs object
 * 
 * @param pid_to_die_for Pid the mount process should die with
 * @param path_dir_mount Path to the mount directory
 * @param path_dir_upper Upper directory where the changes of overlayfs are stored
 * @param path_dir_work Work directory of overlayfs
 * @param vec_path_dir_layers Vector of directories to overlay with overlayfs (bottom-up)
 */
inline Overlayfs::Overlayfs(pid_t pid_to_die_for
    , fs::path const& path_dir_mount
    , fs::path const& path_dir_upper
    , fs::path const& path_dir_work
    , std::vector<fs::path> const& vec_path_dir_layers
    )
  : ns_filesystem::Filesystem(pid_to_die_for, path_dir_mount)
  , m_path_dir_upper(path_dir_upper)
  , m_path_dir_work(path_dir_work)
  , m_vec_path_dir_layers(vec_path_dir_layers)
{
  if(auto ret = this->mount(); not ret)
  {
    ns_log::error()("Could not mount overlayfs filesystem to '{}': {}", path_dir_mount, ret.error());
  }
}

/**
 * @brief Mounts the filesystem
 * 
 * @return Expected<void> Nothing on success or the respective error
 */
inline Expected<void> Overlayfs::mount()
{
  std::error_code ec;
  // Validate directories
  qreturn_if(not fs::exists(m_path_dir_upper, ec) and not fs::create_directories(m_path_dir_upper, ec)
    , std::unexpected("Could not create modifications dir for overlayfs")
  );
  qreturn_if(not fs::exists(m_path_dir_mount, ec) and not fs::create_directories(m_path_dir_mount, ec)
    , std::unexpected("Could not create mountpoint for overlayfs")
  );
  // Find overlayfs
  auto path_file_overlayfs = Expect(ns_subprocess::search_path("overlayfs"));
  // Create subprocess
  m_subprocess = std::make_unique<ns_subprocess::Subprocess>(path_file_overlayfs);
  // Get user and group ids
  uid_t user_id = getuid();
  gid_t group_id = getgid();
  // Create string to represent argument of lowerdirs
  // lowerdir= option is top-down
  std::string arg_lowerdir = m_vec_path_dir_layers
    | std::views::reverse
    | std::views::transform([](auto&& e){ return e.string(); })
    | std::views::join_with(std::string{":"})
    | std::ranges::to<std::string>();
  arg_lowerdir = "lowerdir=" + arg_lowerdir;
  // Include arguments and spawn process
  std::ignore = m_subprocess->
     with_args("-f")
    .with_args("-o", "squash_to_uid={}"_fmt(user_id))
    .with_args("-o", "squash_to_gid={}"_fmt(group_id))
    .with_args("-o", arg_lowerdir)
    .with_args("-o", "upperdir={}"_fmt(m_path_dir_upper))
    .with_args("-o", "workdir={}"_fmt(m_path_dir_work))
    .with_args(m_path_dir_mount)
    .with_die_on_pid(m_pid_to_die_for)
    .spawn();
  // Wait for mount
  ns_fuse::wait_fuse(m_path_dir_mount);
  return {};
} // Overlayfs



} // namespace ns_filesystems::ns_overlayfs

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
