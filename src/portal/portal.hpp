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

namespace ns_portal
{

namespace
{

namespace fs = std::filesystem;

} // anonymous namespace

using Daemon = ns_db::ns_portal::ns_daemon::Daemon;
using Logs = ns_db::ns_portal::ns_daemon::ns_log::Logs;

namespace ns_daemon = ns_db::ns_portal::ns_daemon;

class Portal
{
  private:
    std::unique_ptr<ns_subprocess::Child> m_child;
    Portal();

  public:
    friend Value<std::unique_ptr<Portal>> spawn(Daemon const& daemon, Logs const& logs);
    friend constexpr std::unique_ptr<Portal> std::make_unique<Portal>();
};

inline Portal::Portal()
  : m_child(nullptr)
{
}

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
    .with_stdio(ns_subprocess::Stream::Pipe)
    .with_streams(ns_subprocess::stream::null(), std::cout, std::cerr)
    .with_daemon()
    .spawn();
  return portal;
}

} // namespace ns_portal

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
