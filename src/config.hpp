/**
 * @file config.hpp
 * @author Ruan Formigoni
 * @brief FlatImage configuration object
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <pwd.h>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <format>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "lib/env.hpp"
#include "db/env.hpp"
#include "lib/log.hpp"
#include "macro.hpp"
#include "std/expected.hpp"
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
struct Id
{
  mode_t uid;
  mode_t gid;
};

struct User
{
  std::string name;
  std::filesystem::path path_dir_home;
  std::filesystem::path path_file_shell;
  std::filesystem::path path_file_bashrc;
  std::filesystem::path path_file_passwd;
  Id id;
  operator std::string()
  {
    return std::format("{}:x:{}:{}:{}:{}:{}", name, id.uid, id.gid, name, path_dir_home.string(), path_file_shell.string());
  }
};

namespace
{

namespace fs = std::filesystem;

/**
 * @brief Writes the passwd file and returns its path
 * 
 * @return Expected<fs::path> The path to the passwd file
 */
[[nodiscard]] inline Expected<User> write_passwd(fs::path const& path_file_passwd
  , fs::path const& path_file_bash
  , fs::path const& path_file_bashrc
  , Id const& id
  , std::unordered_map<std::string,std::string> const& variables)
{
  // Get user info
  struct passwd *pw = getpwuid(getuid());
  qreturn_if(not pw, Unexpected("E::Failed to get current user info"));
  // Open passwd file
  std::ofstream file_passwd{path_file_passwd};
  qreturn_if(not file_passwd.is_open(), Unexpected("E::Failed to open passwd file at {}", path_file_passwd));
  // Define passwd entries
  User user
  {
    .name = (id.uid == 0)? "root"
      : variables.contains("USER")? variables.at("USER")
      : getenv("USER")?
      : pw->pw_name,
    .path_dir_home = (id.uid == 0)? "/root"
      : variables.contains("HOME")? variables.at("HOME")
      : getenv("HOME")?
      : pw->pw_dir,
    .path_file_shell = variables.contains("SHELL")? fs::path{variables.at("SHELL")}
      : path_file_bash,
    .path_file_bashrc = path_file_bashrc,
    .path_file_passwd = path_file_passwd,
    .id = id,
  };
  // Write to passwd file
  file_passwd << std::string{user} << '\n';
  return user;
}

/**
 * @brief Writes the bashrc file and returns its path
 *
 * @return Expected<fs::path> The path to the bashrc file
 */
[[nodiscard]] inline Expected<fs::path> write_bashrc(fs::path const& path_file_bashrc
  , std::unordered_map<std::string,std::string> const& variables)
{
  // Open new bashrc file
  std::ofstream file_bashrc{path_file_bashrc};
  qreturn_if(not file_bashrc.is_open(), Unexpected("E::Failed to open bashrc file at {}", path_file_bashrc));
  // Write custom PS1 or default value
  if(variables.contains("PS1"))
  {
    file_bashrc << "export PS1=" << '"' << variables.at("PS1") << '"';
  }
  else
  {
    file_bashrc << R"(export PS1="[flatimage-${FIM_DIST,,}] \W > ")";
  }
  return path_file_bashrc;
}

/**
 * @brief Gets the UID and GID for the container
 *
 * Checks environment database for custom UID/GID values, falls back to current user's values.
 * If is_root flag is set, returns 0:0.
 *
 * @return Expected<Id> The UID and GID pair
 */
[[nodiscard]] inline Expected<Id> get_id(bool is_root , std::unordered_map<std::string,std::string> const& hash_env)
{
  // If root mode is requested, return 0:0
  if (is_root)
  {
    return Id{.uid = 0, .gid = 0};
  }
  // Get user info for defaults
  struct passwd *pw = getpwuid(getuid());
  qreturn_if(not pw, Unexpected("E::Failed to get current user info"));
  // Default to actual host user UID/GID
  mode_t uid = pw->pw_uid;
  mode_t gid = pw->pw_gid;
  // Check for custom UID in environment
  if (hash_env.contains("UID"))
  {
    try
    {
      uid = std::stoul(hash_env.at("UID"));
      ns_log::debug()("Using custom UID from environment: {}", uid);
    }
    catch (std::exception const& e)
    {
      ns_log::warn()("Invalid UID in environment '{}', using default: {}", hash_env.at("UID"), pw->pw_uid);
      uid = pw->pw_uid;
    }
  }
  // Check for custom GID in environment
  if (hash_env.contains("GID"))
  {
    try
    {
      gid = std::stoul(hash_env.at("GID"));
      ns_log::debug()("Using custom GID from environment: {}", gid);
    }
    catch (std::exception const& e)
    {
      ns_log::warn()("Invalid GID in environment '{}', using default: {}", hash_env.at("GID"), pw->pw_gid);
      gid = pw->pw_gid;
    }
  }
  return Id{.uid = uid, .gid = gid};
}

} // namespace

struct FlatimageConfig
{
  // Distribution name
  Distribution const distribution;
  // Feature flags
  bool is_root{};
  bool is_debug{};
  bool is_casefold{};
  bool is_notify{};
  // Type of overlay filesystem (bwrap,overlayfs,unionfs)
  ns_reserved::ns_overlay::OverlayType overlay_type;
  // Useful directories
  fs::path const path_dir_global;
  fs::path const path_dir_mount;
  fs::path const path_dir_app;
  fs::path const path_dir_app_bin;
  fs::path const path_dir_app_sbin;
  fs::path const path_dir_instance;
  fs::path const path_file_binary;
  fs::path const path_dir_binary;
  fs::path const path_file_bash;
  fs::path const path_dir_mount_layers;
  fs::path const path_dir_runtime;
  fs::path const path_dir_runtime_host;
  fs::path const path_dir_host_home;
  fs::path const path_dir_host_config;
  fs::path const path_dir_host_config_tmp;
  fs::path const path_dir_mount_ciopfs;
  fs::path const path_dir_data_overlayfs;
  fs::path const path_dir_upper_overlayfs;
  fs::path const path_dir_work_overlayfs;
  fs::path const path_dir_mount_overlayfs;
  //Compression level
  uint32_t layer_compression_level{};
  // PATH environment variable
  std::string env_path;

  Expected<User> configure_user()
  {
    auto hash_env = ns_db::ns_env::key_value(Expect(ns_db::ns_env::get(path_file_binary)));
    Id id = Expect(get_id(is_root, hash_env));
    fs::path path_file_bashrc = Expect(write_bashrc(path_dir_instance / "bashrc", hash_env));
    return Expect(write_passwd(path_dir_instance / "passwd", path_file_bash, path_file_bashrc, id, hash_env));
  }
};

/**
 * @brief Factory method that makes a FlatImage configuration object
 *
 * @return Expected<FlatimageConfig> A FlatImage configuration object or the respective error
 */
[[nodiscard]] inline Expected<std::shared_ptr<FlatimageConfig>> config()
{
  // pid and distribution variables
  ns_env::set("FIM_PID", getpid(), ns_env::Replace::Y);
  ns_env::set("FIM_DIST", FIM_DIST, ns_env::Replace::Y);

  // Standard paths
  fs::path path_dir_global = Expect(ns_env::get_expected("FIM_DIR_GLOBAL"));
  fs::path path_file_binary = Expect(ns_env::get_expected("FIM_FILE_BINARY"));
  fs::path path_dir_binary = path_file_binary.parent_path();
  fs::path path_dir_app = Expect(ns_env::get_expected("FIM_DIR_APP"));
  fs::path path_dir_app_bin = Expect(ns_env::get_expected("FIM_DIR_APP_BIN"));
  fs::path path_dir_app_sbin = Expect(ns_env::get_expected("FIM_DIR_APP_SBIN"));
  fs::path path_dir_instance = Expect(ns_env::get_expected("FIM_DIR_INSTANCE"));
  fs::path path_dir_mount = Expect(ns_env::get_expected("FIM_DIR_MOUNT"));
  fs::path path_file_bash = path_dir_app_bin / "bash";
  fs::path path_dir_mount_layers = path_dir_mount / "layers";
  fs::path path_dir_mount_overlayfs = path_dir_mount / "overlayfs";

  // Distribution
  Distribution distribution = Expect(Distribution::from_string(FIM_DIST));

  // Get built-in environment variables
  auto hash_env = ns_db::ns_env::key_value(Expect(ns_db::ns_env::get(path_file_binary)));

  // Flags
  bool is_root = ns_env::exists("FIM_ROOT", "1")
    or (hash_env.contains("UID") and hash_env.at("UID") == "0");

  // Flags for readonly, debug, notify, and casefolding
  bool is_debug = ns_env::exists("FIM_DEBUG", "1");
  bool is_casefold = ns_env::exists("FIM_CASEFOLD", "1")
    or Expect(ns_reserved::ns_casefold::read(path_file_binary));
  bool is_notify = Expect(ns_reserved::ns_notify::read(path_file_binary));

  // Configure overlay type
  using ns_reserved::ns_overlay::OverlayType;
  OverlayType overlay_type = ns_env::exists("FIM_OVERLAY", "unionfs")? ns_reserved::ns_overlay::OverlayType::UNIONFS
    : ns_env::exists("FIM_OVERLAY", "overlayfs")? ns_reserved::ns_overlay::OverlayType::OVERLAYFS
    : ns_env::exists("FIM_OVERLAY", "bwrap")? ns_reserved::ns_overlay::OverlayType::BWRAP
    : ns_reserved::ns_overlay::read(path_file_binary).value_or(OverlayType::BWRAP).get();

  // Check for case folding usage constraints
  if(is_casefold and overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP)
  {
    ns_log::warn()("casefold cannot be used with bwrap overlayfs, falling back to unionfs");
    overlay_type = ns_reserved::ns_overlay::OverlayType::UNIONFS;
  }

  // Paths only available inside the container (runtime)
  fs::path path_dir_runtime = "/tmp/fim/run";
  fs::path path_dir_runtime_host = path_dir_runtime / "host";
  ns_env::set("FIM_DIR_RUNTIME", path_dir_runtime, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_RUNTIME_HOST", path_dir_runtime_host, ns_env::Replace::Y);

  // Home directory
  fs::path path_dir_host_home = Expect(ns_env::get_expected<fs::path>("HOME")).relative_path();

  // Create host config directory
  fs::path path_dir_host_config = path_file_binary.parent_path() / ".{}.config"_fmt(path_file_binary.filename());
  fs::path path_dir_host_config_tmp = path_dir_host_config / "tmp";
  Expect(ns_fs::create_directories(path_dir_host_config_tmp));
  ns_env::set("FIM_DIR_CONFIG", path_dir_host_config, ns_env::Replace::Y);

  // Overlayfs write data to remain on the host
  fs::path path_dir_mount_ciopfs = path_dir_host_config / "casefold";
  fs::path path_dir_data_overlayfs = path_dir_host_config / "overlays";
  fs::path path_dir_upper_overlayfs = path_dir_data_overlayfs / "upperdir";
  fs::path path_dir_work_overlayfs = path_dir_data_overlayfs / "workdir" / std::to_string(getpid());
  Expect(ns_fs::create_directories(path_dir_upper_overlayfs));
  Expect(ns_fs::create_directories(path_dir_work_overlayfs));

  // Bwrap
  ns_env::set("BWRAP_LOG", path_dir_mount.string() + ".bwrap.log", ns_env::Replace::Y);

  // Environment
  std::string env_path = path_dir_app_bin.string() + ":" + ns_env::get_expected("PATH").value_or("");
  env_path += ":/sbin:/usr/sbin:/usr/local/sbin:/bin:/usr/bin:/usr/local/bin";
  env_path += ":{}"_fmt(path_dir_app_sbin.string());
  ns_env::set("PATH", env_path, ns_env::Replace::Y);

  // Compression level configuration (goes from 0 to 10, default is 7)
  uint32_t layer_compression_level = ({
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
  }

  // Custom deleter for cleanup logic
  auto deleter = [](FlatimageConfig* cfg)
  {
    if (not cfg) { return; }
    fs::path path_dir_work_bwrap = cfg->path_dir_work_overlayfs / "work";
    // Clean up bwrap work directory
    if (ns_fs::exists(path_dir_work_bwrap).value_or(false))
    {
      elog_if(::chmod(path_dir_work_bwrap.c_str(), 0755) < 0
        , "Error to modify permissions '{}': '{}'"_fmt(path_dir_work_bwrap, strerror(errno))
      );
    }
    // Clean up work directory
    std::error_code ec;
    fs::remove_all(cfg->path_dir_work_overlayfs, ec);
    elog_if(ec, "Error to erase '{}': '{}'"_fmt(cfg->path_dir_work_overlayfs, ec.message()));
    delete cfg;
  };

  return std::shared_ptr<FlatimageConfig>(new FlatimageConfig{
    .distribution = distribution,
    .is_root = is_root,
    .is_debug = is_debug,
    .is_casefold = is_casefold,
    .is_notify = is_notify,
    .overlay_type = overlay_type,
    .path_dir_global = path_dir_global,
    .path_dir_mount = path_dir_mount,
    .path_dir_app = path_dir_app,
    .path_dir_app_bin = path_dir_app_bin,
    .path_dir_app_sbin = path_dir_app_sbin,
    .path_dir_instance = path_dir_instance,
    .path_file_binary = path_file_binary,
    .path_dir_binary = path_dir_binary,
    .path_file_bash = path_file_bash,
    .path_dir_mount_layers = path_dir_mount_layers,
    .path_dir_runtime = path_dir_runtime,
    .path_dir_runtime_host = path_dir_runtime_host,
    .path_dir_host_home = path_dir_host_home,
    .path_dir_host_config = path_dir_host_config,
    .path_dir_host_config_tmp = path_dir_host_config_tmp,
    .path_dir_mount_ciopfs = path_dir_mount_ciopfs,
    .path_dir_data_overlayfs = path_dir_data_overlayfs,
    .path_dir_upper_overlayfs = path_dir_upper_overlayfs,
    .path_dir_work_overlayfs = path_dir_work_overlayfs,
    .path_dir_mount_overlayfs = path_dir_mount_overlayfs,
    .layer_compression_level = layer_compression_level,
    .env_path = env_path,
  }, deleter);
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


} // namespace ns_config


/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
