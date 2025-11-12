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

#include "bwrap/bwrap.hpp"
#include "filesystems/controller.hpp"
#include "db/portal/daemon.hpp"
#include "db/portal/dispatcher.hpp"
#include "lib/env.hpp"
#include "db/env.hpp"
#include "lib/log.hpp"
#include "macro.hpp"
#include "std/expected.hpp"
#include "std/enum.hpp"
#include "reserved/casefold.hpp"
#include "reserved/notify.hpp"
#include "reserved/overlay.hpp"
#include "std/filesystem.hpp"

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

/**
 * @brief Get the mounted layers object
 *
 * @param path_dir_layers Path to the layer directory
 * @return std::vector<fs::path> The list of layer directory paths
 */
[[nodiscard]] inline std::vector<fs::path> ns_filesystems::ns_controller::get_mounted_layers(fs::path const& path_dir_layers)
{
  std::vector<fs::path> vec_path_dir_layer = fs::directory_iterator(path_dir_layers)
    | std::views::filter([](auto&& e){ return fs::is_directory(e.path()); })
    | std::views::transform([](auto&& e){ return e.path(); })
    | std::ranges::to<std::vector<fs::path>>();
  std::ranges::sort(vec_path_dir_layer);
  return vec_path_dir_layer;
}



inline ::ns_filesystems::ns_controller::Logs::Logs(fs::path const& path_dir_log)
  : path_file_dwarfs(path_dir_log / "dwarfs.log")
  , path_file_ciopfs(path_dir_log / "ciopfs.log")
  , path_file_overlayfs(path_dir_log / "overlayfs.log")
  , path_file_unionfs(path_dir_log / "unionfs.log")
  , path_file_janitor(path_dir_log / "janitor.log")
{
  fs::create_directories(path_dir_log);
};

inline ::ns_filesystems::ns_controller::Config::Config(bool is_casefold
  , ns_reserved::ns_overlay::OverlayType const& type
  , fs::path const& path_dir_mount
  , fs::path const& path_dir_config
  , fs::path const& path_bin_janitor
  , fs::path const& path_file_binary
)
  : is_casefold(is_casefold)
  , overlay_type(type)
  , path_dir_mount(path_dir_mount / "fuse")
  , path_dir_work(path_dir_config / "work" / std::to_string(getpid()))
  , path_dir_upper(path_dir_config / "data")
  , path_dir_layers_mount(path_dir_mount / "layers")
  , path_dir_ciopfs_mount(path_dir_config / "casefold")
  , path_bin_janitor(path_bin_janitor)
  , path_file_binary(path_file_binary)
{
  fs::create_directories(path_dir_work);
  fs::create_directories(path_dir_upper);
  fs::create_directories(path_dir_mount);
  fs::create_directories(path_dir_layers_mount);
  fs::create_directories(path_dir_ciopfs_mount);
  fs::create_directories(path_dir_ciopfs_mount);
};


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
 * @return Value<fs::path> The path to the passwd file
 */
[[nodiscard]] inline Value<User> write_passwd(fs::path const& path_file_passwd
  , fs::path const& path_file_bash
  , fs::path const& path_file_bashrc
  , Id const& id
  , std::unordered_map<std::string,std::string> const& variables)
{
  // Get user info
  struct passwd *pw = getpwuid(getuid());
  return_if(not pw, Error("E::Failed to get current user info"));
  // Open passwd file
  std::ofstream file_passwd{path_file_passwd};
  return_if(not file_passwd.is_open(), Error("E::Failed to open passwd file at {}", path_file_passwd));
  // Define passwd entries
  User user
  {
    .name = (id.uid == 0)? "root"
      : variables.contains("USER")? variables.at("USER")
      : ns_env::get_expected("USER").value_or(pw->pw_name),
    .path_dir_home = (id.uid == 0)? "/root"
      : variables.contains("HOME")? variables.at("HOME")
      : ns_env::get_expected("HOME").value_or(pw->pw_dir),
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
 * @return Value<fs::path> The path to the bashrc file
 */
[[nodiscard]] inline Value<fs::path> write_bashrc(fs::path const& path_file_bashrc
  , std::unordered_map<std::string,std::string> const& variables)
{
  // Open new bashrc file
  std::ofstream file_bashrc{path_file_bashrc};
  return_if(not file_bashrc.is_open(), Error("E::Failed to open bashrc file at {}", path_file_bashrc));
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
 * @return Value<Id> The UID and GID pair
 */
[[nodiscard]] inline Value<Id> get_id(bool is_root , std::unordered_map<std::string,std::string> const& hash_env)
{
  // If root mode is requested, return 0:0
  if (is_root)
  {
    return Id{.uid = 0, .gid = 0};
  }
  // Get user info for defaults
  struct passwd *pw = getpwuid(getuid());
  return_if(not pw, Error("E::Failed to get current user info"));
  return Id
  {
    .uid = static_cast<mode_t>(Catch(std::stoul(Catch(hash_env.at("UID")).or_default())).value_or(pw->pw_uid)),
    .gid = static_cast<mode_t>(Catch(std::stoul(Catch(hash_env.at("GID")).or_default())).value_or(pw->pw_gid)),
  };
}

} // namespace


struct Logs
{
  private:
    fs::path const path_dir_log;

  public:
  ns_bwrap::Logs const bwrap;
  ns_db::ns_portal::ns_daemon::ns_log::Logs const daemon_host;
  ns_db::ns_portal::ns_daemon::ns_log::Logs const daemon_guest;
  ns_db::ns_portal::ns_dispatcher::Logs const dispatcher;
  ns_filesystems::ns_controller::Logs const filesystems;
  fs::path const path_file_janitor;
  fs::path const path_file_boot;

  Logs() = delete;
  Logs(fs::path const& path_dir_log)
    : path_dir_log(path_dir_log)
    , bwrap(path_dir_log / "bwrap")
    , daemon_host(path_dir_log / "daemon" / "host")
    , daemon_guest(path_dir_log / "daemon" / "guest")
    , dispatcher(path_dir_log / "dispatcher")
    , filesystems(path_dir_log / "fuse")
    , path_file_janitor(path_dir_log / "janitor.log")
    , path_file_boot(path_dir_log / "boot.log")
  {
    fs::create_directories(path_dir_log);
  }
};

struct FlatimageConfig
{
  // Distribution name
  Distribution const distribution;
  // Pid of the current instance
  pid_t const pid;
  // Feature flags
  bool is_root{};
  bool is_debug{};
  bool is_casefold{};
  bool is_notify{};
  // Logging
  Logs logs;
  // Configurations
  ns_filesystems::ns_controller::Config filesystems;
  // Daemon data and logs
  ns_db::ns_portal::ns_daemon::Daemon const daemon_host;
  ns_db::ns_portal::ns_daemon::Daemon const daemon_guest;
  // Type of overlay filesystem (bwrap,overlayfs,unionfs)
  ns_reserved::ns_overlay::OverlayType overlay_type;
  // Structural directories
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
  // Binary files
  fs::path const path_bin_portal_daemon;
  fs::path const path_bin_portal_dispatcher;
  //Compression level
  uint32_t const layer_compression_level;
  // PATH environment variable
  std::string const env_path;

  Value<User> configure_user()
  {
    auto hash_env = ns_db::ns_env::map(Pop(ns_db::ns_env::get(path_file_binary)));
    Id id = Pop(get_id(is_root, hash_env));
    fs::path path_file_bashrc = Pop(write_bashrc(path_dir_instance / "bashrc", hash_env));
    return Pop(write_passwd(path_dir_instance / "passwd", path_file_bash, path_file_bashrc, id, hash_env));
  }
};

/**
 * @brief Factory method that makes a FlatImage configuration object
 *
 * @return Value<FlatimageConfig> A FlatImage configuration object or the respective error
 */
[[nodiscard]] inline Value<std::shared_ptr<FlatimageConfig>> config()
{
  // Get the current instance's pid
  pid_t pid = getpid();
  // pid and distribution variables
  ns_env::set("FIM_PID", pid, ns_env::Replace::Y);
  ns_env::set("FIM_DIST", FIM_DIST, ns_env::Replace::Y);
  
  // Standard paths
  fs::path const path_dir_global = Pop(ns_env::get_expected("FIM_DIR_GLOBAL"));
  fs::path const path_file_binary = Pop(ns_env::get_expected("FIM_FILE_BINARY"));
  fs::path const path_dir_binary = path_file_binary.parent_path();
  fs::path const path_dir_app = Pop(ns_env::get_expected("FIM_DIR_APP"));
  fs::path const path_dir_app_bin = Pop(ns_env::get_expected("FIM_DIR_APP_BIN"));
  fs::path const path_dir_app_sbin = Pop(ns_env::get_expected("FIM_DIR_APP_SBIN"));
  fs::path const path_dir_instance = Pop(ns_env::get_expected("FIM_DIR_INSTANCE"));
  fs::path const path_dir_mount = Pop(ns_env::get_expected("FIM_DIR_MOUNT"));
  fs::path const path_file_bash = path_dir_app_bin / "bash";
  fs::path const path_dir_mount_layers = path_dir_mount / "layers";

  // Binary files
  fs::path const path_bin_janitor = path_dir_app_bin / "fim_janitor";
  fs::path const path_bin_portal_dispatcher = path_dir_app_bin / "fim_portal";
  fs::path const path_bin_portal_daemon = path_dir_app_bin / "fim_portal_daemon";

  // Log files
  Logs logs = Try(Logs(path_dir_instance / "logs"));

// inline ::ns_filesystems::ns_controller::Config::Config(bool is_casefold
//   , ns_reserved::ns_overlay::OverlayType const& type
//   , fs::path const& path_dir_mount
//   , fs::path const& path_dir_config
//   , fs::path const& path_bin_janitor
//   , fs::path const& path_file_binary

  // Distribution
  Distribution const distribution = Pop(Distribution::from_string(FIM_DIST));

  // Get built-in environment variables
  auto hash_env = ns_db::ns_env::map(Pop(ns_db::ns_env::get(path_file_binary)));

  // Flags
  bool const is_root = ns_env::exists("FIM_ROOT", "1")
    or (hash_env.contains("UID") and hash_env.at("UID") == "0");

  // Flags for readonly, debug, notify, and casefolding
  bool const is_debug = ns_env::exists("FIM_DEBUG", "1");
  bool const is_casefold = ns_env::exists("FIM_CASEFOLD", "1")
    or Pop(ns_reserved::ns_casefold::read(path_file_binary));
  bool const is_notify = Pop(ns_reserved::ns_notify::read(path_file_binary));

  // Configure overlay type
  using ns_reserved::ns_overlay::OverlayType;
  OverlayType overlay_type = ns_env::exists("FIM_OVERLAY", "unionfs")? ns_reserved::ns_overlay::OverlayType::UNIONFS
    : ns_env::exists("FIM_OVERLAY", "overlayfs")? ns_reserved::ns_overlay::OverlayType::OVERLAYFS
    : ns_env::exists("FIM_OVERLAY", "bwrap")? ns_reserved::ns_overlay::OverlayType::BWRAP
    : ns_reserved::ns_overlay::read(path_file_binary).value_or(OverlayType::BWRAP).get();

  // Check for case folding usage constraints
  if(is_casefold and overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP)
  {
    logger("W::casefold cannot be used with bwrap overlayfs, falling back to unionfs");
    overlay_type = ns_reserved::ns_overlay::OverlayType::UNIONFS;
  }

  // Paths only available inside the container (runtime)
  fs::path const path_dir_runtime = "/tmp/fim/run";
  fs::path const path_dir_runtime_host = path_dir_runtime / "host";
  ns_env::set("FIM_DIR_RUNTIME", path_dir_runtime, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_RUNTIME_HOST", path_dir_runtime_host, ns_env::Replace::Y);

  // Home directory
  fs::path const path_dir_host_home = Pop(ns_env::get_expected<fs::path>("HOME")).relative_path();

  // Create host config directory
  fs::path const path_dir_host_config = path_file_binary.parent_path() / std::format(".{}.config", path_file_binary.filename().string());
  fs::path const path_dir_host_config_tmp = path_dir_host_config / "tmp";
  Try(fs::create_directories(path_dir_host_config_tmp));
  ns_env::set("FIM_DIR_CONFIG", path_dir_host_config, ns_env::Replace::Y);

  // Environment
  std::string env_path = path_dir_app_bin.string() + ":" + ns_env::get_expected("PATH").value_or("");
  env_path += ":/sbin:/usr/sbin:/usr/local/sbin:/bin:/usr/bin:/usr/local/bin";
  env_path += std::format(":{}", path_dir_app_sbin.string());
  ns_env::set("PATH", env_path, ns_env::Replace::Y);

  // Compression level configuration (goes from 0 to 10, default is 7)
  uint32_t const layer_compression_level = ({
    std::string str_compression_level = ns_env::get_expected("FIM_COMPRESSION_LEVEL").value_or("7");
    Catch(std::stoi(str_compression_level)).value_or(7);
  });

  // Initialize daemon configuration
  fs::path const path_dir_fifo = Pop(ns_fs::create_directories(path_dir_instance / "portal" / "fifo"));
  using DaemonMode = ns_db::ns_portal::ns_daemon::Mode;
  ns_db::ns_portal::ns_daemon::Daemon const daemon_host(DaemonMode::HOST, path_bin_portal_daemon, path_dir_fifo);
  ns_db::ns_portal::ns_daemon::Daemon const daemon_guest(DaemonMode::GUEST, path_bin_portal_daemon, path_dir_fifo);

  // LD_LIBRARY_PATH
  if ( auto ret = ns_env::get_expected("LD_LIBRARY_PATH") )
  {
    ns_env::set("LD_LIBRARY_PATH", std::format("/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:{}", ret.value()), ns_env::Replace::Y);
  }
  else
  {
    ns_env::set("LD_LIBRARY_PATH", "/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu", ns_env::Replace::Y);
  }

  // Filesystem configuration
  auto const filesystems = ns_filesystems::ns_controller::Config(is_casefold
    , overlay_type
    , path_dir_mount
    , path_dir_host_config
    , path_bin_janitor
    , path_file_binary
  );

  // // Custom deleter for cleanup logic
  // auto deleter = [](FlatimageConfig* cfg)
  // {
  //   if (not cfg) { return; }
  //   fs::path path_dir_work_bwrap = cfg->path_dir_work_overlayfs / "work";
  //   std::error_code ec;
  //   // Clean up bwrap work directory
  //   if (fs::exists(path_dir_work_bwrap, ec) && !ec)
  //   {
  //     log_if(::chmod(path_dir_work_bwrap.c_str(), 0755) < 0
  //       , "E::Error to modify permissions '{}': '{}'", path_dir_work_bwrap.string(), strerror(errno)
  //     );
  //   }
  //   // Clean up work directory
  //   ec.clear();
  //   fs::remove_all(cfg->path_dir_work_overlayfs, ec);
  //   log_if(ec, "E::Error to erase '{}': '{}'", cfg->path_dir_work_overlayfs.string(), ec.message());
  //   delete cfg;
  // };

  return std::shared_ptr<FlatimageConfig>(new FlatimageConfig{
    .distribution = distribution,
    .pid = pid,
    .is_root = is_root,
    .is_debug = is_debug,
    .is_casefold = is_casefold,
    .is_notify = is_notify,
    .logs = logs,
    .filesystems = filesystems,
    .daemon_host = daemon_host,
    .daemon_guest = daemon_guest,
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
    .path_bin_portal_daemon = path_bin_portal_daemon,
    .path_bin_portal_dispatcher = path_bin_portal_dispatcher,
    .layer_compression_level = layer_compression_level,
    .env_path = env_path,
  });
}

} // namespace ns_config


/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
