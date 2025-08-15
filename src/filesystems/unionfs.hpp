/**
 * @file unionfs.hpp
 * @author Ruan Formigoni
 * @brief Manages unionfs filesystems
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <unistd.h>

#include "../lib/subprocess.hpp"
#include "../lib/env.hpp"
#include "../lib/fuse.hpp"
#include "filesystem.hpp"

namespace
{

namespace fs = std::filesystem;

} // namespace

namespace ns_filesystems::ns_unionfs
{

class UnionFs final : public ns_filesystem::Filesystem
{
  private:
    fs::path m_path_dir_upper;
    std::vector<fs::path> m_vec_path_dir_layer;
  public:
    UnionFs(pid_t pid_to_die_for
      , fs::path const& path_dir_mount
      , fs::path const& path_dir_upper
      , std::vector<fs::path> const& vec_path_dir_layer
    );
    Expected<void> mount() override;
};

/**
 * @brief Construct a new Union Fs:: Union Fs object
 * 
 * @param pid_to_die_for Pid the mount process should die with
 * @param path_dir_mount Path to the mount directory
 * @param path_dir_upper Upper directory where the changes of overlayfs are stored
 * @param vec_path_dir_layers Vector of directories to overlay with overlayfs (bottom-up)
 */
inline UnionFs::UnionFs(pid_t pid_to_die_for
    , fs::path const& path_dir_mount
    , fs::path const& path_dir_upper
    , std::vector<fs::path> const& vec_path_dir_layer
  )
  : ns_filesystem::Filesystem(pid_to_die_for, path_dir_mount)
  , m_path_dir_upper(path_dir_upper)
  , m_vec_path_dir_layer(vec_path_dir_layer)
{
  if(auto ret = this->mount(); not ret)
  {
    ns_log::error()("Could not mount unionfs filesystem to '{}': {}", path_dir_mount, ret.error());
  }
}

/**
 * @brief Mounts the filesystem
 * 
 * @return Expected<void> Nothing on success or the respective error
 */
inline Expected<void> UnionFs::mount()
{
  std::error_code ec;
  // Validate directories
  qreturn_if(not fs::exists(m_path_dir_upper, ec) and not fs::create_directories(m_path_dir_upper, ec)
    , std::unexpected("Could not create modifications dir for unionfs")
  );
  qreturn_if (not fs::exists(m_path_dir_mount, ec) and not fs::create_directories(m_path_dir_mount, ec)
    , std::unexpected("Could not create mountpoint for unionfs")
  );
  // Find unionfs
  auto path_file_unionfs = Expect(ns_env::search_path("unionfs"));
  // Create subprocess
  m_subprocess = std::make_unique<ns_subprocess::Subprocess>(path_file_unionfs);
  // Create string to represent layers argumnet
  // First layer is the writteable one
  // Layers are overlayed as top_branch:lower_branch:...:lowest_branch
  std::string arg_layers="{}=RW"_fmt(m_path_dir_upper);
  for (auto&& path_dir_layer : m_vec_path_dir_layer | std::views::reverse)
  {
    arg_layers += ":{}=RO"_fmt(path_dir_layer);
  } // for
  // Include arguments and spawn process
  std::ignore = m_subprocess->
     with_args("-f")
    .with_args("-o", "cow")
    .with_args(arg_layers)
    .with_args(m_path_dir_mount)
    .with_die_on_pid(m_pid_to_die_for)
    .spawn();
  // Wait for mount
  ns_fuse::wait_fuse(m_path_dir_mount);
  return {};
}


} // namespace ns_filesystems::ns_unionfs

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
