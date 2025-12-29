/**
 * @file controller.hpp
 * @author Ruan Formigoni
 * @brief Manages filesystems used by flatimage
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <fcntl.h>

#include "../std/expected.hpp"
#include "../reserved/overlay.hpp"
#include "filesystem.hpp"
#include "overlayfs.hpp"
#include "unionfs.hpp"
#include "dwarfs.hpp"
#include "ciopfs.hpp"
#include "utils.hpp"
#include "layers.hpp"

/**
 * @namespace ns_filesystems
 * @brief FlatImage filesystem layer implementations
 *
 * Contains all filesystem-related components including the orchestration controller, specific
 * filesystem implementations (DwarFS, UnionFS, OverlayFS, CIOPFS), base filesystem interface,
 * and utility functions.
 */

/**
 * @namespace ns_filesystems::ns_controller
 * @brief Filesystem stack orchestration and management
 *
 * Orchestrates the entire FlatImage filesystem layer, managing DwarFS base layers, overlay
 * filesystems (UnionFS/OverlayFS/BWRAP), optional CIOPFS case-folding layer, and janitor
 * cleanup processes. Coordinates mounting, layering, and unmounting of all filesystem
 * components to create the final merged root directory.
 */
namespace ns_filesystems::ns_controller
{

struct Logs
{
  fs::path const path_file_dwarfs;
  fs::path const path_file_ciopfs;
  fs::path const path_file_overlayfs;
  fs::path const path_file_unionfs;
  fs::path const path_file_janitor;
};

struct Config
{
  // Filesystem configuration
  bool const is_casefold;
  uint32_t const compression_level;
  ns_reserved::ns_overlay::OverlayType overlay_type;
  // Main mount point
  fs::path const path_dir_mount;
  // Overlayfs / Unionfs
  fs::path const path_dir_work;
  fs::path const path_dir_upper;
  fs::path const path_dir_layers;
  // Ciopfs
  fs::path const path_dir_ciopfs;
  fs::path const path_bin_janitor;
  fs::path const path_bin_self;
  // Extra layers to mount
  ns_layers::Layers const layers;
};

class Controller
{
  private:
    Logs const m_logs;
    fs::path m_path_dir_mount;
    fs::path m_path_dir_work;
    std::vector<fs::path> m_vec_path_dir_mountpoints;
    std::vector<std::unique_ptr<ns_dwarfs::Dwarfs>> m_dwarfs;
    std::vector<std::unique_ptr<ns_filesystem::Filesystem>> m_filesystems;
    std::unique_ptr<ns_subprocess::Child> m_child_janitor;
    ns_layers::Layers const m_layers;

    [[nodiscard]] uint64_t mount_dwarfs(fs::path const& path_dir_mount);
    void mount_ciopfs(fs::path const& path_dir_lower, fs::path const& path_dir_upper);
    void mount_unionfs(std::vector<fs::path> const& vec_path_dir_layer
      , fs::path const& path_dir_data
      , fs::path const& path_dir_mount
    );
    void mount_overlayfs(std::vector<fs::path> const& vec_path_dir_layer
      , fs::path const& path_dir_data
      , fs::path const& path_dir_mount
      , fs::path const& path_dir_workdir
    );
    // In case the parent process fails to clean the mountpoints, this child does it
    [[nodiscard]] Value<void> spawn_janitor(fs::path const& path_bin_janitor, fs::path const& path_file_log);

  public:
    Controller(Logs const& logs, Config const& config);
    ~Controller();
    Controller(Controller const&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(Controller const&) = delete;
    Controller& operator=(Controller&&) = delete;
};

/**
 * @brief Construct a new Controller:: Controller object
 *
 * @param config FlatImage configuration object
 */
inline Controller::Controller(Logs const& logs , Config const& config)
  : m_logs(logs)
  , m_path_dir_mount(config.path_dir_mount)
  , m_path_dir_work(config.path_dir_work)
  , m_vec_path_dir_mountpoints()
  , m_dwarfs()
  , m_filesystems()
  , m_child_janitor(nullptr)
  , m_layers(config.layers)
{
  // Mount compressed layers
  [[maybe_unused]] uint64_t index_fs = mount_dwarfs(config.path_dir_layers);
  // Use unionfs-fuse
  if ( config.overlay_type == ns_reserved::ns_overlay::OverlayType::UNIONFS )
  {
    logger("D::Overlay type: UNIONFS_FUSE");
    mount_unionfs(::ns_filesystems::ns_utils::get_mounted_layers(config.path_dir_layers)
      , config.path_dir_upper
      , config.path_dir_mount
    );
  }
  // Use fuse-overlayfs
  else if ( config.overlay_type == ns_reserved::ns_overlay::OverlayType::OVERLAYFS )
  {
    logger("D::Overlay type: FUSE_OVERLAYFS");
    mount_overlayfs(::ns_filesystems::ns_utils::get_mounted_layers(config.path_dir_layers)
      , config.path_dir_upper
      , config.path_dir_mount
      , config.path_dir_work
    );
  }
  else
  {
    logger("D::Overlay type: BWRAP");
  }
  // Put casefold over overlayfs or unionfs
  if (config.is_casefold)
  {
    if(config.overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP)
    {
      logger("W::casefold cannot be used with bwrap overlays");
    }
    else
    {
      mount_ciopfs(config.path_dir_mount, config.path_dir_ciopfs);
      logger("D::casefold is enabled");
    }
  }
  else
  {
    logger("D::casefold is disabled");
  }
  // Spawn janitor, make it permissive since flatimage works without it
  spawn_janitor(config.path_bin_janitor, logs.path_file_janitor).discard("E::Could not spawn janitor");
}

/**
 * @brief Destroy the Controller:: Controller object and
 * stops the janitor if it is running.
 */
inline Controller::~Controller()
{
  // Check if janitor is running
  return_if(not m_child_janitor,,"E::Janitor is not running");
  // Stop janitor loop
  m_child_janitor->kill(SIGTERM);
}

/**
 * @brief Spawns the janitor.
 * In case the parent process fails to clean the mountpoints, this child does it
 *
 * @param path_bin_janitor Path to the janitor executable binary
 * @param path_file_log Path to the log file for janitor output
 * @return Value<void> Nothing on success, or the respective error
 */
[[nodiscard]] inline Value<void> Controller::spawn_janitor(fs::path const& path_bin_janitor
  , fs::path const& path_file_log)
{
  // Spawn
  m_child_janitor = ns_subprocess::Subprocess(path_bin_janitor)
    .with_args(getpid(), path_file_log, this->m_vec_path_dir_mountpoints)
    .with_log_file(path_file_log)
    .spawn();
  // Check if janitor is running
  return_if(not m_child_janitor, Error("E::Failed to start janitor"));
  // Check if child is running
  if(auto pid = m_child_janitor->get_pid().value_or(-1); pid > 0)
  {
    logger("D::Spawned janitor with PID '{}'", pid);
  }
  else
  {
    return Error("E::Failed to fork janitor");
  }
  return {};
}

/**
 * @brief Mounts all dwarfs filesystems from the layers collection
 *
 * Iterates through the pre-populated layers (from m_layers) and mounts each one.
 * Layers should be populated before Controller construction using Layers::push_binary()
 * for embedded filesystems and Layers::push() for external layer files.
 *
 * @param path_dir_mount Path to the directory to mount the filesystems
 * @return uint64_t The index of the last mounted dwarfs filesystem + 1
 */
inline uint64_t Controller::mount_dwarfs(fs::path const& path_dir_mount)
{
  // Filesystem index
  uint64_t index_fs{};

  auto f_mount = [this](fs::path const& _path_file_binary
    , fs::path const& _path_dir_mount
    , uint64_t _index_fs
    , uint64_t _offset
    , uint64_t _size_fs) -> Value<void>
  {
    // Create mountpoint
    fs::path path_dir_mount_index = _path_dir_mount / std::to_string(_index_fs);
    Try(fs::create_directories(path_dir_mount_index));
    // Configure and mount the filesystem
    // Keep the object alive in the filesystems vector
    this->m_dwarfs.emplace_back(
      std::make_unique<ns_dwarfs::Dwarfs>(getpid()
        , path_dir_mount_index
        , _path_file_binary
        , m_logs.path_file_dwarfs
        , _offset
        , _size_fs
      )
    );
    // Include current mountpoint in the mountpoints vector
    m_vec_path_dir_mountpoints.push_back(path_dir_mount_index);
    return {};
  };

  // Mount all filesystems (both embedded and external)
  for (auto const& [path_file_layer, offset, size] : m_layers.get_layers())
  {
    logger("D::Mounting layer from '{}' with offset '{}'", path_file_layer.filename(), offset);
    // Check if filesystem is of type 'DWARFS'
    continue_if(not ns_dwarfs::is_dwarfs(path_file_layer, offset)
      , "E::Invalid dwarfs filesystem appended on the image"
    );
    // Mount file as a filesystem
    if (not f_mount(path_file_layer, path_dir_mount, index_fs, offset, size))
    {
      logger("E::Failed to mount filesystem at index {}", index_fs);
      continue;
    }
    // Go to next filesystem if exists
    index_fs += 1;
  } // for

  return index_fs;
}

/**
 * @brief Mounts an unionfs filesystem
 *
 * @param vec_path_dir_layer Vector with the directories to be overlayed (bottom-up)
 * @param path_dir_data Directory to store file changes
 * @param path_dir_mount Path to the mountpoint of the unionfs filesystem
 */
inline void Controller::mount_unionfs(std::vector<fs::path> const& vec_path_dir_layer
  , fs::path const& path_dir_data
  , fs::path const& path_dir_mount)
{
  m_filesystems.emplace_back(std::make_unique<ns_unionfs::UnionFs>(getpid()
    , path_dir_mount
    , path_dir_data
    , m_logs.path_file_unionfs
    , vec_path_dir_layer
  ));
  m_vec_path_dir_mountpoints.push_back(path_dir_mount);
}

/**
 * @brief Mounts a fuse-overlayfs filesystem
 *
 * @param vec_path_dir_layer Vector with the directories to be overlayed (bottom-up)
 * @param path_dir_data Directory to store file changes
 * @param path_dir_mount Path to the mountpoint of the unionfs filesystem
 * @param path_dir_workdir Work directory of fuse-overlayfs
 */
inline void Controller::mount_overlayfs(std::vector<fs::path> const& vec_path_dir_layer
  , fs::path const& path_dir_data
  , fs::path const& path_dir_mount
  , fs::path const& path_dir_workdir)
{
  m_filesystems.emplace_back(std::make_unique<ns_overlayfs::Overlayfs>(getpid()
    , path_dir_mount
    , path_dir_data
    , path_dir_workdir
    , m_logs.path_file_overlayfs
    , vec_path_dir_layer
  ));
  m_vec_path_dir_mountpoints.push_back(path_dir_mount);
}

/**
 * @brief Mounts a fuse-ciopfs filesystem, which is used for case-insensitivity in file paths
 *
 * @param path_dir_lower Directory where the files are stored in
 * @param path_dir_upper Mount point of the fuse-ciopfs filesystem
 */
inline void Controller::mount_ciopfs(fs::path const& path_dir_lower, fs::path const& path_dir_upper)
{
  m_filesystems.emplace_back(std::make_unique<ns_ciopfs::Ciopfs>(getpid()
    , path_dir_lower
    , path_dir_upper
    , m_logs.path_file_ciopfs
  ));
  m_vec_path_dir_mountpoints.push_back(path_dir_upper);
}

} // namespace ns_filesystems::ns_controller

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
