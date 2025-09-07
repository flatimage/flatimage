/**
 * @file controller.hpp
 * @author Ruan Formigoni
 * @brief Manages filesystems used by flatimage
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <memory>
#include <fcntl.h>

#include "../std/filesystem.hpp"
#include "../config.hpp"
#include "filesystem.hpp"
#include "overlayfs.hpp"
#include "unionfs.hpp"
#include "dwarfs.hpp"
#include "ciopfs.hpp"

namespace ns_filesystems::ns_controller
{

extern "C" uint32_t FIM_RESERVED_OFFSET;

class Controller
{
  private:
    fs::path m_path_dir_mount;
    std::vector<fs::path> m_vec_path_dir_mountpoints;
    std::vector<std::unique_ptr<ns_dwarfs::Dwarfs>> m_layers;
    std::vector<std::unique_ptr<ns_filesystem::Filesystem>> m_filesystems;
    std::optional<pid_t> m_opt_pid_janitor;
    [[nodiscard]] uint64_t mount_dwarfs(fs::path const& path_dir_mount, fs::path const& path_file_binary, uint64_t offset);
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
    [[nodiscard]] Expected<void> spawn_janitor();

  public:
    Controller(ns_config::FlatimageConfig const& config);
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
inline Controller::Controller(ns_config::FlatimageConfig const& config)
  : m_path_dir_mount(config.path_dir_mount)
  , m_vec_path_dir_mountpoints()
  , m_layers()
  , m_filesystems()
  , m_opt_pid_janitor(std::nullopt)
{
  // Mount compressed layers
  [[maybe_unused]] uint64_t index_fs = mount_dwarfs(config.path_dir_mount_layers, config.path_file_binary, FIM_RESERVED_OFFSET + FIM_RESERVED_SIZE);
  // Push config files to upper directories if they do not exist in it
  ns_config::push_config_files(config.path_dir_mount_layers, config.path_dir_upper_overlayfs);
  // Use unionfs-fuse
  if ( config.overlay_type == ns_config::OverlayType::FUSE_UNIONFS )
  {
    mount_unionfs(ns_config::get_mounted_layers(config.path_dir_mount_layers)
      , config.path_dir_upper_overlayfs
      , config.path_dir_mount_overlayfs
    );
  }
  // Use fuse-overlayfs
  else if ( config.overlay_type == ns_config::OverlayType::FUSE_OVERLAYFS )
  {
    mount_overlayfs(ns_config::get_mounted_layers(config.path_dir_mount_layers)
      , config.path_dir_upper_overlayfs
      , config.path_dir_mount_overlayfs
      , config.path_dir_work_overlayfs
    );
  }
  // Put casefold over overlayfs or unionfs
  if (config.is_casefold)
  {
    if(config.overlay_type == ns_config::OverlayType::BWRAP)
    {
      ns_log::warn()("casefold cannot be used with bwrap overlays");
    }
    else
    {
      mount_ciopfs(config.path_dir_mount_overlayfs, config.path_dir_mount_ciopfs);
      ns_log::debug()("casefold is enabled");
    }
  }
  else
  {
    ns_log::debug()("casefold is disabled");
  }
  // Spawn janitor, make it permissive since flatimage works without it
  elog_expected("Could not spawn janitor: {}", spawn_janitor());
}

/**
 * @brief Destroy the Controller:: Controller object and
 * stops the janitor if it is running.
 */
inline Controller::~Controller()
{
  if ( m_opt_pid_janitor and m_opt_pid_janitor.value() > 0)
  {
    // Stop janitor loop & wait for cleanup
    kill(m_opt_pid_janitor.value(), SIGTERM);
    // Wait for janitor to finish execution
    int status;
    waitpid(m_opt_pid_janitor.value(), &status, 0);
    dreturn_if(not WIFEXITED(status), "Janitor exited abnormally");
    int code = WEXITSTATUS(status);
    dreturn_if(code != 0, "Janitor exited with code '{}'"_fmt(code));
  } // if
  else
  {
    ns_log::error()("Janitor is not running");
  } // else
}

/**
 * @brief Spawns the janitor.
 * In case the parent process fails to clean the mountpoints, this child does it
 */
[[nodiscard]] inline Expected<void> Controller::spawn_janitor()
{
  // Find janitor binary
  fs::path path_file_janitor = fs::path{Expect(ns_env::get_expected("FIM_DIR_APP_BIN"))} / "fim_janitor";

  // Fork and execve into the janitor process
  pid_t pid_parent = getpid();
  m_opt_pid_janitor = fork();
  qreturn_if(m_opt_pid_janitor < 0, std::unexpected("Failed to fork janitor"));

  // Is parent
  qreturn_if(m_opt_pid_janitor > 0, std::unexpected("Spawned janitor with PID '{}'"_fmt(*m_opt_pid_janitor)));

  // Redirect stdout/stderr to a log file
  fs::path path_stdout = std::string{Expect(ns_env::get_expected("FIM_DIR_MOUNT"))} + ".janitor.stdout.log";
  fs::path path_stderr = std::string{Expect(ns_env::get_expected("FIM_DIR_MOUNT"))} + ".janitor.stderr.log";
  int fd_stdout = open(path_stdout.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  int fd_stderr = open(path_stderr.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  e_exitif(fd_stdout < 0, "Failed to open stdout janitor file", 1);
  e_exitif(fd_stderr < 0, "Failed to open stderr janitor file", 1);
  e_exitif(dup2(fd_stdout, STDOUT_FILENO) < 0, "Failed to dup2 fd_stdout", 1);
  e_exitif(dup2(fd_stderr, STDERR_FILENO) < 0, "Failed to dup2 fd_sterr", 1);
  close(fd_stdout);
  close(fd_stderr);
  close(STDIN_FILENO);

  // Keep parent pid in a variable
  ns_env::set("PID_PARENT", pid_parent, ns_env::Replace::Y);

  // Create args to janitor
  std::vector<std::string> vec_argv_custom;
  vec_argv_custom.push_back(path_file_janitor);
  std::copy(m_vec_path_dir_mountpoints.rbegin(), m_vec_path_dir_mountpoints.rend(), std::back_inserter(vec_argv_custom));

  auto argv_custom = std::make_unique<const char*[]>(vec_argv_custom.size() + 1);
  argv_custom[vec_argv_custom.size()] = nullptr;
  std::ranges::transform(vec_argv_custom, argv_custom.get(), [](auto&& e) { return e.c_str(); });

  // Execve to janitor
  execve(path_file_janitor.c_str(), const_cast<char**>(argv_custom.get()), environ);

  // Exit process in case of an error
  _exit(1);
}

/**
 * @brief Mounts a dwarfs filesystem
 * 
 * @param path_dir_mount Path to the directory to mount the filesystem
 * @param path_file_binary Path to the file which contains the dwarfs filesystem
 * @param offset Offset in the 'path_file_binary' in which the filesystem starts
 * @return uint64_t The index of the last mounted dwarfs filesystem + 1
 */
inline uint64_t Controller::mount_dwarfs(fs::path const& path_dir_mount, fs::path const& path_file_binary, uint64_t offset)
{
  // Open the main binary
  std::ifstream file_binary(path_file_binary, std::ios::binary);

  // Filesystem index
  uint64_t index_fs{};

  auto f_mount = [this](fs::path const& _path_file_binary, fs::path const& _path_dir_mount, uint64_t _index_fs, uint64_t _offset, uint64_t _size_fs)
  {
    std::error_code ec;
    ns_log::debug()("Offset to filesystem is '{}'", _offset);
    // Create mountpoint
    fs::path path_dir_mount_index = _path_dir_mount / std::to_string(_index_fs);
    fs::create_directories(path_dir_mount_index, ec);
    // Configure and mount the filesystem
    // Keep the object alive in the filesystems vector
    this->m_layers.emplace_back(std::make_unique<ns_dwarfs::Dwarfs>(getpid(), path_dir_mount_index, _path_file_binary, _offset, _size_fs));
    // Include current mountpoint in the mountpoints vector
    m_vec_path_dir_mountpoints.push_back(path_dir_mount_index);
  };

  // Advance offset
  file_binary.seekg(offset);

  // Mount filesystem concatenated in the image itself
  while (true)
  {
    // Read filesystem size
    int64_t size_fs;
    dbreak_if(not file_binary.read(reinterpret_cast<char*>(&size_fs), sizeof(size_fs)), "Stopped reading at index {}"_fmt(index_fs));
    ns_log::debug()("Filesystem size is '{}'", size_fs);
    // Skip size bytes
    offset += 8;
    // Check if filesystem is of type 'DWARFS'
    ebreak_if(not ns_dwarfs::is_dwarfs(path_file_binary, offset), "Invalid dwarfs filesystem appended on the image");
    // Mount filesystem
    f_mount(path_file_binary, path_dir_mount, index_fs, offset, size_fs);
    // Go to next filesystem if exists
    index_fs += 1;
    offset += size_fs;
    file_binary.seekg(offset);
  } // while
  file_binary.close();

  // Get layers from layer directories
  std::vector<fs::path> vec_path_file_layer = ns_env::get_expected("FIM_DIRS_LAYER")
    // Expand variable, allow expansion to fail to be non-fatal
    .transform([](auto&& e){ return ns_env::expand(e).value_or(std::string{e}); })
    // Split directories by the char ':'
    .transform([](auto&& e){ return ns_vector::from_string(e, ':'); })
    // Get all files from each directory into a single vector
    .transform([](auto&& e)
    {
      return e
        // Each directory expands to a file list
        | std::views::transform([](auto&& f){ return ns_filesystem::ns_path::list_files(f); })
        // Filter and transform into a vector of vectors
        | std::views::filter([](auto&& f){ return f.has_value(); })
        | std::views::transform([](auto&& f){ return f.value(); })
        // Joins into a single vector
        | std::views::join
        // Collect
        | std::ranges::to<std::vector<fs::path>>();
    }).value_or(std::vector<fs::path>{});
  // Get layers from file paths
  ns_vector::append_range(vec_path_file_layer, ns_env::get_expected("FIM_FILES_LAYER")
    // Expand variable, allow expansion to fail to be non-fatal
    .transform([](auto&& e){ return ns_env::expand(e).value_or(std::string{e}); })
    // Split files by the char ':'
    .transform([](auto&& e){ return ns_vector::from_string(e, ':') | std::ranges::to<std::vector<fs::path>>(); })
    .value_or(std::vector<fs::path>{})
  );
  // Mount external filesystems
  for (fs::path const& path_file_layer : vec_path_file_layer)
  {
    // Check if filesystem is of type 'DWARFS'
    econtinue_if(not ns_dwarfs::is_dwarfs(path_file_layer, 0), "Invalid dwarfs filesystem appended on the image");
    // Mount file as a filesystem
    f_mount(path_file_layer, path_dir_mount, index_fs, 0, fs::file_size(path_file_layer));
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
  m_filesystems.emplace_back(std::make_unique<ns_ciopfs::Ciopfs>(getpid(), path_dir_lower, path_dir_upper));
  m_vec_path_dir_mountpoints.push_back(path_dir_upper);
}

} // namespace ns_filesystems::ns_controller

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
