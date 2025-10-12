/**
 * @file config.hpp
 * @author Ruan Formigoni
 * @brief FlatImage configuration object
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <system_error>
#include <unistd.h>
#include <filesystem>
#include <ranges>

#include "lib/env.hpp"
#include "std/filesystem.hpp"
#include "reserved/casefold.hpp"
#include "reserved/notify.hpp"
#include "reserved/overlay.hpp"
#include "reserved/casefold.hpp"

// Version
#ifndef FIM_VERSION
#error "FIM_VERSION is undefined"
#endif

// Git commit hash
#ifndef FIM_COMMIT
#error "FIM_COMMIT is undefined"
#endif

// Distribution
#ifndef FIM_DIST
#error "FIM_DIST is undefined"
#endif

// Compilation timestamp
#ifndef FIM_TIMESTAMP
#error "FIM_TIMESTAMP is undefined"
#endif

// Json file with binary tools
#ifndef FIM_FILE_TOOLS
#error "FIM_FILE_TOOLS is undefined"
#endif

// Dependencies metadata
#ifndef FIM_FILE_META
#error "FIM_FILE_META is undefined"
#endif

// Size for the configuration reserved space in the flatimage
#ifndef FIM_RESERVED_SIZE
  #error "FIM_RESERVED_SIZE is undefined"
#endif

namespace ns_config
{

namespace
{

namespace fs = std::filesystem;

/**
 * @brief Searches a configuration file from the highest directory of overlay filesystems
 * 
 * @param vec_path_dir_layer Path to the layer directory stack
 * @param path_dir_upper Path to the upper directory of the overlay filesystem
 * @param path_file_config Path to the configuration file to query
 */
inline void search_stack(std::vector<fs::path> const& vec_path_dir_layer
  , fs::path const& path_dir_upper
  , fs::path const& path_file_config)
{
  // Check if configuration exists in upperdir
  dreturn_if(fs::exists(path_dir_upper / path_file_config), "Configuration file '{}' exists"_fmt(path_file_config));
  // Try to find configuration file in layer stack with descending order
  auto it = std::ranges::find_if(vec_path_dir_layer, [&](auto&& e){ return fs::exists(e / path_file_config); });
  dreturn_if(it == std::ranges::end(vec_path_dir_layer),  "Could not find '{}' in layer stack"_fmt(path_file_config));
  // Copy to upperdir
  fs::copy_file(*it / path_file_config, path_dir_upper / path_file_config, fs::copy_options::skip_existing);
}

} // namespace

struct FlatimageConfig
{
  // Distribution name
  std::string str_dist;
  // Feature flags
  bool is_root;
  bool is_readonly;
  bool is_debug;
  bool is_casefold;
  bool is_notify;
  // Type of overlay filesystem (bwrap,overlayfs,unionfs)  
  ns_reserved::ns_overlay::OverlayType overlay_type;
  // Offset 
  uint64_t offset_reserved;
  // Useful directories
  fs::path path_dir_global;
  fs::path path_dir_mount;
  fs::path path_dir_app;
  fs::path path_dir_app_bin;
  fs::path path_dir_app_sbin;
  fs::path path_dir_instance;
  fs::path path_file_binary;
  fs::path path_dir_binary;
  fs::path path_file_bashrc;
  fs::path path_file_bash;
  fs::path path_dir_mount_layers;
  fs::path path_dir_runtime;
  fs::path path_dir_runtime_host;
  fs::path path_dir_host_home;
  fs::path path_dir_host_config;
  fs::path path_dir_host_config_tmp;
  fs::path path_dir_mount_ciopfs;
  fs::path path_dir_data_overlayfs;
  fs::path path_dir_upper_overlayfs;
  fs::path path_dir_work_overlayfs;
  fs::path path_dir_mount_overlayfs;
  //Compression level
  uint32_t layer_compression_level;
  // PATH environment variable
  std::string env_path;

  FlatimageConfig() = default;
  FlatimageConfig(FlatimageConfig const&) = delete;
  FlatimageConfig& operator=(FlatimageConfig const&) = delete;
  ~FlatimageConfig()
  {
    fs::path path_dir_work_bwrap = this->path_dir_work_overlayfs / "work";
    // Clean up bwrap work directory
    if ( fs::exists(path_dir_work_bwrap))
    {
      std::error_code ec;
      fs::permissions(path_dir_work_bwrap
        ,   fs::perms::owner_read  | fs::perms::owner_write | fs::perms::owner_exec
          | fs::perms::group_read  | fs::perms::group_exec
          | fs::perms::others_read | fs::perms::others_exec
        , ec
      );
      elog_if(ec, "Error to modify permissions '{}': '{}'"_fmt(path_dir_work_bwrap, ec.message()));
    }
    // Clean up work directory
    std::error_code ec;
    fs::remove_all(this->path_dir_work_overlayfs, ec);
    elog_if(ec, "Error to erase '{}': '{}'"_fmt(this->path_dir_work_overlayfs, ec.message()));
  }
};

/**
 * @brief Creates a FlatImage configuration object
 * 
 * @return Expected<FlatimageConfig> A FlatImage configuration object or the respective error
 */
inline Expected<std::shared_ptr<FlatimageConfig>> config()
{
  std::error_code ec;
  auto config = std::make_shared<FlatimageConfig>();

  ns_env::set("FIM_PID", getpid(), ns_env::Replace::Y);
  ns_env::set("FIM_DIST", FIM_DIST, ns_env::Replace::Y);

  // Distribution
  config->str_dist = FIM_DIST;

  // Paths in /tmp
  config->path_dir_global          = Expect(ns_env::get_expected("FIM_DIR_GLOBAL"));
  config->path_file_binary         = Expect(ns_env::get_expected("FIM_FILE_BINARY"));
  config->path_dir_binary          = config->path_file_binary.parent_path();
  config->path_dir_app             = Expect(ns_env::get_expected("FIM_DIR_APP"));
  config->path_dir_app_bin         = Expect(ns_env::get_expected("FIM_DIR_APP_BIN"));
  config->path_dir_app_sbin        = Expect(ns_env::get_expected("FIM_DIR_APP_SBIN"));
  config->path_dir_instance        = Expect(ns_env::get_expected("FIM_DIR_INSTANCE"));
  config->path_dir_mount           = Expect(ns_env::get_expected("FIM_DIR_MOUNT"));
  config->path_file_bashrc         = config->path_dir_app / ".bashrc";
  config->path_file_bash           = config->path_dir_app_bin / "bash";
  config->path_dir_mount_layers    = config->path_dir_mount / "layers";
  config->path_dir_mount_overlayfs = config->path_dir_mount / "overlayfs";
  // Flags
  config->is_root = ns_env::exists("FIM_ROOT", "1");
  config->is_readonly = ns_env::exists("FIM_RO", "1");
  config->is_debug = ns_env::exists("FIM_DEBUG", "1");
  config->is_casefold = ns_env::exists("FIM_CASEFOLD", "1")
    or Expect(ns_reserved::ns_casefold::read(config->path_file_binary));
  config->is_notify = Expect(ns_reserved::ns_notify::read(config->path_file_binary));
  config->overlay_type = ns_reserved::ns_overlay::read(config->path_file_binary)
    .value_or(ns_reserved::ns_overlay::OverlayType::BWRAP);
  config->overlay_type = ns_env::exists("FIM_OVERLAY", "unionfs")? ns_reserved::ns_overlay::OverlayType::UNIONFS
    : ns_env::exists("FIM_OVERLAY", "overlayfs")? ns_reserved::ns_overlay::OverlayType::OVERLAYFS
    : ns_env::exists("FIM_OVERLAY", "bwrap")? ns_reserved::ns_overlay::OverlayType::BWRAP
    : config->overlay_type.get();
  if(config->is_casefold and config->overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP)
  {
    ns_log::warn()("casefold cannot be used with bwrap overlayfs, falling back to unionfs");
    config->overlay_type = ns_reserved::ns_overlay::OverlayType::UNIONFS;
  }
  // Paths only available inside the container (runtime)
  config->path_dir_runtime = "/tmp/fim/run";
  config->path_dir_runtime_host = config->path_dir_runtime / "host";
  ns_env::set("FIM_DIR_RUNTIME", config->path_dir_runtime, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_RUNTIME_HOST", config->path_dir_runtime_host, ns_env::Replace::Y);
  // Home directory
  config->path_dir_host_home = Expect(ns_env::get_expected<fs::path>("HOME")).relative_path();
  // Create host config directory
  config->path_dir_host_config = config->path_file_binary.parent_path() / ".{}.config"_fmt(config->path_file_binary.filename());
  config->path_dir_host_config_tmp = config->path_dir_host_config / "tmp";
  Expect(ns_filesystem::ns_path::create_if_not_exists(config->path_dir_host_config_tmp));
  ns_env::set("FIM_DIR_CONFIG", config->path_dir_host_config, ns_env::Replace::Y);
  // Overlayfs write data to remain on the host
  config->path_dir_mount_ciopfs = config->path_dir_host_config / "casefold";
  config->path_dir_data_overlayfs = config->path_dir_host_config / "overlays";
  config->path_dir_upper_overlayfs = config->path_dir_data_overlayfs / "upperdir";
  config->path_dir_work_overlayfs = config->path_dir_data_overlayfs / "workdir" / std::to_string(getpid());
  Expect(ns_filesystem::ns_path::create_if_not_exists(config->path_dir_upper_overlayfs));
  Expect(ns_filesystem::ns_path::create_if_not_exists(config->path_dir_work_overlayfs));
  // Bwrap
  ns_env::set("BWRAP_LOG", config->path_dir_mount.string() + ".bwrap.log", ns_env::Replace::Y);
  // Environment
  config->env_path = config->path_dir_app_bin.string() + ":" + ns_env::get_expected("PATH").value_or("");
  config->env_path += ":/sbin:/usr/sbin:/usr/local/sbin:/bin:/usr/bin:/usr/local/bin";
  config->env_path += ":{}"_fmt(config->path_dir_app_sbin.string());
  ns_env::set("PATH", config->env_path, ns_env::Replace::Y);
  // Compression level configuration (goes from 0 to 10, default is 7)
  config->layer_compression_level  = ({
   std::string str_compression_level = ns_env::get_expected("FIM_COMPRESSION_LEVEL").value_or("7");
   uint32_t compression_level = std::ranges::all_of(str_compression_level, ::isdigit) ? std::stoi(str_compression_level) : 7;
   std::clamp(compression_level, uint32_t{0}, uint32_t{10});
  });
  // LD_LIBRARY_PATH
  if ( auto ret = ns_env::get_expected("LD_LIBRARY_PATH") )
  {
    ns_env::set("LD_LIBRARY_PATH", "/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:{}"_fmt(ret.value()), ns_env::Replace::Y);
  }
  else
  {
    ns_env::set("LD_LIBRARY_PATH", "/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu", ns_env::Replace::Y);
  } // else

  return config;
}

/**
 * @brief Get the mounted layers object
 * 
 * @param path_dir_layers Path to the layer directory
 * @return std::vector<fs::path> The list of layer directory paths
 */
inline std::vector<fs::path> get_mounted_layers(fs::path const& path_dir_layers)
{
  std::vector<fs::path> vec_path_dir_layer = fs::directory_iterator(path_dir_layers)
    | std::views::filter([](auto&& e){ return fs::is_directory(e.path()); })
    | std::views::transform([](auto&& e){ return e.path(); })
    | std::ranges::to<std::vector<fs::path>>();
  std::ranges::sort(vec_path_dir_layer);
  return vec_path_dir_layer;
}

} // namespace ns_config


/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
