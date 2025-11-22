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
#include "../lib/env.hpp"
#include "../macro.hpp"
#include "filesystem.hpp"

/**
 * @namespace ns_filesystems::ns_dwarfs
 * @brief DwarFS compressed read-only filesystem wrapper
 *
 * Provides high-compression read-only filesystem support using DwarFS with on-the-fly
 * decompression. Manages mounting compressed base layers from specific byte offsets within
 * the FlatImage binary, enabling efficient storage and fast random access to the embedded
 * Linux distribution filesystem.
 */
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
      , fs::path const& path_file_log
      , uint64_t offset
      , uint64_t size_image);
    Value<void> mount() override;
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
inline Dwarfs::Dwarfs(pid_t pid_to_die_for
  , fs::path const& path_dir_mount
  , fs::path const& path_file_image
  , fs::path const& path_file_log
  , uint64_t offset
  , uint64_t size_image
)
  : ns_filesystem::Filesystem(pid_to_die_for, path_dir_mount, path_file_log)
  , m_path_file_image(path_file_image)
  , m_offset(offset)
  , m_size_image(size_image)
{
  this->mount().discard("E::Could not mount dwarfs filesystem '{}' to '{}'", path_file_image, path_dir_mount);
}

/**
 * @brief Mounts the filesystem
 *
 * @return Value<void> Nothing on success or the respective error
 */
inline Value<void> Dwarfs::mount()
{
  // Check if image exists and is a regular file
  return_if(not Try(fs::is_regular_file(m_path_file_image))
    , Error("E::'{}' does not exist or is not a regular file", m_path_file_image.string())
  );
  // Check if mountpoint exists and is directory
  return_if(not Try(fs::is_directory(m_path_dir_mount))
    , Error("E::'{}' does not exist or is not a directory", m_path_dir_mount.string())
  );
  // Find command in PATH
  auto path_file_dwarfs = Pop(ns_env::search_path("dwarfs"), "E::Could not find dwarfs in PATH");
  // Spawn command
  m_child = ns_subprocess::Subprocess(path_file_dwarfs)
    .with_args(m_path_file_image, m_path_dir_mount)
    .with_args("-f", "-o", std::format("auto_unmount,offset={},imagesize={}", m_offset, m_size_image))
    .with_die_on_pid(m_pid_to_die_for)
    .with_stdio(ns_subprocess::Stream::Pipe)
    .with_log_file(m_path_file_log)
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
  return_if(not file_dwarfs.is_open(), false, "E::Could not open file '{}'", path_file_dwarfs.string());
  // Adjust offset
  file_dwarfs.seekg(offset);
  return_if(not file_dwarfs, false, "E::Failed to seek offset '{}' in file '{}'", offset, path_file_dwarfs.string());
  // Read initial 'DWARFS' identifier
  std::array<char,6> header;
  return_if(not file_dwarfs.read(header.data(), header.size()), false, "E::Could not read bytes from file '{}'", path_file_dwarfs.string());
  // Check for a successful read
  return_if(file_dwarfs.gcount() != header.size(), false, "E::Short read for file '{}'", path_file_dwarfs.string());
  // Check for match
  return std::ranges::equal(header, std::string_view("DWARFS"));
}

} // namespace ns_filesystems::ns_dwarfs

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
