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

namespace ns_portal
{

namespace
{

namespace fs = std::filesystem;

} // anonymous namespace

class Portal
{
  private:
    std::unique_ptr<ns_subprocess::Subprocess> m_process;
    fs::path m_path_file_daemon;
    fs::path m_path_file_guest;
    Portal();

  public:
    ~Portal();
    friend Value<std::unique_ptr<Portal>> create(pid_t const pid_reference, std::string const& mode);
    friend constexpr std::unique_ptr<Portal> std::make_unique<Portal>();
};

inline Portal::Portal()
  : m_process(nullptr)
  , m_path_file_daemon()
  , m_path_file_guest()
{
}

inline Portal::~Portal()
{
  m_process->kill(SIGTERM);
  std::ignore = m_process->wait();
}

[[nodiscard]] inline Value<std::unique_ptr<Portal>> create(pid_t const pid_reference, std::string const& mode)
{
  auto portal = std::make_unique<Portal>();
  qreturn_if(portal == nullptr, Error("E::Could not create portal object"));
  // Path to flatimage binaries
  std::string str_dir_app_bin = Pop(ns_env::get_expected("FIM_DIR_APP_BIN"));
  // Create path to daemon
  portal->m_path_file_daemon = fs::path{str_dir_app_bin} / "fim_portal_daemon";
  qreturn_if(not fs::exists(portal->m_path_file_daemon), Error("E::Daemon not found in {}", portal->m_path_file_daemon));
  // Create path to portal
  portal->m_path_file_guest = fs::path{str_dir_app_bin} / "fim_portal";
  qreturn_if(not fs::exists(portal->m_path_file_guest), Error("E::Guest not found in {}", portal->m_path_file_guest));
  // Create a portal that uses the reference file to create an unique communication key
  portal->m_process = std::make_unique<ns_subprocess::Subprocess>(portal->m_path_file_daemon);
  // Spawn process to background
  std::ignore = portal->m_process->with_piped_outputs()
    .with_args(pid_reference, mode)
    .spawn();
  return portal;
}

} // namespace ns_portal

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
