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
    ~Portal();
    friend Value<std::unique_ptr<Portal>> spawn(Daemon const& daemon, Logs const& logs);
    friend constexpr std::unique_ptr<Portal> std::make_unique<Portal>();
};

inline Portal::Portal()
  : m_child(nullptr)
{
}

inline Portal::~Portal()
{
  if (m_child)
  {
    m_child->kill(SIGTERM);
    std::ignore = m_child->wait();
  }
}

[[nodiscard]] inline Value<std::unique_ptr<Portal>> spawn(Daemon const& daemon, Logs const& logs)
{
  auto portal = std::make_unique<Portal>();
  // Path to daemon
  fs::path path_bin_daemon = daemon.get_path_bin_daemon();
  qreturn_if(portal == nullptr, Error("E::Could not create portal object"));
  qreturn_if(not Try(fs::exists(path_bin_daemon)), Error("E::Daemon not found in {}", path_bin_daemon));
  // Create a portal that uses the reference file to create an unique communication key
  // Spawn process to background
  using enum ns_subprocess::Stream;
  portal->m_child = ns_subprocess::Subprocess(path_bin_daemon)
    .with_var("FIM_DAEMON_CFG", Pop(ns_daemon::serialize(daemon)))
    .with_var("FIM_DAEMON_LOG", Pop(ns_daemon::ns_log::serialize(logs)))
    .with_log_stdio()
    .spawn();
  return portal;
}

} // namespace ns_portal

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
