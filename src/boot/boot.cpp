/**
 * @file boot.cpp
 * @author Ruan Formigoni
 * @brief The main flatimage program
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#include <elf.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>

#include "../std/expected.hpp"
#include "../lib/linux.hpp"
#include "../lib/env.hpp"
#include "../lib/log.hpp"
#include "../parser/executor.hpp"
#include "../portal/portal.hpp"
#include "../config.hpp"
#include "relocate.hpp"

// Section for offset to filesystems
extern "C"
{
  alignas(4)
  __attribute__((section(".fim_reserved_offset"), used))
  uint32_t FIM_RESERVED_OFFSET = 0;
}

// Unix environment variables
extern char** environ;

/**
 * @brief Boots the main flatimage program and the portal process
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return The return code of the process or an internal flatimage error '125'
 */
[[nodiscard]] Expected<int> boot(int argc, char** argv)
{
  // Create configuration object
  std::shared_ptr<ns_config::FlatimageConfig> config = Expect(ns_config::config());
  qreturn_if(config == nullptr, Unexpected("E::Failed to initialize configuration"));
  // Set log file, permissive
  ns_log::set_sink_file(config->path_dir_mount.string() + ".boot.log");
  // Start host portal, permissive
  [[maybe_unused]] auto portal = ns_portal::create(getpid(), "host")
    .forward("E::Could not start portal daemon");
  // Execute flatimage command if exists
  return Expect(ns_parser::execute_command(*config, argc, argv));
}

/**
 * @brief Set the logger level
 * 
 * @param argc Argument count
 * @param argv Argument vector
 */
void set_logger_level(int argc, char** argv)
{
  // Force debug mode
  if(ns_env::exists("FIM_DEBUG", "1"))
  {
    ns_log::set_level(ns_log::Level::DEBUG);
    return;
  }
  // Default critical only mode when no commands are passed
  // Default critical only mode when commands are fim-root or fim-exec
  if(argc < 2
  or std::string_view{argv[1]} == "fim-exec"
  or std::string_view{argv[1]} == "fim-root"
  or not std::string_view{argv[1]}.starts_with("fim-"))
  {
    ns_log::set_level(ns_log::Level::CRITICAL);
    return;
  }
  // Otherwise, info mode is the default
  ns_log::set_level(ns_log::Level::INFO);
}

int main(int argc, char** argv)
{
  auto __expected_fn = [](auto&&){ return 125; };
  // Configure logger
  set_logger_level(argc, argv);
  // Make variables available in environment
  ns_env::set("FIM_VERSION", FIM_VERSION, ns_env::Replace::Y);
  ns_env::set("FIM_COMMIT", FIM_COMMIT, ns_env::Replace::Y);
  ns_env::set("FIM_DIST", FIM_DIST, ns_env::Replace::Y);
  ns_env::set("FIM_TIMESTAMP", FIM_TIMESTAMP, ns_env::Replace::Y);
  // Check if linux has the fuse module loaded
  ns_linux::module_check("fuse").discard("W::'fuse' module might not be loaded");
  // If it is outside /tmp, move the binary
  Expect(ns_relocate::relocate(argv, FIM_RESERVED_OFFSET), "C::Failure to relocate binary");
  // Launch flatimage
  return Expect(boot(argc, argv), "C::Program exited with error");
}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
