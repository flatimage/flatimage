/**
 * @file bwrap.hpp
 * @author Ruan Formigoni
 * @brief Configures and launches [bubblewrap](https://github.com/containers/bubblewrap)
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <fcntl.h>
#include <filesystem>
#include <ranges>
#include <sys/types.h>
#include <pwd.h>
#include <regex>

#include "../db/bind.hpp"
#include "../db/portal/daemon.hpp"
#include "../db/portal/dispatcher.hpp"
#include "../reserved/permissions.hpp"
#include "../std/expected.hpp"
#include "../std/vector.hpp"
#include "../lib/log.hpp"
#include "../lib/subprocess.hpp"
#include "../lib/env.hpp"
#include "../macro.hpp"

/**
 * @namespace ns_bwrap
 * @brief Bubblewrap sandboxing integration
 *
 * Provides unprivileged containerization via Linux namespaces using bubblewrap.
 * Manages sandbox configuration, permission-based bind mounts, user namespace isolation,
 * overlay filesystem setup, and GPU/device access control. Supports both native overlay
 * and FUSE-based overlay modes.
 */
namespace ns_bwrap
{

namespace
{

namespace fs = std::filesystem;

}

/**
 * @namespace ns_bwrap::ns_proxy
 * @brief Bubblewrap proxy types and user configuration
 *
 * Contains data structures for configuring the bubblewrap sandbox environment,
 * including user identity mapping (UID/GID), overlay filesystem configuration,
 * log file paths, and container user representation with passwd/bashrc generation.
 */
namespace ns_proxy
{

/**
 * @brief A pair of uid and gid mode_t values
 */
struct Id
{
  mode_t uid;
  mode_t gid;
};


/**
 * @brief Bwrap's native overlay related options
 */
struct Overlay
{
  std::vector<fs::path> vec_path_dir_layer;
  fs::path path_dir_upper;
  fs::path path_dir_work;
};

/**
 * @brief Log files used by bwrap
 */
struct Logs
{
  fs::path const path_file_apparmor;
  Logs(fs::path const& path_dir_log) : path_file_apparmor(path_dir_log / "apparmor.log")
  {
    fs::create_directories(path_file_apparmor.parent_path());
  }
};

/**
 * @brief The representation of a user in bubblewrap
 */
struct User
{
  struct UserData
  {
    Id id;
    std::string const name;
    std::filesystem::path const path_dir_home;
    std::filesystem::path const path_file_shell;
    std::filesystem::path const path_file_bashrc;
    std::filesystem::path const path_file_passwd;
    /**
    * @brief Generates the passwd file entry
    *
    * @return std::string The passwd file entry
    */
    inline operator std::string()
    {
      return std::format("{}:x:{}:{}:{}:{}:{}", name, id.uid, id.gid, name, path_dir_home.string(), path_file_shell.string());
    }
  } data;

  /**
   * @brief Construct a new User object with the provided user data
   *
   * @param data The user data for the container environment
   */
  User(UserData const& data)
    : data(data)
  {}

  /**
   * @brief Writes the passwd entry to the provided file path
   *
   * @return Value<void> Nothing on success, or the respective error
   */
  [[nodiscard]] Value<void> write_passwd(fs::path const& path_file_passwd)
  {
    std::ofstream file_passwd{path_file_passwd};
    return_if(not file_passwd.is_open(), Error("E::Failed to open passwd file at {}", path_file_passwd));
    file_passwd << std::string{data} << '\n';
    return {};
  }

  /**
  * @brief Writes the bashrc file and returns its path
  *
  * @return Value<fs::path> The path to the bashrc file
  */
  [[nodiscard]] Value<void> write_bashrc(fs::path const& path_file_bashrc, std::string_view ps1)
  {
    // Open new bashrc file
    std::ofstream file_bashrc{path_file_bashrc};
    return_if(not file_bashrc.is_open(), Error("E::Failed to open bashrc file at {}", path_file_bashrc));
    // Write custom ps1 or default value
    if(ps1.empty())
    {
      file_bashrc << R"(export PS1="[flatimage-${FIM_DIST,,}] \W > ")";
    }
    else
    {
      file_bashrc << "export PS1=" << '"' << ps1 << '"';
    }
    return {};
  }
};

} // namespace ns_proxy

/**
 * @brief Cleans up the bwrap work directory
 *
 * Bwrap leaves behind a 'root' owned empty directory.
 * It is possible to remove without root since it is empty after bwrap is finished.
 * Might take some attempts.
 *
 * @param path_dir_work Path to the bwrap work directory to clean
 * @return Value<void> Nothing on success, or the respective error
 * @note Even if it fails, this won't affect the next program execution since the directory is empty
 *
*/
inline Value<void> bwrap_clean(fs::path const& path_dir_work)
{
  // Check if directory exists
  if(not Try(fs::exists(path_dir_work), "E::Could not check bwrap work directory"))
  {
    return {};
  }
  // Try to change permissions
  if(::chmod(path_dir_work.c_str(), 0755) < 0)
  {
    logger("D::Error to modify permissions '{}': '{}'", path_dir_work, strerror(errno));
  }
  // True if the file was deleted
  // False if it did not exist
  // Exception if it cannot be removed: cannot remove all: Read-only file system'
  // Thus, the check is if it throws or not
  Try(fs::remove_all(path_dir_work), "E::Failed to remove bwrap work directory");
  // Success
  return {};
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


using Permissions = ns_reserved::ns_permissions::Permissions;
using Permission = ns_reserved::ns_permissions::Permission;

struct bwrap_run_ret_t { int code; int syscall_nr; int errno_nr; };

/**
 * @brief Manages bubblewrap (bwrap) containerization
 *
 * Provides a high-level interface for configuring and running processes
 * within isolated bubblewrap containers with customizable bindings,
 * overlays, and permissions.
 */
class Bwrap
{
  private:
    // Logging
    ns_proxy::Logs m_logs;
    // Program to run, its arguments and environment
    fs::path m_path_file_program;
    std::vector<std::string> m_program_args;
    std::vector<std::string> m_program_env;
    // Overlay
    std::optional<ns_proxy::Overlay> m_overlay;
    // Root directory
    fs::path const m_path_dir_root;
    // XDG_RUNTIME_DIR
    fs::path m_path_dir_xdg_runtime;
    // Arguments and environment to bwrap
    std::vector<std::string> m_args;
    // Run bwrap with uid and gid equal to 0
    bool m_is_root;
    // Bwrap native --overlay options
    void overlay(ns_proxy::Overlay const& overlay);
    // Set XDG_RUNTIME_DIR
    void set_xdg_runtime_dir();
    // Setup
    Value<fs::path> test_and_setup(fs::path const& path_file_bwrap);
    Bwrap& symlink_nvidia(fs::path const& path_dir_root_guest, fs::path const& path_dir_root_host);

  public:
    Bwrap(ns_proxy::Logs logs
      , ns_proxy::User user
      , fs::path const& path_dir_root
      , fs::path const& path_file_program
      , std::vector<std::string> const& program_args
      , std::vector<std::string> const& program_env);
    ~Bwrap();
    Bwrap(Bwrap const&) = delete;
    Bwrap(Bwrap&&) = delete;
    Bwrap& operator=(Bwrap const&) = delete;
    Bwrap& operator=(Bwrap&&) = delete;
    [[maybe_unused]] [[nodiscard]] Bwrap& with_binds(ns_db::ns_bind::Binds const& binds);
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_home();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_media();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_audio();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_wayland();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_xorg();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_dbus_user();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_dbus_system();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_udev();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_input();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_usb();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_network();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_shm();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_optical();
    [[maybe_unused]] [[nodiscard]] Bwrap& bind_dev();
    [[maybe_unused]] [[nodiscard]] Bwrap& with_bind_gpu(fs::path const& path_dir_root_guest, fs::path const& path_dir_root_host);
    [[maybe_unused]] [[nodiscard]] Bwrap& with_bind(fs::path const& src, fs::path const& dst);
    [[maybe_unused]] [[nodiscard]] Bwrap& with_bind_ro(fs::path const& src, fs::path const& dst);
    [[maybe_unused]] void set_overlay(ns_proxy::Overlay const& overlay);
    [[maybe_unused]] [[nodiscard]] Value<bwrap_run_ret_t> run(Permissions const& permissions
      , fs::path const& path_file_daemon
      , ns_db::ns_portal::ns_dispatcher::Dispatcher const& arg1_dispatcher
      , ns_db::ns_portal::ns_daemon::Daemon const& arg1_daemon
      , ns_db::ns_portal::ns_daemon::ns_log::Logs const& arg2_daemon
    );
};

 /**
 * @brief Construct a new Bwrap object
 *
 * @param logs Log files used by bwrap (e.g., apparmor logs)
 * @param user The user representation in the bubblewrap container
 * @param path_dir_root Path to the sandbox root directory
 * @param path_file_program Program to launch in the sandbox
 * @param program_args Arguments for the program launched in the sandbox
 * @param program_env Environment variables for the program launched in the sandbox
 */
inline Bwrap::Bwrap(ns_proxy::Logs logs
      , ns_proxy::User user
      , fs::path const& path_dir_root
      , fs::path const& path_file_program
      , std::vector<std::string> const& program_args
      , std::vector<std::string> const& program_env)
  : m_logs(logs)
  , m_path_file_program(path_file_program)
  , m_program_args(program_args)
  , m_program_env()
  , m_overlay(std::nullopt)
  , m_path_dir_root(path_dir_root)
  , m_path_dir_xdg_runtime()
  , m_args()
  , m_is_root(user.data.id.uid == 0)
{
  // Push passed environment
  std::ranges::for_each(program_env, [&](auto&& e){ logger("I::ENV: {}", e); m_program_env.push_back(e); });
  // Configure TERM
  m_program_env.push_back("TERM=xterm");
  // Configure user info
  ns_vector::push_back(m_args
    // Configure user name
    , "--setenv", "USER", std::string{user.data.name}
    // Configure uid and gid
    , "--uid", std::to_string(user.data.id.uid), "--gid", std::to_string(user.data.id.gid)
    // Configure HOME
    ,  "--setenv", "HOME", user.data.path_dir_home.string()
    // Configure SHELL
    ,  "--setenv", "SHELL", user.data.path_file_shell.string()
  );
  // Setup .bashrc
  ns_env::set("BASHRC_FILE", user.data.path_file_bashrc.c_str(), ns_env::Replace::Y);
  // System bindings
  ns_vector::push_back(m_args
    , "--dev", "/dev"
    , "--proc", "/proc"
    , "--bind", "/tmp", "/tmp"
    , "--bind", "/sys", "/sys"
    , "--bind-try", "/etc/group", "/etc/group"
  );
  // Configure passwd file, make it as a rw binding, because some package managers
  // might try to update it.
  ns_vector::push_back(m_args, "--bind-try", user.data.path_file_passwd, "/etc/passwd");
  // Check if XDG_RUNTIME_DIR is set or try to set it manually
  set_xdg_runtime_dir();
}

/**
 * @brief Destroy the Bwrap:: Bwrap object
 */
inline Bwrap::~Bwrap()
{
  if(not m_overlay) { return; }

  ns_bwrap::bwrap_clean(m_overlay->path_dir_work / "work").discard("E::Could not clean bwrap directory");
}

/**
 * @brief Configures the bubblewrap overlay filesystem options
 *
 * @param vec_path_dir_layer Directories to overlay
 * @param path_dir_upper Path to the upper directory where changes are saved to
 * @param path_dir_work Path to the work directory, required by overlayfs
 */
inline void Bwrap::overlay(ns_proxy::Overlay const& overlay)
{
  std::vector<std::string> args;
  // Build --overlay related commands
  for(fs::path const& path_dir_layer : overlay.vec_path_dir_layer)
  {
    logger("I::Overlay layer '{}'", path_dir_layer);
    ns_vector::push_back(args, "--overlay-src", path_dir_layer);
  } // for
  ns_vector::push_back(args, "--overlay", overlay.path_dir_upper, overlay.path_dir_work, "/");
  m_args.insert(m_args.begin(), args.begin(), args.end());
}

/**
 * @brief Configures the XDG_RUNTIME_DIR variable in the sandbox
 */
inline void Bwrap::set_xdg_runtime_dir()
{
  m_path_dir_xdg_runtime = ns_env::get_expected<"W">("XDG_RUNTIME_DIR").value_or(std::format("/run/user/{}", getuid()));
  logger("I::XDG_RUNTIME_DIR: {}", m_path_dir_xdg_runtime);
  m_program_env.push_back(std::format("XDG_RUNTIME_DIR={}", m_path_dir_xdg_runtime.string()));
  ns_vector::push_back(m_args, "--setenv", "XDG_RUNTIME_DIR", m_path_dir_xdg_runtime);
}

/**
 * @brief Runs bubblewrap and tries to integrate with apparmor if execution fails
 *
 * @param path_file_bwrap_src Path to the bubblewrap binary
 * @return The path of the bubblewrap binary greenlit in apparmor
 */
inline Value<fs::path> Bwrap::test_and_setup(fs::path const& path_file_bwrap_src)
{
  // Test current bwrap binary
  using enum ns_subprocess::Stream;
  auto ret = ns_subprocess::Subprocess(path_file_bwrap_src)
    .with_args("--bind", "/", "/", "bash", "-c", "echo")
    .with_stdio(Pipe)
    .spawn()->wait();
  return_if(ret and *ret == 0, path_file_bwrap_src);
  // Try to use bwrap installed by flatimage
  fs::path path_file_bwrap_opt = "/opt/flatimage/bwrap";
  ret = ns_subprocess::Subprocess(path_file_bwrap_opt)
    .with_args("--bind", "/", "/", "bash", "-c", "echo")
    .with_stdio(Pipe)
    .spawn()->wait();
  return_if(ret and *ret == 0, path_file_bwrap_opt);
  // Error might be EACCES, try to integrate with apparmor
  fs::path path_file_pkexec = Pop(ns_env::search_path("pkexec"));
  fs::path path_file_bwrap_apparmor = Pop(ns_env::search_path("fim_bwrap_apparmor"));
  Pop(ns_subprocess::Subprocess(path_file_pkexec)
    .with_args(path_file_bwrap_apparmor, m_logs.path_file_apparmor, path_file_bwrap_src)
    .spawn()->wait());
  return path_file_bwrap_opt;
}

/**
 * @brief Setup symlinks to nvidia drivers
 *
 * @param path_dir_root_guest Path to the root directory of the sandbox
 * @param path_dir_root_host Path to the root directory of the host system (from the guest)
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::symlink_nvidia(fs::path const& path_dir_root_guest, fs::path const& path_dir_root_host)
{
  std::regex regex_exclude("gst|icudata|egl-wayland", std::regex_constants::extended);

  auto f_find_and_bind = [&]<typename... Args>(fs::path const& path_dir_search, Args&&... args) -> void
  {
    std::vector<std::string_view> keywords{std::forward<Args>(args)...};
    return_if(not fs::exists(path_dir_search),, "E::Search path does not exist: '{}'", path_dir_search);
    auto f_process_entry = [&](fs::path const& path_file_entry) -> void
    {
      // Skip ignored matches
      return_if(std::regex_search(path_file_entry.c_str(), regex_exclude),);
      // Skip directories
      return_if(fs::is_directory(path_file_entry),);
      // Skip files that do not match keywords
      return_if(not std::ranges::any_of(keywords, [&](auto&& f){ return path_file_entry.filename().string().contains(f); }),);
      // Symlink target is the file and the end of the symlink chain
      // fs::canonical throws if path_file_entry does not exist
      auto path_file_entry_realpath = fs::canonical(path_file_entry);
      // Create target and symlink names
      fs::path path_link_target = path_dir_root_host / path_file_entry_realpath.relative_path();
      fs::path path_link_name = path_dir_root_guest / path_file_entry.relative_path();
      // File already exists in the container as a regular file or directory, skip
      return_if(fs::exists(path_link_name) and not fs::is_symlink(path_link_name),);
      // Create parent directories
      fs::create_directories(path_link_name.parent_path());
      // Remove existing link
      fs::remove(path_link_name);
      // Symlink
      fs::create_symlink(path_link_target.c_str(), path_link_name.c_str());
      // Log symlink successful
      logger("D::PERM(NVIDIA): {} -> {}", path_link_name, path_link_target);
    };
    // Process entries
    for(auto&& path_file_entry : fs::directory_iterator(path_dir_search) | std::views::transform([](auto&& e){ return e.path(); }))
    {
      Catch(f_process_entry(path_file_entry)).template discard();
    } // for
  };

  // Bind files
  f_find_and_bind("/usr/lib", "nvidia", "cuda", "nvcuvid", "nvoptix");
  f_find_and_bind("/usr/lib/x86_64-linux-gnu", "nvidia", "cuda", "nvcuvid", "nvoptix");
  f_find_and_bind("/usr/lib/i386-linux-gnu", "nvidia", "cuda", "nvcuvid", "nvoptix");
  f_find_and_bind("/usr/bin", "nvidia");
  f_find_and_bind("/usr/share", "nvidia");
  f_find_and_bind("/usr/share/vulkan/icd.d", "nvidia");
  f_find_and_bind("/usr/lib32", "nvidia", "cuda");

  // Bind devices
  for(auto&& entry : fs::directory_iterator("/dev")
    | std::views::transform([](auto&& e){ return e.path(); })
    | std::views::filter([](auto&& e){ return e.filename().string().contains("nvidia"); }))
  {
    ns_vector::push_back(m_args, "--dev-bind-try", entry, entry);
  } // for

  return *this;
}

/**
 * @brief Allows to specify custom bindings from a json file
 *
 * @param path_file_bindings Path to the json file which contains the bindings
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::with_binds(ns_db::ns_bind::Binds const& binds)
{
  using TypeBind = ns_db::ns_bind::Type;
  // Load bindings from the filesystem if any
  for(auto&& bind : binds.get())
  {
    std::string src = bind.path_src;
    std::string dst = bind.path_dst;
    std::string type = (bind.type == TypeBind::DEV)? "--dev-bind-try"
      : (bind.type == TypeBind::RO)? "--ro-bind-try"
      : "--bind-try";
    m_args.push_back(type);
    m_args.push_back(ns_env::expand(src).value_or(src));
    m_args.push_back(ns_env::expand(dst).value_or(dst));
  } // for
  return *this;
}

/**
 * @brief Includes a binding from the host to the guest
 *
 * @param src Source of the binding from the host
 * @param dst Destination of the binding in the guest
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::with_bind(fs::path const& src, fs::path const& dst)
{
  ns_vector::push_back(m_args, "--bind-try", src, dst);
  return *this;
}

/**
 * @brief Includes a read-only binding from the host to the guest
 *
 * @param src Source of the binding from the host
 * @param dst Destination of the binding in the guest
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::with_bind_ro(fs::path const& src, fs::path const& dst)
{
  ns_vector::push_back(m_args, "--ro-bind-try", src, dst);
  return *this;
}

/**
 * @brief Enable bwrap's overlay filesystem
 *
 * @param overlay The overlay configuration object
 */
inline void Bwrap::set_overlay(ns_proxy::Overlay const& overlay)
{
  m_overlay = overlay;
}

/**
 * @brief Includes a binding from the host $HOME to the guest
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_home()
{
  if ( m_is_root ) { return *this; }
  logger("D::PERM(HOME)");
  std::string str_dir_home = ({
    auto ret = ns_env::get_expected("HOME");
    return_if(not ret, *this, "E::HOME environment variable is unset");
    ret.value();
  });
  ns_vector::push_back(m_args, "--bind-try", str_dir_home, str_dir_home);
  return *this;
}

/**
 * @brief Binds the host's media directories to the guest
 *
 * The bindings are `/media`, `/run/media`, and `/mnt`
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_media()
{
  logger("D::PERM(MEDIA)");
  ns_vector::push_back(m_args, "--bind-try", "/media", "/media");
  ns_vector::push_back(m_args, "--bind-try", "/run/media", "/run/media");
  ns_vector::push_back(m_args, "--bind-try", "/mnt", "/mnt");
  return *this;
}

/**
 * @brief Binds the host's audio sockets and devices to the guest
 *
 * The bindings are $XDG_RUNTIME_DIR/{/pulse/native,pipewire-0}
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_audio()
{
  logger("D::PERM(AUDIO)");

  // Try to bind pulse socket
  fs::path path_socket_pulse = m_path_dir_xdg_runtime / "pulse/native";
  ns_vector::push_back(m_args, "--bind-try", path_socket_pulse, path_socket_pulse);
  ns_vector::push_back(m_args, "--setenv", "PULSE_SERVER", "unix:" + path_socket_pulse.string());

  // Try to bind pipewire socket
  fs::path path_socket_pipewire = m_path_dir_xdg_runtime / "pipewire-0";
  ns_vector::push_back(m_args, "--bind-try", path_socket_pipewire, path_socket_pipewire);

  // Other paths required to sound
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev/dsp", "/dev/dsp");
  ns_vector::push_back(m_args, "--bind-try", "/dev/snd", "/dev/snd");
  ns_vector::push_back(m_args, "--bind-try", "/proc/asound", "/proc/asound");

  return *this;
}

/**
 * @brief Binds the wayland socket from the host to the guest
 *
 * Requires the WAYLAND_DISPLAY variable set
 * The binding is $XDG_RUNTIME_DIR/$WAYLAND_DISPLAY
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_wayland()
{
  logger("D::PERM(WAYLAND)");
  // Get WAYLAND_DISPLAY
  std::string env_wayland_display = ({
    auto ret = ns_env::get_expected("WAYLAND_DISPLAY");
    return_if(not ret, *this, "E::WAYLAND_DISPLAY is undefined");
    ret.value();
  });

  // Get wayland socket
  fs::path path_socket_wayland = m_path_dir_xdg_runtime / env_wayland_display;

  // Bind
  ns_vector::push_back(m_args, "--bind-try", path_socket_wayland, path_socket_wayland);
  ns_vector::push_back(m_args, "--setenv", "WAYLAND_DISPLAY", env_wayland_display);

  return *this;
}

/**
 * @brief Binds the xorg socket from the host to the guest
 *
 * Requires the DISPLAY environment variable set
 * Requires the XAUTHORITY environment variable set
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_xorg()
{
  logger("D::PERM(XORG)");
  // Get DISPLAY
  std::string env_display = ({
    auto ret = ns_env::get_expected("DISPLAY");
    return_if(not ret, *this, "E::DISPLAY is undefined");
    ret.value();
  });
  // Get XAUTHORITY
  std::string env_xauthority = ({
    auto ret = ns_env::get_expected("XAUTHORITY");
    return_if(not ret, *this, "E::XAUTHORITY is undefined");
    ret.value();
  });
  // Bind
  ns_vector::push_back(m_args, "--ro-bind-try", env_xauthority, env_xauthority);
  ns_vector::push_back(m_args, "--setenv", "XAUTHORITY", env_xauthority);
  ns_vector::push_back(m_args, "--setenv", "DISPLAY", env_display);
  return *this;
}

/**
 * @brief Binds the user session bus from the host to the guest
 *
 * Requires the DBUS_SESSION_BUS_ADDRESS environment variable set
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_dbus_user()
{
  logger("D::PERM(DBUS_USER)");
  // Get DBUS_SESSION_BUS_ADDRESS
  std::string env_dbus_session_bus_address = ({
    auto ret = ns_env::get_expected("DBUS_SESSION_BUS_ADDRESS");
    return_if(not ret, *this, "E::DBUS_SESSION_BUS_ADDRESS is undefined");
    ret.value();
  });

  // Path to current session bus
  std::string str_dbus_session_bus_path = env_dbus_session_bus_address;

  // Variable has expected contents similar to: 'unix:path=/run/user/1000/bus,guid=bb1adf978ae9c14....'
  // Erase until the first '=' (inclusive)
  if ( auto pos = str_dbus_session_bus_path.find('/'); pos != std::string::npos )
  {
    str_dbus_session_bus_path.erase(0, pos);
  } // if

  // Erase from the first ',' (inclusive)
  if ( auto pos = str_dbus_session_bus_path.find(','); pos != std::string::npos )
  {
    str_dbus_session_bus_path.erase(pos);
  } // if

  // Bind
  ns_vector::push_back(m_args, "--setenv", "DBUS_SESSION_BUS_ADDRESS", env_dbus_session_bus_address);
  ns_vector::push_back(m_args, "--bind-try", str_dbus_session_bus_path, str_dbus_session_bus_path);

  return *this;
}

/**
 * @brief Binds the syst from the host to the guest
 *
 * the binding is `/run/dbus/system_bus_socket`
 *
 * @return bwrap& a reference to *this
 */
inline Bwrap& Bwrap::bind_dbus_system()
{
  logger("D::PERM(DBUS_SYSTEM)");
  ns_vector::push_back(m_args, "--bind-try", "/run/dbus/system_bus_socket", "/run/dbus/system_bus_socket");
  return *this;
}

/**
 * @brief binds the udev folder from the host to the guest
 *
 * The binding is `/run/udev`
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_udev()
{
  logger("D::PERM(UDEV)");
  ns_vector::push_back(m_args, "--bind-try", "/run/udev", "/run/udev");
  return *this;
}

/**
 * @brief Binds the input devices from the host to the guest
 *
 * The bindings are `/dev/{input,uinput}`
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_input()
{
  logger("D::PERM(INPUT)");
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev/input", "/dev/input");
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev/uinput", "/dev/uinput");
  return *this;
}

/**
 * @brief Binds the usb devices from the host to the guest
 *
 * The bindings are `/dev/bus/usb` and `/dev/usb`
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_usb()
{
  logger("D::PERM(USB)");
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev/bus/usb", "/dev/bus/usb");
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev/usb", "/dev/usb");
  return *this;
}

/**
 * @brief Binds the network configuration from the host to the guest
 *
 * The bindings are:
 * - `/etc/host.conf`
 * - `/etc/hosts`
 * - `/etc/nsswitch.conf`
 * - `/etc/resolv.conf`
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_network()
{
  logger("D::PERM(NETWORK)");
  ns_vector::push_back(m_args, "--ro-bind-try", "/etc/host.conf", "/etc/host.conf");
  ns_vector::push_back(m_args, "--ro-bind-try", "/etc/hosts", "/etc/hosts");
  ns_vector::push_back(m_args, "--ro-bind-try", "/etc/nsswitch.conf", "/etc/nsswitch.conf");
  ns_vector::push_back(m_args, "--ro-bind-try", "/etc/resolv.conf", "/etc/resolv.conf");
  return *this;
}

/**
 * @brief Binds the /dev/shm directory to the containter
 *
 * A tmpfs mount used for POSIX shared memory
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_shm()
{
  logger("D::PERM(SHM)");
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev/shm", "/dev/shm");
  return *this;
}

/**
 * @brief Binds optical devices to the container
 *
 * Grants access to optical devices such as CD and DVD drives.
 * The maximum number of scsi devices is defined in the linux kernel
 * as [#define SR_DISKS	256](https://github.com/torvalds/linux/blob/master/drivers/scsi/sr.c).
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_optical()
{
  logger("D::PERM(OPTICAL)");
  auto f_bind = [this](fs::path const& path_device) -> bool
  {
    auto __expected_fn = [](auto&&){ return false; };
    if(not Try(fs::exists(path_device), "E::Error to check if file exists: {}", path_device))
    {
      return false;
    }
    ns_vector::push_back(m_args, "--dev-bind-try", path_device, path_device);
    return true;
  };
  // Bind /dev/sr[0-255] and /dev/sg[0-255]
  // Devices are sequential, so stop when both sr and sg don't exist
  for(int i : std::views::iota(0,256))
  {
    bool sr_exists = f_bind(std::format("/dev/sr{}", i));
    bool sg_exists = f_bind(std::format("/dev/sg{}", i));
    // Stop if neither device exists
    if (!sr_exists && !sg_exists) break;
  }
  return *this;
}

/**
 * @brief Binds the /dev directory to the containter
 *
 * Superseeds all previous /dev related bindings
 *
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_dev()
{
  logger("D::PERM(DEV)");
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev", "/dev");
  return *this;
}


/**
 * @brief Binds the gpu device from the host to the guest
 *
 * @param path_dir_root_guest Path to the root directory of the sandbox
 * @param path_dir_root_host Path to the root directory of the host system (from the guest)
 * @return Bwrap&
 */
inline Bwrap& Bwrap::with_bind_gpu(fs::path const& path_dir_root_guest, fs::path const& path_dir_root_host)
{
  logger("D::PERM(GPU)");
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev/dri", "/dev/dri");
  return symlink_nvidia(path_dir_root_guest, path_dir_root_host);
}

/**
 * @brief Runs the command in the bubblewrap sandbox
 *
 * @param permissions Permissions for the program (HOME, MEDIA, AUDIO, etc.), configured in bubblewrap
 * @param path_file_daemon Path to the portal daemon executable
 * @param arg1_dispatcher Dispatcher configuration for the portal communication (controls FIFO communication between host and container)
 * @param arg1_daemon Daemon host configuration for the portal (controls daemon communication settings)
 * @param arg2_daemon Log configuration for the portal daemon (specifies log file paths and settings)
 * @return Value<bwrap_run_ret_t> Return value containing exit code, syscall number, and errno on error
 */
inline Value<bwrap_run_ret_t> Bwrap::run(Permissions const& permissions
  , fs::path const& path_file_daemon
  , ns_db::ns_portal::ns_dispatcher::Dispatcher const& arg1_dispatcher
  , ns_db::ns_portal::ns_daemon::Daemon const& arg1_daemon
  , ns_db::ns_portal::ns_daemon::ns_log::Logs const& arg2_daemon)
{
  // Configure bindings
  if(permissions.contains(Permission::HOME)){ std::ignore = bind_home(); };
  if(permissions.contains(Permission::MEDIA)){ std::ignore = bind_media(); };
  if(permissions.contains(Permission::AUDIO)){ std::ignore = bind_audio(); };
  if(permissions.contains(Permission::WAYLAND)){ std::ignore = bind_wayland(); };
  if(permissions.contains(Permission::XORG)){ std::ignore = bind_xorg(); };
  if(permissions.contains(Permission::DBUS_USER)){ std::ignore = bind_dbus_user(); };
  if(permissions.contains(Permission::DBUS_SYSTEM)){ std::ignore = bind_dbus_system(); };
  if(permissions.contains(Permission::UDEV)){ std::ignore = bind_udev(); };
  if(permissions.contains(Permission::INPUT)){ std::ignore = bind_input(); };
  if(permissions.contains(Permission::USB)){ std::ignore = bind_usb(); };
  if(permissions.contains(Permission::NETWORK)){ std::ignore = bind_network(); };
  if(permissions.contains(Permission::SHM)){ std::ignore = bind_shm(); };
  if(permissions.contains(Permission::OPTICAL)){ std::ignore = bind_optical(); };
  if(permissions.contains(Permission::DEV)){ std::ignore = bind_dev(); };

  // Search for bash
  fs::path path_file_bash = Pop(ns_env::search_path("bash"));

  // Use builtin bwrap or native if exists
  fs::path path_file_bwrap = Pop(ns_env::search_path("bwrap"));

  // Test bwrap and setup apparmor if it is required
  // Adjust the bwrap path to the one integrated with apparmor if needed
  path_file_bwrap = Pop(test_and_setup(path_file_bwrap));

  // Pipe to receive errors from bwrap
  int pipe_error[2];
  return_if(pipe(pipe_error) == -1, Error("E::Could not open bwrap error pipe: {}", strerror(errno)));

  // Configure pipe read end as non-blocking
  if(fcntl(pipe_error[0], F_SETFL, fcntl(pipe_error[0], F_GETFL, 0) | O_NONBLOCK) < 0)
  {
    return Error("E::Could not configure bwrap pipe to be non-blocking");
  }

  // Get path to daemon
  std::string str_arg1_dispatcher = Pop(ns_db::ns_portal::ns_dispatcher::serialize(arg1_dispatcher));
  std::string str_arg1_daemon = Pop(ns_db::ns_portal::ns_daemon::serialize(arg1_daemon));
  std::string str_arg2_daemon = Pop(ns_db::ns_portal::ns_daemon::ns_log::serialize(arg2_daemon));

  // Pass daemon configuration via environment variables to avoid shell escaping issues
  // The daemon will read FIM_DAEMON_CFG and FIM_DAEMON_LOG from environment
  ns_vector::push_back(m_args, "--setenv", "FIM_DISPATCHER_CFG", str_arg1_dispatcher);
  ns_vector::push_back(m_args, "--setenv", "FIM_DAEMON_CFG", str_arg1_daemon);
  ns_vector::push_back(m_args, "--setenv", "FIM_DAEMON_LOG", str_arg2_daemon);

  return_if(not Try(fs::exists(path_file_daemon)), Error("E::Missing portal daemon to run binary file path"));

  // Configure bwrap overlay filesystem if required
  if(m_overlay)
  {
    overlay(m_overlay.value());
  }
  else
  {
    ns_vector::push_front(m_args, "--bind", m_path_dir_root, "/");
  }

  // Run Bwrap
  auto code = ns_subprocess::Subprocess(path_file_bash)
    .with_args("-c", std::format(R"("{}" "$@")", path_file_bwrap.string()), "--")
    .with_args("--error-fd", std::to_string(pipe_error[1]))
    .with_args(m_args)
    .with_args(path_file_bash, "-c", std::format(R"(&>/dev/null nohup "{}" & disown; "{}" "$@")", path_file_daemon.string(), m_path_file_program.string()), "--")
    .with_args(m_program_args)
    .with_env(m_program_env)
    .spawn()->wait().value_or(125);

  // Failed syscall and errno
  int syscall_nr = -1;
  int errno_nr = -1;

  // Read possible errors if any
  log_if(read(pipe_error[0], &syscall_nr, sizeof(syscall_nr)) < 0, "D::Could not read syscall error, success?");
  log_if(read(pipe_error[0], &errno_nr, sizeof(errno_nr)) < 0, "D::Could not read errno number, success?");

  // Close pipe
  close(pipe_error[0]);
  close(pipe_error[1]);

  // Return value and possible errors
  return bwrap_run_ret_t{.code=code, .syscall_nr=syscall_nr, .errno_nr=errno_nr};
}

} // namespace ns_bwrap

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
