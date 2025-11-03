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
#include <pwd.h>
#include <filesystem>
#include <fstream>
#include <ranges>

#include "lib/env.hpp"
#include "db/env.hpp"
#include "std/filesystem.hpp"
#include "std/enum.hpp"
#include "reserved/casefold.hpp"
#include "reserved/notify.hpp"
#include "reserved/overlay.hpp"

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

// Distribution enum
ENUM(Distribution, ARCH, ALPINE, BLUEPRINT)

// UID/GID pair
struct UidGid
{
  mode_t uid;
  mode_t gid;
};

namespace
{

namespace fs = std::filesystem;

} // namespace

struct FlatimageConfig
{
  // Distribution name
  Distribution distribution;
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
  fs::path path_file_passwd;
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
  Expected<fs::path> write_passwd() const;
  Expected<fs::path> write_bashrc() const;
  Expected<UidGid> get_uid_gid() const;
};

/**
 * @brief Creates a FlatImage configuration object
 * 
 * @return Expected<FlatimageConfig> A FlatImage configuration object or the respective error
 */
[[nodiscard]] inline Expected<std::shared_ptr<FlatimageConfig>> config()
{
  std::error_code ec;
  auto config = std::make_shared<FlatimageConfig>();

  ns_env::set("FIM_PID", getpid(), ns_env::Replace::Y);
  ns_env::set("FIM_DIST", FIM_DIST, ns_env::Replace::Y);

  // Distribution
  config->distribution = Expect(Distribution::from_string(FIM_DIST));

  // Paths in /tmp
  config->path_dir_global          = Expect(ns_env::get_expected("FIM_DIR_GLOBAL"));
  config->path_file_binary         = Expect(ns_env::get_expected("FIM_FILE_BINARY"));
  config->path_dir_binary          = config->path_file_binary.parent_path();
  config->path_dir_app             = Expect(ns_env::get_expected("FIM_DIR_APP"));
  config->path_dir_app_bin         = Expect(ns_env::get_expected("FIM_DIR_APP_BIN"));
  config->path_dir_app_sbin        = Expect(ns_env::get_expected("FIM_DIR_APP_SBIN"));
  config->path_dir_instance        = Expect(ns_env::get_expected("FIM_DIR_INSTANCE"));
  config->path_dir_mount           = Expect(ns_env::get_expected("FIM_DIR_MOUNT"));
  config->path_file_bashrc         = config->path_dir_instance / "bashrc";
  config->path_file_passwd         = config->path_dir_instance / "passwd";
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
  Expect(ns_fs::create_directories(config->path_dir_host_config_tmp));
  ns_env::set("FIM_DIR_CONFIG", config->path_dir_host_config, ns_env::Replace::Y);
  // Overlayfs write data to remain on the host
  config->path_dir_mount_ciopfs = config->path_dir_host_config / "casefold";
  config->path_dir_data_overlayfs = config->path_dir_host_config / "overlays";
  config->path_dir_upper_overlayfs = config->path_dir_data_overlayfs / "upperdir";
  config->path_dir_work_overlayfs = config->path_dir_data_overlayfs / "workdir" / std::to_string(getpid());
  Expect(ns_fs::create_directories(config->path_dir_upper_overlayfs));
  Expect(ns_fs::create_directories(config->path_dir_work_overlayfs));
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
   std::ranges::all_of(str_compression_level, ::isdigit) ? std::stoi(str_compression_level) : 7;
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
[[nodiscard]] inline std::vector<fs::path> get_mounted_layers(fs::path const& path_dir_layers)
{
  std::vector<fs::path> vec_path_dir_layer = fs::directory_iterator(path_dir_layers)
    | std::views::filter([](auto&& e){ return fs::is_directory(e.path()); })
    | std::views::transform([](auto&& e){ return e.path(); })
    | std::ranges::to<std::vector<fs::path>>();
  std::ranges::sort(vec_path_dir_layer);
  return vec_path_dir_layer;
}

/**
 * @brief Writes the passwd file and returns its path
 * 
 * @return Expected<fs::path> The path to the passwd file
 */
[[nodiscard]] inline Expected<fs::path> FlatimageConfig::write_passwd() const
{
  // Get user info
  struct passwd *pw = getpwuid(getuid());
  qreturn_if(not pw, Unexpected("E::Failed to get current user info"));
  // Get UID/GID (respects is_root flag and custom values from environment)
  auto uid_gid = Expect(this->get_uid_gid());
  // Get environment variables
  auto program_env = Expect(ns_db::ns_env::get(this->path_file_binary));
  auto variables = ns_db::ns_env::key_value(program_env);
  // Open passwd file
  std::ofstream file_passwd{this->path_file_passwd};
  qreturn_if(not file_passwd.is_open(), Unexpected("E::Failed to open passwd file at {}", this->path_file_passwd));
  // Write to passwd file using configured UID/GID
  if(variables.contains("USER"))
  {
    std::string user = variables.at("USER");
    file_passwd << user << ":x:" << uid_gid.uid << ":" << uid_gid.gid << ":"
      << user << ":" << std::format("/home/{}", user) << ":" << pw->pw_shell << "\n";
  }
  else
  {
    file_passwd << pw->pw_name << ":x:" << uid_gid.uid << ":" << uid_gid.gid << ":"
                << pw->pw_gecos << ":" << pw->pw_dir << ":" << pw->pw_shell << "\n";
  }
  file_passwd.close();
  return this->path_file_passwd;
}

/**
 * @brief Writes the bashrc file and returns its path
 *
 * @return Expected<fs::path> The path to the bashrc file
 */
[[nodiscard]] inline Expected<fs::path> FlatimageConfig::write_bashrc() const
{
  // Get environment variables
  auto program_env = Expect(ns_db::ns_env::get(this->path_file_binary));
  auto variables = ns_db::ns_env::key_value(program_env);
  // Open new bashrc file
  std::ofstream file_bashrc{this->path_file_bashrc};
  qreturn_if(not file_bashrc.is_open(), Unexpected("E::Failed to open bashrc file at {}", this->path_file_bashrc));
  // Write custom PS1 or default value
  if(variables.contains("PS1"))
  {
    file_bashrc << "export PS1=" << '"' << variables.at("PS1") << '"';
  }
  else
  {
    file_bashrc << R"(export PS1="[flatimage-${FIM_DIST,,}] \W > ")";
  }
  file_bashrc.close();
  return this->path_file_bashrc;
}

/**
 * @brief Gets the UID and GID for the container
 *
 * Checks environment database for custom UID/GID values, falls back to current user's values.
 * If is_root flag is set, returns 0:0.
 *
 * @return Expected<UidGid> The UID and GID pair
 */
[[nodiscard]] inline Expected<UidGid> FlatimageConfig::get_uid_gid() const
{
  // If root mode is requested, return 0:0
  if (this->is_root)
  {
    return UidGid{.uid = 0, .gid = 0};
  }

  // Get user info for defaults
  struct passwd *pw = getpwuid(getuid());
  qreturn_if(not pw, Unexpected("E::Failed to get current user info"));

  // Get environment variables to check for custom UID/GID
  auto program_env = Expect(ns_db::ns_env::get(this->path_file_binary));
  auto variables = ns_db::ns_env::key_value(program_env);

  // Default to actual host user UID/GID
  mode_t uid = pw->pw_uid;
  mode_t gid = pw->pw_gid;

  // Check for custom UID in environment
  if (variables.contains("UID"))
  {
    try
    {
      uid = std::stoul(variables.at("UID"));
      ns_log::debug()("Using custom UID from environment: {}", uid);
    }
    catch (std::exception const& e)
    {
      ns_log::warn()("Invalid UID in environment '{}', using default: {}", variables.at("UID"), pw->pw_uid);
      uid = pw->pw_uid;
    }
  }

  // Check for custom GID in environment
  if (variables.contains("GID"))
  {
    try
    {
      gid = std::stoul(variables.at("GID"));
      ns_log::debug()("Using custom GID from environment: {}", gid);
    }
    catch (std::exception const& e)
    {
      ns_log::warn()("Invalid GID in environment '{}', using default: {}", variables.at("GID"), pw->pw_gid);
      gid = pw->pw_gid;
    }
  }

  return UidGid{.uid = uid, .gid = gid};
}

} // namespace ns_config


/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
