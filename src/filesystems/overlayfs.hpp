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
#include "../lib/env.hpp"
#include "../lib/fuse.hpp"
#include "../std/filesystem.hpp"
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
        , fs::path const& path_dir_upper
        , fs::path const& path_dir_work
        , fs::path const& path_file_log
        , std::vector<fs::path> const& vec_path_dir_layers
    );
    Value<void> mount() override;
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
    , fs::path const& path_file_log
    , std::vector<fs::path> const& vec_path_dir_layers
    )
  : ns_filesystem::Filesystem(pid_to_die_for, path_dir_mount, path_file_log)
  , m_path_dir_upper(path_dir_upper)
  , m_path_dir_work(path_dir_work)
  , m_vec_path_dir_layers(vec_path_dir_layers)
{
  this->mount().discard("E::Could not mount overlayfs filesystem to '{}'", path_dir_mount);
}

/**
 * @brief Mounts the filesystem
 *
 * @return Value<void> Nothing on success or the respective error
 */
inline Value<void> Overlayfs::mount()
{
  // Validate directories
  Pop(ns_fs::create_directories(m_path_dir_upper), "E::Failed to create upper directory");
  Pop(ns_fs::create_directories(m_path_dir_mount), "E::Failed to create mount directory");
  // Find overlayfs
  auto path_file_overlayfs = Pop(ns_env::search_path("overlayfs"), "E::Could not find overlayfs in PATH");
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
  using enum ns_subprocess::Stream;
  m_child = ns_subprocess::Subprocess(path_file_overlayfs)
    .with_args("-f")
    .with_args("-o", std::format("squash_to_uid={}", user_id))
    .with_args("-o", std::format("squash_to_gid={}", group_id))
    .with_args("-o", arg_lowerdir)
    .with_args("-o", std::format("upperdir={}", m_path_dir_upper.string()))
    .with_args("-o", std::format("workdir={}", m_path_dir_work.string()))
    .with_args(m_path_dir_mount)
    .with_die_on_pid(m_pid_to_die_for)
    .with_stdio(ns_subprocess::Stream::Pipe)
    .with_log_file(m_path_file_log)
    .spawn();
  // Wait for mount
  ns_fuse::wait_fuse(m_path_dir_mount);
  return {};
} // Overlayfs



} // namespace ns_filesystems::ns_overlayfs

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
