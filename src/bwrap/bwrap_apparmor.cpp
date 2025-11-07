/**
 * @file bwrap_apparmor.cpp
 * @author Ruan Formigoni
 * @brief Configures apparmor to allow the execution of [bubblewrap](https://github.com/containers/bubblewrap)
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#include <filesystem>

#include "../std/expected.hpp"
#include "../lib/log.hpp"
#include "../lib/subprocess.hpp"
#include "../lib/env.hpp"
#include "../macro.hpp"

constexpr std::string_view const profile_bwrap =
R"(abi <abi/4.0>,
include <tunables/global>
profile bwrap /opt/flatimage/bwrap flags=(unconfined) {
  userns,
}
)";

namespace fs = std::filesystem;

int main(int argc, char const* argv[])
{
  auto __expected_fn = [](auto&&){ return EXIT_FAILURE; };
  // Check arguments
  ereturn_if(argc != 3, "Incorrect # of arguments for bwrap-apparmor", EXIT_FAILURE);
  // Set log file location
  fs::path path_file_log = std::string{argv[1]} + ".bwrap-apparmor.log";
  ns_log::set_sink_file(path_file_log);
  // Find apparmor_parser
  fs::path path_file_apparmor_parser = Pop(ns_env::search_path("apparmor_parser"));
  // Define paths
  fs::path path_file_bwrap_src{argv[2]};
  fs::path path_dir_bwrap{"/opt/flatimage"};
  fs::path path_file_bwrap_dst{path_dir_bwrap / "bwrap"};
  fs::path path_file_profile{"/etc/apparmor.d/flatimage"};
  // Try to create /opt/bwrap directory
  Try(fs::create_directories(path_dir_bwrap));
  // Try copy bwrap to /opt/bwrap
  Try(fs::copy_file(path_file_bwrap_src, path_file_bwrap_dst, fs::copy_options::overwrite_existing));
  // Try to set permissions for bwrap binary ( chmod 755 )
  using fs::perms;
  auto perms = perms::owner_all | perms::group_read | perms::group_exec | perms::others_read | perms::others_exec;
  Catch(fs::permissions(path_file_bwrap_dst, perms, fs::perm_options::replace))
    .discard("C::Failed to set permissions to '{}'", path_file_bwrap_dst);
  // Try to create profile
  std::ofstream file_profile(path_file_profile);
  ereturn_if(not file_profile.is_open(), "Could not open profile file", EXIT_FAILURE);
  file_profile << profile_bwrap;
  file_profile.close();
  // Reload profile
  return Pop(ns_subprocess::Subprocess(path_file_apparmor_parser)
    .with_args("-r", path_file_profile)
    .with_log_stdio()
    .wait()
  );
} // main
