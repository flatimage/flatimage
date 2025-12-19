/**
 * @file controller.hpp
 * @author Ruan Formigoni
 * @brief Manages filesystems used by flatimage
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <memory>
#include <fcntl.h>

#include "../std/expected.hpp"
#include "../std/filesystem.hpp"
#include "../reserved/overlay.hpp"
#include "filesystem.hpp"
#include "overlayfs.hpp"
#include "unionfs.hpp"
#include "dwarfs.hpp"
#include "ciopfs.hpp"
#include "utils.hpp"

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

extern "C" uint32_t FIM_RESERVED_OFFSET;

struct Logs
{
  fs::path const path_file_dwarfs;
  fs::path const path_file_ciopfs;
  fs::path const path_file_overlayfs;
  fs::path const path_file_unionfs;
  fs::path const path_file_janitor;
};

/**
 * @class Layers
 * @brief Manages external DwarFS layer files and directories for the filesystem controller
 *
 * Provides a unified interface for collecting layer files from both individual file paths
 * and directories. Supports the FIM_LAYERS environment variable which accepts a colon-separated
 * list of paths that can be files or directories.
 *
 * **Directory Scanning:**
 * - Non-recursive (only scans direct children)
 * - Alphabetical order within each directory
 * - Validates files as DwarFS filesystems
 *
 * **File Processing:**
 * - Direct file paths are validated and added immediately
 * - Invalid files are skipped with a warning
 *
 * @example
 * @code
 * Layers layers;
 * layers.push_from_var("FIM_LAYERS");  // Process FIM_LAYERS env var
 * layers.push("/path/to/layers");       // Add directory or file
 * auto const& paths = layers.get_layers();  // Retrieve all layer paths
 * @endcode
 */
class Layers
{
  private:
    std::vector<fs::path> layers;  ///< Collection of validated layer file paths

    /**
     * @brief Validates and appends a single layer file
     *
     * Checks if the file is a valid DwarFS filesystem by examining magic bytes.
     * Invalid files are skipped with a warning.
     *
     * @param path Path to the layer file to validate and add
     */
    [[nodiscard]] Value<void> append_file(fs::path const& path)
    {
      return_if(not ns_dwarfs::is_dwarfs(path), Error("W::Skipping invalid dwarfs filesystem '{}'", path));
      layers.push_back(path);
      return {};
    }

    /**
     * @brief Scans a directory for layer files and appends them
     *
     * Performs non-recursive directory scanning to collect regular files.
     * Sorts files lexicographically
     * Each file is validated via append_file() before being added.
     *
     * @param path Path to directory to scan
     */
    [[nodiscard]] Value<void> append_directory(fs::path const& path)
    {
      // Get all regular files from the directory
      auto result = Pop(ns_fs::regular_files(path));
      // Sort layers files
      std::ranges::sort(result);
      // Process each file
      for(auto&& file : result)
      {
        append_file(file).discard("W::Failed to append layer from directory");
      }
      return {};
    }

  public:
    /**
     * @brief Adds a layer from a file or directory path
     *
     * Automatically detects whether the path is a file or directory and processes accordingly:
     * - **File:** Validates and adds directly
     * - **Directory:** Scans for layer files and adds them alphabetically
     *
     * @param path Filesystem path (file or directory)
     * @return Value<void> Success or error
     */
    [[nodiscard]] Value<void> push(fs::path const& path)
    {
      if(Try(fs::is_regular_file(path)))
      {
        append_file(path).discard("W::Failed to append layer from regular file");
      }
      else if(Try(fs::is_directory(path)))
      {
        append_directory(path).discard("W::Failed to append layer from directory");
      }
      return {};
    }

    /**
     * @brief Loads layers from a colon-separated environment variable
     *
     * Processes environment variables like FIM_LAYERS which contain colon-separated
     * paths. Each path can be either a file or directory. Performs shell expansion
     * on variable values before processing.
     *
     * **Processing Steps:**
     * 1. Retrieves environment variable value
     * 2. Performs word expansion (variables, subshells)
     * 3. Splits on ':' delimiter
     * 4. Processes each path via push()
     *
     * @param var Name of environment variable to read (e.g., "FIM_LAYERS")
     * @return Value<void> Success or error
     */
    [[nodiscard]] Value<void> push_from_var(std::string_view var)
    {
      for(fs::path const& path : ns_env::get_expected<"Q">(var)
        // Perform word expansions (variables, run subshells...)
        .transform([](auto&& e){ return ns_env::expand(e).value_or(std::string{e}); })
        // Get value or default to empty
        .value_or(std::string{})
        // Split values variable on ':'
        | std::views::split(':')
        // Create a path
        | std::views::transform([](auto&& e){ return fs::path(e.begin(), e.end()); })
      )
      {
        Pop(push(path));
      }
      return {};
    }

    /**
     * @brief Retrieves the collected layer file paths
     *
     * Returns the final list of validated layer files in the order they were added.
     * This is used by the filesystem controller to mount layers sequentially.
     *
     * @return const reference to vector of layer file paths
     */
    std::vector<fs::path> const& get_layers() const
    {
      return layers;
    }
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
  Layers const layers;
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
    Layers const m_layers;

    [[nodiscard]] uint64_t mount_dwarfs(fs::path const& path_dir_mount, fs::path const& path_bin_self, uint64_t offset);
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
  [[maybe_unused]] uint64_t index_fs = mount_dwarfs(config.path_dir_layers
    , config.path_bin_self
    , FIM_RESERVED_OFFSET + FIM_RESERVED_SIZE
  );
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
 * @brief Mounts a dwarfs filesystem
 *
 * @param path_dir_mount Path to the directory to mount the filesystem
 * @param path_bin_self Path to the file which contains the dwarfs filesystem
 * @param offset Offset in the 'path_bin_self' in which the filesystem starts
 * @return uint64_t The index of the last mounted dwarfs filesystem + 1
 */
inline uint64_t Controller::mount_dwarfs(fs::path const& path_dir_mount, fs::path const& path_bin_self, uint64_t offset)
{
  // Open the main binary
  std::ifstream file_binary(path_bin_self, std::ios::binary);

  // Filesystem index
  uint64_t index_fs{};

  auto f_mount = [this](fs::path const& _path_file_binary
    , fs::path const& _path_dir_mount
    , uint64_t _index_fs
    , uint64_t _offset
    , uint64_t _size_fs) -> Value<void>
  {
    logger("D::Offset to filesystem is '{}'", _offset);
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

  // Advance offset
  file_binary.seekg(offset);

  // Mount filesystem concatenated in the image itself
  while (true)
  {
    // Read filesystem size
    int64_t size_fs;
    break_if(not file_binary.read(reinterpret_cast<char*>(&size_fs), sizeof(size_fs))
      , "D::Stopped reading at index {}", index_fs
    );
    logger("D::Filesystem size is '{}'", size_fs);
    // Validate filesystem size
    break_if(size_fs <= 0, "E::Invalid filesystem size '{}' at index {}", size_fs, index_fs);
    // Skip size bytes
    offset += 8;
    // Check if filesystem is of type 'DWARFS'
    break_if(not ns_dwarfs::is_dwarfs(path_bin_self, offset)
      , "E::Invalid dwarfs filesystem appended on the image"
    );
    // Mount filesystem
    break_if(not f_mount(path_bin_self, path_dir_mount, index_fs, offset, size_fs)
      , "E::Failed to mount filesystem at index {}", index_fs
    );
    // Go to next filesystem if exists
    index_fs += 1;
    offset += size_fs;
    file_binary.seekg(offset);
  } // while
  file_binary.close();

  // Mount external filesystems
  for (fs::path const& path_file_layer : m_layers.get_layers())
  {
    // Check if filesystem is of type 'DWARFS'
    continue_if(not ns_dwarfs::is_dwarfs(path_file_layer, 0)
      , "E::Invalid dwarfs filesystem appended on the image"
    );
    // Mount file as a filesystem
    auto size_result = Catch(fs::file_size(path_file_layer));
    if (!size_result)
    {
      logger("E::Failed to get file size for layer: {}", path_file_layer.string());
      continue;
    }
    auto mount_result = f_mount(path_file_layer, path_dir_mount, index_fs, 0, size_result.value());
    if (!mount_result)
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
