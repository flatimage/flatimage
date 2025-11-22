/**
 * @file portal.hpp
 * @author Ruan Formigoni
 * @brief Spawns a portal daemon process
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#include <filesystem>
#include <memory>

#include "../std/expected.hpp"
#include "../lib/env.hpp"
#include "../lib/subprocess.hpp"
#include "../db/portal/daemon.hpp"

/**
 * @namespace ns_portal
 * @brief Portal IPC system for host-container communication
 *
 * Manages the portal daemon lifecycle for FIFO-based inter-process communication between
 * host and container environments. Spawns and controls portal daemon processes that enable
 * cross-boundary command execution, stdio redirection, and message routing for multi-instance
 * FlatImage applications.
 */
namespace ns_portal
{

namespace
{

namespace fs = std::filesystem;

} // anonymous namespace

using Daemon = ns_db::ns_portal::ns_daemon::Daemon;
using Logs = ns_db::ns_portal::ns_daemon::ns_log::Logs;

namespace ns_daemon = ns_db::ns_portal::ns_daemon;

/**
 * @brief Manages portal daemon for inter-process communication
 *
 * The Portal class provides FIFO-based IPC between host and container
 * processes, managing the daemon lifecycle and child process communication.
 */
class Portal
{
  private:
    std::unique_ptr<ns_subprocess::Child> m_child;
    Portal();

  public:
    friend Value<std::unique_ptr<Portal>> spawn(Daemon const& daemon, Logs const& logs);
    friend constexpr std::unique_ptr<Portal> std::make_unique<Portal>();
};


/**
  * @brief Private constructor, use spawn() to create instances
  */
inline Portal::Portal()
  : m_child(nullptr)
{
}

/**
 * @brief Spawns a new portal daemon instance
 *
 * @param daemon Daemon configuration
 * @param logs Logging configuration
 * @return Value containing unique pointer to Portal or error
 */
[[nodiscard]] inline Value<std::unique_ptr<Portal>> spawn(Daemon const& daemon, Logs const& logs)
{
  auto portal = std::make_unique<Portal>();
  // Path to daemon
  fs::path path_bin_daemon = daemon.get_path_bin_daemon();
  return_if(portal == nullptr, Error("E::Could not create portal object"));
  return_if(not Try(fs::exists(path_bin_daemon)), Error("E::Daemon not found in {}", path_bin_daemon));
  // Create a portal that uses the reference file to create an unique communication key
  // Spawn process to background
  portal->m_child = ns_subprocess::Subprocess(path_bin_daemon)
    .with_var("FIM_DAEMON_CFG", Pop(ns_daemon::serialize(daemon)))
    .with_var("FIM_DAEMON_LOG", Pop(ns_daemon::ns_log::serialize(logs)))
    .with_daemon()
    .spawn();
  return portal;
}

} // namespace ns_portal

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
