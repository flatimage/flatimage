/**
 * @file config.hpp
 * @author Ruan Formigoni
 * @brief FlatImage configuration object
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <filesystem>
#include <format>
#include <unordered_map>

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

using DaemonMode = ns_db::ns_portal::ns_daemon::Mode;

/**
 * @namespace ns_config
 * @brief Central FlatImage configuration system
 *
 * Serves as the single source of truth for all FlatImage configuration, paths, and runtime flags.
 * Aggregates directory structures, binary locations, log paths, module configurations (filesystem,
 * daemon), and feature flags into one immutable configuration object. Factory method config()
 * initializes all settings from environment variables, reserved space, and compile-time constants.
 */
namespace ns_config
{

// Distribution enum
ENUM(Distribution, ARCH, ALPINE, BLUEPRINT)

namespace
{

namespace fs = std::filesystem;

} // namespace

/**
 * @brief Defines all fundamental FlatImage paths
 *
 * Organizes paths into three categories: directories, files, and binaries.
 * All paths are computed from the binary location and process ID.
 *
 * FlatImage uses the following directory structure
 *
 * @code
 * /tmp/fim/                                    (global)
 * ├── app/
 * │   └── {COMMIT}_{TIMESTAMP}/               (app)
 * │       ├── bin/                            (app_bin)
 * │       │   ├── bash
 * │       │   ├── fim_janitor
 * │       │   ├── fim_portal
 * │       │   └── fim_portal_daemon
 * │       ├── sbin/                           (app_sbin)
 * │       └── instance/
 * │           └── <PID>/                      (instance)
 * │               ├── bashrc
 * │               ├── passwd
 * │               ├── portal/                 (portal)
 * │               │   ├── daemon/
 * │               │   └── dispatcher/
 * │               ├── logs/
 * │               ├── mount/                  (merged root)
 * │               └── layers/                 (layer mounts)
 * │                   ├── 0/
 * │                   └── N/
 * └── run/                                    (runtime)
 *     └── host/                               (runtime_host)
 *
 * {BINARY_DIR}/                               (self)
 * └── .{BINARY_NAME}.data/                    (host_data)
 *     ├── tmp/                                (host_data_tmp)
 *     ├── work/<PID>/                         (fuse work)
 *     ├── data/
 *     ├── casefold/
 *     └── recipes/
 * @endcode
 */
class Path
{
  /**
   * @brief Directory paths used throughout FlatImage
   */
  public:
  struct Dir
  {
    fs::path const self;            ///< Parent directory of the FlatImage binary
    fs::path const global;          ///< Global temporary directory (/tmp/fim)
    fs::path const app;             ///< Application directory (versioned by commit/timestamp)
    fs::path const app_bin;         ///< Application binaries directory
    fs::path const app_sbin;        ///< Application system binaries directory
    fs::path const instance;        ///< Instance-specific directory (per-PID)
    fs::path const portal;          ///< Portal directory for IPC
    fs::path const runtime;         ///< Runtime directory (/tmp/fim/run)
    fs::path const runtime_host;    ///< Host-side runtime directory
    fs::path const host_home;       ///< Relative path to user's home directory
    fs::path const host_data;       ///< Data directory next to binary
    fs::path const host_data_tmp;   ///< Temporary files in data directory
    fs::path const host_data_layers; ///< Layers directory in data directory

    static Value<Dir> create()
    {
      // Get path to self
      fs::path path_bin_self = fs::path{Pop(ns_env::get_expected("FIM_BIN_SELF"))};
      // Compute all paths first
      fs::path self = path_bin_self.parent_path();
      fs::path global = fs::path("/tmp/fim");
      fs::path app = global / "app" / std::format("{}_{}", FIM_COMMIT, FIM_TIMESTAMP);
      fs::path app_bin = app / "bin";
      fs::path app_sbin = app / "sbin";
      fs::path instance = std::format("{}/{}/{}", app.string(), "instance", std::to_string(getpid()));
      fs::path portal = instance / "portal";
      fs::path runtime = fs::path("/tmp/fim/run");
      fs::path runtime_host = runtime / "host";
      fs::path host_home = fs::path{ns_env::get_expected("HOME").value()}.relative_path();
      fs::path host_data = getenv("FIM_DIR_DATA")?:
        self / std::format(".{}.data", path_bin_self.filename().string());
      fs::path host_data_tmp = host_data / "tmp";
      fs::path host_data_layers = host_data / "layers";
      // Side effect: create directories
      Pop(ns_fs::create_directories(host_data_tmp));
      Pop(ns_fs::create_directories(host_data_layers));
      // Aggregate initialization with const members
      return Dir
      {
        .self = std::move(self),
        .global = std::move(global),
        .app = std::move(app),
        .app_bin = std::move(app_bin),
        .app_sbin = std::move(app_sbin),
        .instance = std::move(instance),
        .portal = std::move(portal),
        .runtime = std::move(runtime),
        .runtime_host = std::move(runtime_host),
        .host_home = std::move(host_home),
        .host_data = std::move(host_data),
        .host_data_tmp = std::move(host_data_tmp),
        .host_data_layers = std::move(host_data_layers)
      };
    }
  } dir;

  /**
   * @brief Instance-specific configuration files
   */
  struct File
  {
    fs::path const bashrc;  ///< Path to instance-specific bashrc file
    fs::path const passwd;  ///< Path to instance-specific passwd file

    static File create(fs::path const& path_dir_instance)
    {
      return File{
        .bashrc = path_dir_instance / "bashrc",
        .passwd = path_dir_instance / "passwd"
      };
    }
  } file;

  /**
   * @brief Paths to embedded and extracted binaries
   */
  struct Bin
  {
    fs::path const self;               ///< Path to the FlatImage binary itself
    fs::path const bash;               ///< Embedded bash shell
    fs::path const janitor;            ///< Janitor cleanup process
    fs::path const portal_daemon;      ///< Portal daemon for host-container IPC
    fs::path const portal_dispatcher;  ///< Portal dispatcher for command routing

    static Value<Bin> create(fs::path const& path_dir_app_bin)
    {
      return Bin
      {
        .self = Pop(ns_env::get_expected("FIM_BIN_SELF")),
        .bash = path_dir_app_bin / "bash",
        .janitor = path_dir_app_bin / "fim_janitor",
        .portal_daemon = path_dir_app_bin / "fim_portal_daemon",
        .portal_dispatcher = path_dir_app_bin / "fim_portal"
      };
    }
  } bin;

  /**
   * @brief Factory method to create Path configuration
   *
   * Orchestrates initialization in the correct dependency order:
   * 1. Dir::create() - Computes and creates directory structure
   * 2. File::create() - Creates file paths based on instance directory
   * 3. Bin::create() - Creates binary paths based on app_bin directory
   *
   * @return Value<Path> Initialized path configuration or error
   */
  static Value<Path> create()
  {
    auto dir = Pop(Dir::create());
    auto file = File::create(dir.instance);
    auto bin = Pop(Bin::create(dir.app_bin));
    return Path(dir, file, bin);
  }

  private:
  Path(Dir const& dir, File const& file, Bin const& bin)
    : dir(dir)
    , file(file)
    , bin(bin)
    {
    }
};

/**
 * @brief Log file paths for all FlatImage subsystems
 *
 * Creates the following log directory structure:
 * @code
 * {instance}/logs/
 * ├── bwrap/                        - Bubblewrap sandbox logs
 * ├── daemon/
 * │   ├── host/                     - Portal daemon (host-side) logs
 * │   │   ├── <PID>                -
 * │   │   │   ├── child.log        -
 * │   │   │   └── grand.log        -
 * │   │   └── daemon.log           - Daemon process logging
 * │   ├── guest/                   - Portal daemon (guest-side) logs
 * │   │   ├── <PID>                -
 * │   │   │   ├── child.log        -
 * │   │   │   └── grand.log        -
 * │   └── └── daemon.log                - Portal daemon (host-side) logs
 * ├── dispatcher/                   - Portal dispatcher logs
 * │   ├── host/                     - Portal daemon (host-side) logs
 * │   │   └── <PID>.log             - Portal daemon (host-side) logs
 * │   ├── guest/                    - Portal daemon (guest-side) logs
 * │   └── └── <PID>.log             - Portal daemon (host-side) logs
 * ├── fuse/
 * │   ├── dwarfs.log                - DwarFS filesystem logs
 * │   ├── ciopfs.log                - Case-insensitive FS logs
 * │   ├── overlayfs.log             - OverlayFS logs
 * │   ├── unionfs.log               - UnionFS logs
 * │   ├── janitor.log               - Filesystem janitor logs
 * │   └── guest/                    - Guest filesystem logs
 * └── boot.log                      - Boot/initialization logs
 * @endcode
 */
struct Logs
{
  ns_bwrap::ns_proxy::Logs const bwrap;                           ///< Bubblewrap sandbox log paths
  ns_db::ns_portal::ns_daemon::ns_log::Logs const daemon_host;    ///< Host portal daemon logs
  ns_db::ns_portal::ns_daemon::ns_log::Logs const daemon_guest;   ///< Guest portal daemon logs
  ns_db::ns_portal::ns_dispatcher::Logs const dispatcher;         ///< Portal dispatcher logs
  ns_filesystems::ns_controller::Logs const filesystems;          ///< Filesystem subsystem logs
  fs::path const path_file_boot;                                  ///< Boot initialization log file

  Logs() = delete;
  Logs(fs::path const& path_dir_log)
    : bwrap(path_dir_log / "bwrap")
    , daemon_host(path_dir_log / "daemon" / "host")
    , daemon_guest(path_dir_log / "daemon" / "guest")
    , dispatcher(path_dir_log / "dispatcher")
    , filesystems(ns_filesystems::ns_controller::Logs
      {
        .path_file_dwarfs = path_dir_log / "fuse" / "dwarfs.log",
        .path_file_ciopfs = path_dir_log / "fuse" / "ciopfs.log",
        .path_file_overlayfs = path_dir_log / "fuse" / "overlayfs.log",
        .path_file_unionfs = path_dir_log / "fuse" / "unionfs.log",
        .path_file_janitor = path_dir_log / "fuse" / "janitor.log",
      })
    , path_file_boot(path_dir_log / "boot.log")
  {
    fs::create_directories(path_dir_log);
    fs::create_directories(path_dir_log / "bwrap");
    fs::create_directories(path_dir_log / "daemon" / "host");
    fs::create_directories(path_dir_log / "fuse" / "guest");
    fs::create_directories(path_dir_log / "dispatcher");
  }
};

/**
 * @brief Module configurations linked with FlatImage paths
 *
 * Manages configurations for fuse and daemon subsystems, creating:
 */
struct Config
{
  /**
   * @brief Fuse subsystem configuration
   *
   * Controls the layered filesystem stack with compression and overlay settings.
   */
  ns_filesystems::ns_controller::Config fuse;  ///< Filesystem controller config

  /**
   * @brief Portal daemon configuration
   *
   * Configures IPC daemons for host-container communication.
   */
  struct Daemon
  {
    ns_db::ns_portal::ns_daemon::Daemon const host;   ///< Host-side portal daemon
    ns_db::ns_portal::ns_daemon::Daemon const guest;  ///< Guest-side portal daemon
  } daemon;

  /**
   * @brief Factory method to create Config
   *
   * Creates all necessary directories and initializes module configurations:
   * - Filesystem: mount, work, upper, layers, casefold directories
   * - Daemon: host and guest portal configurations
   *
   * @param is_casefold Enable case-insensitive filesystem layer
   * @param path_dir_instance Instance-specific directory path
   * @param path_dir_host_data Host configuration directory path
   * @param path_bin_janitor Path to janitor cleanup binary
   * @param path_bin_self Path to FlatImage binary
   * @param path_bin_portal_daemon Path to portal daemon binary
   * @param path_dir_portal Portal FIFO directory path
   * @return Value<Config> Initialized configuration or error
   */
  static Value<Config> create(
    bool const is_casefold,
    fs::path const& path_dir_instance,
    fs::path const& path_dir_host_data,
    fs::path const& path_bin_janitor,
    fs::path const& path_bin_self,
    fs::path const& path_bin_portal_daemon,
    fs::path const& path_dir_portal
  )
  {
    // Compute paths first
    auto path_dir_mount = path_dir_instance / "mount";
    auto path_dir_work = path_dir_host_data / "work" / std::to_string(getpid());
    auto path_dir_upper = path_dir_host_data / "data";
    auto path_dir_layers = path_dir_instance / "layers";
    auto path_dir_ciopfs = path_dir_host_data / "casefold";

    // Side effects: create directories
    fs::create_directories(path_dir_mount);
    fs::create_directories(path_dir_work);
    fs::create_directories(path_dir_upper);
    fs::create_directories(path_dir_layers);
    fs::create_directories(path_dir_ciopfs);

    // Configure overlay type
    using ns_reserved::ns_overlay::OverlayType;
    OverlayType overlay_type =
        ns_env::exists("FIM_OVERLAY", "unionfs")? ns_reserved::ns_overlay::OverlayType::UNIONFS
      : ns_env::exists("FIM_OVERLAY", "overlayfs")? ns_reserved::ns_overlay::OverlayType::OVERLAYFS
      : ns_env::exists("FIM_OVERLAY", "bwrap")? ns_reserved::ns_overlay::OverlayType::BWRAP
      : ns_reserved::ns_overlay::read(path_bin_self)
        .value_or(OverlayType::BWRAP).get();
    if(is_casefold and overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP)
    {
      logger("W::casefold cannot be used with bwrap overlayfs, falling back to unionfs");
      overlay_type = ns_reserved::ns_overlay::OverlayType::UNIONFS;
    }

    // Compression level configuration (goes from 0 to 10, default is 7)
    uint32_t const compression_level = ({
      std::string str_compression_level = ns_env::get_expected<"D">("FIM_COMPRESSION_LEVEL").value_or("7");
      Catch(std::stoi(str_compression_level)).value_or(7);
    });

    // Construct configuration objects
    auto fuse = ns_filesystems::ns_controller::Config
    {
      .is_casefold = is_casefold,
      .compression_level = compression_level,
      .overlay_type = overlay_type,
      .path_dir_mount = std::move(path_dir_mount),
      .path_dir_work = std::move(path_dir_work),
      .path_dir_upper = std::move(path_dir_upper),
      .path_dir_layers = std::move(path_dir_layers),
      .path_dir_ciopfs = std::move(path_dir_ciopfs),
      .path_bin_janitor = path_bin_janitor,
      .path_bin_self = path_bin_self,
    };

    auto daemon = Daemon
    {
      .host = ns_db::ns_portal::ns_daemon::Daemon(DaemonMode::HOST
        , path_bin_portal_daemon
        , path_dir_portal
      ),
      .guest = ns_db::ns_portal::ns_daemon::Daemon(DaemonMode::GUEST
        , path_bin_portal_daemon
        , path_dir_portal
      ),
    };

    return Config
    {
      .fuse = std::move(fuse),
      .daemon = std::move(daemon)
    };
  }
};

/**
 * @brief Runtime feature flags
 *
 * Controls FlatImage behavior through environment variables and reserved space:
 *
 * Flag resolution priority (highest to lowest):
 * 1. Environment variables (FIM_ROOT, FIM_DEBUG, FIM_CASEFOLD)
 * 2. Reserved space configuration (casefold, notify)
 * 3. Binary environment database (UID check for root)
 *
 * @note All flags are immutable after creation
 */
struct Flags
{
  private:
  Flags() = default;

  public:
  bool is_root;       ///< Execute as root (UID=0)? Checked via FIM_ROOT env or stored UID
  bool is_debug;      ///< Enable debug logging? Set via FIM_DEBUG=1
  bool is_casefold;   ///< Enable case-insensitive filesystem? Via FIM_CASEFOLD or reserved space
  bool is_notify;     ///< Show desktop notifications? Stored in reserved space

  /**
   * @brief Factory method to create Flags
   *
   * Resolves flags from multiple sources in priority order:
   * - is_root: FIM_ROOT env → binary UID database
   * - is_debug: FIM_DEBUG env
   * - is_casefold: FIM_CASEFOLD env → reserved space
   * - is_notify: reserved space only
   *
   * @param path_bin_self Path to FlatImage binary
   * @return Value<Flags> Initialized flags or error
   */
  static Value<Flags> create(fs::path const& path_bin_self)
  {
    Flags flags;
    auto environment = ns_db::ns_env::map(Pop(ns_db::ns_env::get(path_bin_self)));
    flags.is_root = ns_env::exists("FIM_ROOT", "1")
      or (environment.contains("UID") and environment.at("UID") == "0");
    flags.is_casefold = ns_env::exists("FIM_CASEFOLD", "1")
      or Pop(ns_reserved::ns_casefold::read(path_bin_self));
    flags.is_notify = Pop(ns_reserved::ns_notify::read(path_bin_self));
    flags.is_debug = ns_env::exists("FIM_DEBUG", "1");
    return flags;
  }
};

/**
 * @brief Main FlatImage configuration object
 *
 * Central "single source of truth" for all FlatImage configuration.
 * Aggregates paths, flags, logs, and module configurations into one immutable structure.
 */
struct FlatImage
{
  Distribution const distribution;  ///< Linux distribution (ARCH/ALPINE/BLUEPRINT)
  pid_t const pid;                  ///< Current instance process ID
  Flags flags;                      ///< Runtime feature flags
  Logs logs;                        ///< Log file paths for all subsystems
  Config config;                    ///< Module configurations (filesystem, daemon)
  Path path;                        ///< Directory, file, and binary paths

  [[nodiscard]] Value<ns_bwrap::ns_proxy::User> configure_bwrap() const
  {
    using namespace ns_bwrap::ns_proxy;
    // Get environment variables saved in the binary
    auto variables = ns_db::ns_env::map(Pop(ns_db::ns_env::get(path.bin.self)));
    // Get user info for defaults
    struct passwd *pw = getpwuid(getuid());
    return_if(not pw, Error("E::Failed to get current user info"));
    // Get user id with this priority
    // 1. is_root
    // 1. Binary environment data
    // 2. pw
    Id id = flags.is_root? Id{.uid = 0, .gid = 0} : Id
    {
      .uid = static_cast<mode_t>(Catch(std::stoul(Catch(variables.at("UID")).or_default())).value_or(pw->pw_uid)),
      .gid = static_cast<mode_t>(Catch(std::stoul(Catch(variables.at("GID")).or_default())).value_or(pw->pw_gid)),
    };
    // Create user
    User user(User::UserData
    {
      .id = id,
      .name = (id.uid == 0)? "root"
        : variables.contains("USER")? variables.at("USER")
        : ns_env::get_expected("USER").value_or(pw->pw_name),
      .path_dir_home = (id.uid == 0)? "/root"
        : variables.contains("HOME")? variables.at("HOME")
        : ns_env::get_expected("HOME").value_or(pw->pw_dir),
      .path_file_shell = variables.contains("SHELL")? fs::path{variables.at("SHELL")}
        : path.bin.bash,
      .path_file_bashrc = path.file.bashrc,
      .path_file_passwd = path.file.passwd,
    });
    // Write files
    Pop(user.write_bashrc(path.file.bashrc, variables.contains("PS1")? variables.at("PS1") : ""));
    Pop(user.write_passwd(path.file.passwd));
    return user;
  }

  ns_reserved::ns_overlay::OverlayType overlay_type()
  {
    // Configure overlay type
    using ns_reserved::ns_overlay::OverlayType;
    OverlayType overlay_type = ns_env::exists("FIM_OVERLAY", "unionfs")? ns_reserved::ns_overlay::OverlayType::UNIONFS
      : ns_env::exists("FIM_OVERLAY", "overlayfs")? ns_reserved::ns_overlay::OverlayType::OVERLAYFS
      : ns_env::exists("FIM_OVERLAY", "bwrap")? ns_reserved::ns_overlay::OverlayType::BWRAP
      : ns_reserved::ns_overlay::read(path.bin.self).value_or(OverlayType::BWRAP).get();

    // Check for case folding usage constraints
    if(flags.is_casefold and overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP)
    {
      logger("W::casefold cannot be used with bwrap overlayfs, falling back to unionfs");
      overlay_type = ns_reserved::ns_overlay::OverlayType::UNIONFS;
    }
    return overlay_type;
  }
}; // struct FlatImage

/**
 * @brief Factory method that makes a FlatImage configuration object
 *
 * @return Value<FlatImage> A FlatImage configuration object or the respective error
 */
[[nodiscard]] inline Value<std::shared_ptr<FlatImage>> config()
{
  // Distribution
  Distribution const distribution = Pop(Distribution::from_string(FIM_DIST));

  // Standard paths
  Path path = Pop(Path::create());

  // Flags
  Flags flags = Pop(Flags::create(path.bin.self));

  // Log files
  Logs logs = Try(Logs(path.dir.instance / "logs"));

  // Module configuration
  Config config = Pop(Config::create(
    flags.is_casefold,
    path.dir.instance,
    path.dir.host_data,
    path.bin.janitor,
    path.bin.self,
    path.bin.portal_daemon,
    path.dir.portal
  ));

  // LD_LIBRARY_PATH
  if ( auto ret = ns_env::get_expected<"D">("LD_LIBRARY_PATH") )
  {
    ns_env::set("LD_LIBRARY_PATH"
      , std::format("/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu:{}", ret.value())
      , ns_env::Replace::Y
    );
  }
  else
  {
    ns_env::set("LD_LIBRARY_PATH"
      , "/usr/lib/x86_64-linux-gnu:/usr/lib/i386-linux-gnu"
      , ns_env::Replace::Y
    );
  }

  // PATH
  std::string env_path = path.dir.app_bin.string()
    + ":"
    + ns_env::get_expected("PATH").value_or("")
    + ":"
    + "/sbin:/usr/sbin:/usr/local/sbin:/bin:/usr/bin:/usr/local/bin"
    + ":"
    + path.dir.app_sbin.string();
  ns_env::set("PATH", env_path, ns_env::Replace::Y);

  // Get the current instance's pid
  pid_t pid = getpid();

  // Set environment variables
  Pop(ns_env::get_expected("FIM_BIN_SELF"), "C::Path to self is not defined");
  ns_env::set("FIM_DIR_GLOBAL", path.dir.global, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_APP", path.dir.app, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_APP_BIN", path.dir.app_bin, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_APP_SBIN", path.dir.app_sbin, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_INSTANCE", path.dir.instance, ns_env::Replace::Y);
  ns_env::set("FIM_PID", pid, ns_env::Replace::Y);
  ns_env::set("FIM_DIST", FIM_DIST, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_RUNTIME", path.dir.runtime, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_RUNTIME_HOST", path.dir.runtime_host, ns_env::Replace::Y);
  ns_env::set("FIM_DIR_DATA", path.dir.host_data, ns_env::Replace::Y);

  // Create FlatImage object
  return std::shared_ptr<FlatImage>(new FlatImage{
    .distribution = distribution,
    .pid = pid,
    .flags = flags,
    .logs = logs,
    .config = config,
    .path = path,
  });
}

} // namespace ns_config


/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
