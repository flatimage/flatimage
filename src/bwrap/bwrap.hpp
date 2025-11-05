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
#include "../reserved/permissions.hpp"
#include "../std/expected.hpp"
#include "../std/vector.hpp"
#include "../lib/log.hpp"
#include "../lib/subprocess.hpp"
#include "../lib/env.hpp"
#include "../macro.hpp"

namespace ns_bwrap
{

namespace
{

namespace fs = std::filesystem;

}

struct Overlay
{
  std::vector<fs::path> vec_path_dir_layer;
  fs::path path_dir_upper;
  fs::path path_dir_work;
};

using Permissions = ns_reserved::ns_permissions::Permissions;
using Permission = ns_reserved::ns_permissions::Permission;

struct bwrap_run_ret_t { int code; int syscall_nr; int errno_nr; };

class Bwrap
{
  private:
    // Program to run, its arguments and environment
    fs::path m_path_file_program;
    std::vector<std::string> m_program_args;
    std::vector<std::string> m_program_env;
    // XDG_RUNTIME_DIR
    fs::path m_path_dir_xdg_runtime;
    // Overlay work dir, bwrap leaves this directory with
    // 000 permissions after usage, save it to make it 755
    // on exit
    std::optional<fs::path> m_opt_path_dir_work;
    // Arguments and environment to bwrap
    std::vector<std::string> m_args;
    // Run bwrap with uid and gid equal to 0
    bool m_is_root;
    // Bwrap native --overlay options
    void overlay(std::vector<fs::path> const& vec_path_dir_layer
      , fs::path const& path_dir_upper
      , fs::path const& path_dir_work);
    // Set XDG_RUNTIME_DIR
    void set_xdg_runtime_dir();
    // Setup
    Value<fs::path> test_and_setup(fs::path const& path_file_bwrap);

  public:
    Bwrap(std::string_view user
      , mode_t uid
      , mode_t gid
      , std::optional<Overlay> opt_overlay
      , fs::path const& path_dir_root
      , fs::path const& path_dir_home
      , fs::path const& path_file_bashrc
      , fs::path const& path_file_passwd
      , fs::path const& path_file_shell
      , fs::path const& path_file_program
      , std::vector<std::string> const& program_args
      , std::vector<std::string> const& program_env);
    ~Bwrap();
    Bwrap(Bwrap const&) = delete;
    Bwrap(Bwrap&&) = delete;
    Bwrap& operator=(Bwrap const&) = delete;
    Bwrap& operator=(Bwrap&&) = delete;
    [[maybe_unused]] [[nodiscard]] Bwrap& symlink_nvidia(fs::path const& path_dir_root_guest, fs::path const& path_dir_root_host);
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
    [[maybe_unused]] [[nodiscard]] Value<bwrap_run_ret_t> run(Permissions const& permissions
      , fs::path const& path_dir_app_bin);
};

 /**
 * @brief Construct a new Bwrap:: Bwrap object
 *
 * @param is_root A flag to set the sandbox user as root
 * @param opt_overlay Optional overlay filesystem configuration
 * @param path_dir_root Path to the sandbox root directory
 * @param path_file_bashrc Path to the `.bashrc` file used by the sandbox shell
 * @param path_file_passwd Path to the passwd file to bind
 * @param path_file_program Program to launch in the sandbox
 * @param program_args Arguments for the program launched in the sandbox
 * @param program_env Environment for the program launched in the sandbox
 */
inline Bwrap::Bwrap(std::string_view user
    , mode_t uid
    , mode_t gid
    , std::optional<Overlay> opt_overlay
    , fs::path const& path_dir_root
    , fs::path const& path_dir_home
    , fs::path const& path_file_bashrc
    , fs::path const& path_file_passwd
    , fs::path const& path_file_shell
    , fs::path const& path_file_program
    , std::vector<std::string> const& program_args
    , std::vector<std::string> const& program_env)
  : m_path_file_program(path_file_program)
  , m_program_args(program_args)
  , m_opt_path_dir_work(opt_overlay.transform([](auto&& e){ return e.path_dir_work; }))
  , m_is_root(uid == 0)
{
  // Push passed environment
  std::ranges::for_each(program_env, [&](auto&& e){ ns_log::info()("ENV: {}", e); m_program_env.push_back(e); });
  // Configure TERM
  m_program_env.push_back("TERM=xterm");
  // Configure user info
  ns_vector::push_back(m_args
    // Configure user name
    , "--setenv", "USER", std::string{user}
    // Configure uid and gid
    , "--uid", std::to_string(uid), "--gid", std::to_string(gid)
    // Configure HOME
    ,  "--setenv", "HOME", path_dir_home.string()
    // Configure SHELL
    ,  "--setenv", "SHELL", path_file_shell.string()
  );
  // Setup .bashrc
  ns_env::set("BASHRC_FILE", path_file_bashrc.c_str(), ns_env::Replace::Y);
  // Use native bwrap --overlay options or overlayfs
  if ( opt_overlay )
  {
    overlay(opt_overlay->vec_path_dir_layer
      , opt_overlay->path_dir_upper
      , opt_overlay->path_dir_work
    );
  }
  else
  {
    elog_if(not fs::is_directory(path_dir_root)
      , "'{}' does not exist or is not a directory"_fmt(path_dir_root)
    );
    ns_vector::push_back(m_args, "--bind", path_dir_root, "/");
  }
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
  ns_vector::push_back(m_args, "--bind-try", path_file_passwd, "/etc/passwd");
  // Check if XDG_RUNTIME_DIR is set or try to set it manually
  set_xdg_runtime_dir();
}

/**
 * @brief Destroy the Bwrap:: Bwrap object
 */
inline Bwrap::~Bwrap()
{
}

/**
 * @brief Configures the bubblewrap overlay filesystem options
 * 
 * @param vec_path_dir_layer Directories to overlay
 * @param path_dir_upper Path to the upper directory where changes are saved to
 * @param path_dir_work Path to the work directory, required by overlayfs
 */
inline void Bwrap::overlay(std::vector<fs::path> const& vec_path_dir_layer
  , fs::path const& path_dir_upper
  , fs::path const& path_dir_work)
{
  // Build --overlay related commands
  for(fs::path const& path_dir_layer : vec_path_dir_layer)
  {
    ns_log::info()("Overlay layer '{}'", path_dir_layer);
    ns_vector::push_back(m_args, "--overlay-src", path_dir_layer);
  } // for
  ns_vector::push_back(m_args, "--overlay", path_dir_upper, path_dir_work, "/");
}

/**
 * @brief Configures the XDG_RUNTIME_DIR variable in the sandbox
 */
inline void Bwrap::set_xdg_runtime_dir()
{
  m_path_dir_xdg_runtime = ns_env::get_expected("XDG_RUNTIME_DIR").value_or("/run/user/{}"_fmt(getuid()));
  ns_log::info()("XDG_RUNTIME_DIR: {}", m_path_dir_xdg_runtime);
  m_program_env.push_back("XDG_RUNTIME_DIR={}"_fmt(m_path_dir_xdg_runtime));
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
  auto ret = ns_subprocess::Subprocess(path_file_bwrap_src)
    .with_piped_outputs()
    .with_args("--bind", "/", "/", "bash", "-c", "echo")
    .spawn()
    .wait();
  qreturn_if (ret and *ret == 0, path_file_bwrap_src);
  // Try to use bwrap installed by flatimage
  fs::path path_file_bwrap_opt = "/opt/flatimage/bwrap";
  ret = ns_subprocess::Subprocess(path_file_bwrap_opt)
    .with_piped_outputs()
    .with_args("--bind", "/", "/", "bash", "-c", "echo")
    .spawn()
    .wait();
  qreturn_if (ret and *ret == 0, path_file_bwrap_opt);
  // Error might be EACCES, try to integrate with apparmor
  fs::path path_file_pkexec = Pop(ns_env::search_path("pkexec"));
  fs::path path_file_bwrap_apparmor = Pop(ns_env::search_path("fim_bwrap_apparmor"));
  fs::path path_dir_mount = Pop(ns_env::get_expected("FIM_DIR_MOUNT"));
  ret = ns_subprocess::Subprocess(path_file_pkexec)
    .with_args(path_file_bwrap_apparmor, path_dir_mount, path_file_bwrap_src)
    .spawn()
    .wait();
  qreturn_if(not ret, Error("E::Could not find create profile (abnormal exit)"));
  qreturn_if(ret and *ret != 0, Error("E::Could not find create profile with exit code '{}'", *ret));
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

  auto f_find_and_bind = [&]<typename... Args>(fs::path const& path_dir_search, Args&&... args)
  {
    std::vector<std::string_view> keywords{std::forward<Args>(args)...};
    ireturn_if(not fs::exists(path_dir_search), "Search path does not exist: '{}'"_fmt(path_dir_search));
    for(auto&& path_file_entry : fs::directory_iterator(path_dir_search) | std::views::transform([](auto&& e){ return e.path(); }))
    {
      // Skip ignored matches
      dcontinue_if(std::regex_search(path_file_entry.c_str(), regex_exclude), "Ignoring match '{}'"_fmt(path_file_entry));
      // Skip directories
      qcontinue_if(fs::is_directory(path_file_entry));
      // Skip files that do not match keywords
      qcontinue_if(not std::ranges::any_of(keywords, [&](auto&& f){ return path_file_entry.filename().string().contains(f); }));
      // Symlink target is the file and the end of the symlink chain
      auto path_file_entry_realpath = Catch(fs::canonical(path_file_entry));
      econtinue_if(not path_file_entry_realpath, "Broken symlink: '{}'"_fmt(path_file_entry));
      // Create target and symlink names
      fs::path path_link_target = path_dir_root_host / path_file_entry_realpath->relative_path();
      fs::path path_link_name = path_dir_root_guest / path_file_entry.relative_path();
      // File already exists in the container as a regular file or directory, skip
      qcontinue_if(fs::exists(path_link_name) and not fs::is_symlink(path_link_name));
      // Create parent directories
      std::error_code ec;
      fs::create_directories(path_link_name.parent_path(), ec);
      econtinue_if (ec, ec.message());
      // Remove existing link
      fs::remove(path_link_name, ec);
      // Symlink
      econtinue_if(symlink(path_link_target.c_str(), path_link_name.c_str()) < 0, "{}: {}"_fmt(strerror(errno), path_link_name));
      // Log symlink successful
      ns_log::debug()("PERM(NVIDIA): {} -> {}", path_link_name, path_link_target);
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
 * @brief Includes a binding from the host $HOME to the guest
 * 
 * @return Bwrap& A reference to *this
 */
inline Bwrap& Bwrap::bind_home()
{
  if ( m_is_root ) { return *this; }
  ns_log::debug()("PERM(HOME)");
  std::string str_dir_home = ({
    auto ret = ns_env::get_expected("HOME");
    ereturn_if(not ret, "HOME environment variable is unset", *this);
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
  ns_log::debug()("PERM(MEDIA)");
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
  ns_log::debug()("PERM(AUDIO)");

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
  ns_log::debug()("PERM(WAYLAND)");
  // Get WAYLAND_DISPLAY
  std::string env_wayland_display = ({
    auto ret = ns_env::get_expected("WAYLAND_DISPLAY");
    ereturn_if(not ret, "WAYLAND_DISPLAY is undefined", *this);
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
  ns_log::debug()("PERM(XORG)");
  // Get DISPLAY
  std::string env_display = ({
    auto ret = ns_env::get_expected("DISPLAY");
    ereturn_if(not ret, "DISPLAY is undefined", *this);
    ret.value();
  });
  // Get XAUTHORITY
  std::string env_xauthority = ({
    auto ret = ns_env::get_expected("XAUTHORITY");
    ereturn_if(not ret, "XAUTHORITY is undefined", *this);
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
  ns_log::debug()("PERM(DBUS_USER)");
  // Get DBUS_SESSION_BUS_ADDRESS
  std::string env_dbus_session_bus_address = ({
    auto ret = ns_env::get_expected("DBUS_SESSION_BUS_ADDRESS");
    ereturn_if(not ret, "DBUS_SESSION_BUS_ADDRESS is undefined", *this);
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
  ns_log::debug()("PERM(DBUS_SYSTEM)");
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
  ns_log::debug()("PERM(UDEV)");
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
  ns_log::debug()("PERM(INPUT)");
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
  ns_log::debug()("PERM(USB)");
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
  ns_log::debug()("PERM(NETWORK)");
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
  ns_log::debug()("PERM(SHM)");
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
  ns_log::debug()("PERM(OPTICAL)");
  auto f_bind = [this](fs::path const& path_device)
  {
    std::error_code ec; 
    return fs::exists(path_device, ec)?
        (ns_vector::push_back(m_args, "--dev-bind-try", path_device, path_device), true)
      : (({elog_if(ec, "Error to check if file exists: {}"_fmt(ec.message()))}), false);
  };
  // Bind /dev/sr[0-255] and /dev/sg[0-255]
  for(int i : std::views::iota(0,256))
  {
    qbreak_if(not f_bind("/dev/sr{}"_fmt(i)) and not f_bind("/dev/sg{}"_fmt(i)));
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
  ns_log::debug()("PERM(DEV)");
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
  ns_log::debug()("PERM(GPU)");
  ns_vector::push_back(m_args, "--dev-bind-try", "/dev/dri", "/dev/dri");
  return symlink_nvidia(path_dir_root_guest, path_dir_root_host);
}

/**
 * @brief Runs the command in the bubblewrap sandbox
 * 
 * @param permissions Permissions for the program, configured in bubblewrap
 * @param path_dir_app_bin Path to the binary directory of flatimage's binary files
 * @return bwrap_run_ret_t 
 */
inline Value<bwrap_run_ret_t> Bwrap::run(Permissions const& permissions
  , fs::path const& path_dir_app_bin)
{
  std::error_code ec;
  
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
  qreturn_if(pipe(pipe_error) == -1, strerror(errno), Error("E::Could not open bwrap error pipe"));

  // Configure pipe read end as non-blocking
  if(fcntl(pipe_error[0], F_SETFL, fcntl(pipe_error[0], F_GETFL, 0) | O_NONBLOCK) < 0)
  {
    return Error("E::Could not configure bwrap pipe to be non-blocking");
  }

  // Get path to daemon
  fs::path path_file_daemon = path_dir_app_bin / "fim_portal_daemon";
  qreturn_if(not fs::exists(path_file_daemon, ec), Error("E::Missing portal daemon to run binary file path"));

  // Run Bwrap
  auto code = ns_subprocess::Subprocess(path_file_bash)
    .with_args("-c", R"("{}" "$@")"_fmt(path_file_bwrap), "--")
    .with_args("--error-fd", std::to_string(pipe_error[1]))
    .with_args(m_args)
    .with_args(path_file_bash, "-c", R"(&>/dev/null nohup "{}" "{}" guest & disown; "{}" "$@")"_fmt(path_file_daemon, getpid(), m_path_file_program), "--")
    .with_args(m_program_args)
    .with_env(m_program_env)
    .spawn()
    .wait();
  if ( not code ) { ns_log::error()("bwrap exited abnormally"); }
  if ( code.value() != 0 ) { ns_log::error()("bwrap exited with non-zero exit code '{}'", code.value()); }

  // Failed syscall and errno
  int syscall_nr = -1;
  int errno_nr = -1;

  // Read possible errors if any
  wlog_if(read(pipe_error[0], &syscall_nr, sizeof(syscall_nr)) < 0, "Could not read syscall error, success?");
  wlog_if(read(pipe_error[0], &errno_nr, sizeof(errno_nr)) < 0, "Could not read errno number, success?");

  // Close pipe
  close(pipe_error[0]);
  close(pipe_error[1]);

  // Return value and possible errors
  return bwrap_run_ret_t{.code=code.value_or(125), .syscall_nr=syscall_nr, .errno_nr=errno_nr};
}

} // namespace ns_bwrap

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
