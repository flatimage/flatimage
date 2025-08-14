/**
 * @file dwarfs.hpp
 * @author Ruan Formigoni
 * @brief Manage dwarfs filesystems
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>

#include "../lib/subprocess.hpp"
#include "../macro.hpp"
#include "filesystem.hpp"

namespace ns_filesystems::ns_dwarfs
{

namespace
{

namespace fs = std::filesystem;

};

class Dwarfs final : public ns_filesystem::Filesystem
{
  private:
    fs::path m_path_file_image;
    uint64_t m_offset;
    uint64_t m_size_image;
  public:
    Dwarfs(pid_t pid_to_die_for
      , fs::path const& path_dir_mount
      , fs::path const& path_file_image
      , uint64_t offset
      , uint64_t size_image);
    Expected<void> mount() override;
};

/**
 * @brief Construct a new Dwarfs:: Dwarfs object
 * 
 * @param pid_to_die_for Pid the mount process should die with
 * @param path_dir_mount Path to the mount directory
 * @param path_file_image Path to the flatimage file
 * @param offset Offset to the filesystem start
 * @param size_image Image length
 */
inline Dwarfs::Dwarfs(pid_t pid_to_die_for, fs::path const& path_dir_mount, fs::path const& path_file_image, uint64_t offset, uint64_t size_image)
  : ns_filesystem::Filesystem(pid_to_die_for, path_dir_mount)
  , m_path_file_image(path_file_image)
  , m_offset(offset)
  , m_size_image(size_image)
{
  if(auto ret = this->mount(); not ret)
  {
    ns_log::error()("Could not mount filesystem '{}' to '{}': {}", path_file_image, path_dir_mount, ret.error());
  }
}

/**
 * @brief Mounts the filesystem
 * 
 * @return Expected<void> Nothing on success or the respective error
 */
inline Expected<void> Dwarfs::mount()
{
  std::error_code ec;
  // Check if image exists and is a regular file
  qreturn_if(not fs::is_regular_file(m_path_file_image, ec)
    , std::unexpected("'{}' does not exist or is not a regular file"_fmt(m_path_file_image))
  );
  // Check if mountpoint exists and is directory
  qreturn_if(not fs::is_directory(m_path_dir_mount, ec)
    , std::unexpected("'{}' does not exist or is not a directory"_fmt(m_path_dir_mount))
  );
  // Find command in PATH
  auto path_file_dwarfs = Expect(ns_subprocess::search_path("dwarfs"));
  // Create command
  m_subprocess = std::make_unique<ns_subprocess::Subprocess>(path_file_dwarfs);
  // Spawn command
  std::ignore = m_subprocess->with_piped_outputs()
    .with_args(m_path_file_image, m_path_dir_mount)
    .with_args("-f", "-o", "auto_unmount,offset={},imagesize={}"_fmt(m_offset, m_size_image))
    .with_die_on_pid(m_pid_to_die_for)
    .spawn();
  // Wait for mount
  ns_fuse::wait_fuse(m_path_dir_mount);
  return {};  
}

/**
 * @brief Checks if the filesystem is a `Dwarfs` filesystem with a given offset
 * 
 * @param path_file_dwarfs Path to the file that contains a dwarfs filesystem
 * @param offset Offset in the file at which the dwarfs filesystem starts
 * @return The boolean result
 */
inline bool is_dwarfs(fs::path const& path_file_dwarfs, uint64_t offset = 0)
{
  // Open file
  std::ifstream file_dwarfs(path_file_dwarfs, std::ios::binary | std::ios::in);
  ereturn_if(not file_dwarfs.is_open(), "Could not open file '{}'"_fmt(path_file_dwarfs), false);
  // Adjust offset
  file_dwarfs.seekg(offset);
  ereturn_if(not file_dwarfs, "Failed to seek offset '{}' in file '{}'"_fmt(offset, path_file_dwarfs), false);
  // Read initial 'DWARFS' identifier
  std::array<char,6> header;
  ereturn_if(not file_dwarfs.read(header.data(), header.size()), "Could not read bytes from file '{}'"_fmt(path_file_dwarfs), false);
  // Check for a successful read
  ereturn_if(file_dwarfs.gcount() != header.size(), "Short read for file '{}'"_fmt(path_file_dwarfs), false);
  // Check for match
  return std::ranges::equal(header, std::string_view("DWARFS"));
}

} // namespace ns_filesystems::ns_dwarfs

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
