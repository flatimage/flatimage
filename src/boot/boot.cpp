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

#include "../lib/linux.hpp"
#include "../lib/env.hpp"
#include "../lib/log.hpp"
#include "../parser/parser.hpp"
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
  ns_config::FlatimageConfig config = ns_config::config();
  // Set log file, permissive
  if(auto ret = ns_log::set_sink_file(config.path_dir_mount.string() + ".boot.log"); not ret)
  {
    std::cerr << "Could not setup logger sink: " << ret.error() << '\n';
  }
  // Start host portal
  ns_portal::Portal portal = ns_portal::Portal(getpid(), "host");
  // Parse flatimage command if exists
  return Expect(ns_parser::parse_cmds(config, argc, argv));
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
  // Configure logger
  set_logger_level(argc, argv);

  // Check for version queries
  if(argc > 1)
  {
    std::string_view arg{argv[1]};
    // Print version and exit
    if ( arg == "fim-version" )
    {
      std::println(FIM_VERSION);
      return EXIT_SUCCESS;
    } // if
    
    // Print version-full and exit
    if ( arg == "fim-version-full" )
    {
      ns_db::Db db;
      db("VERSION") = FIM_VERSION;
      db("COMMIT") = FIM_COMMIT;
      db("DISTRIBUTION") = FIM_DIST;
      db("TIMESTAMP") = FIM_TIMESTAMP;
      std::println("{}", db.dump());
      return EXIT_SUCCESS;
    } // if
  }
  
  // Make variables available in environment
  ns_env::set("FIM_VERSION", FIM_VERSION, ns_env::Replace::Y);
  ns_env::set("FIM_COMMIT", FIM_COMMIT, ns_env::Replace::Y);
  ns_env::set("FIM_DIST", FIM_DIST, ns_env::Replace::Y);
  ns_env::set("FIM_TIMESTAMP", FIM_TIMESTAMP, ns_env::Replace::Y);

  // Check if linux has the fuse module loaded
  if(auto ret = ns_linux::module_check("fuse"); not ret)
  {
    ns_log::error()("'fuse' module might not be loaded: {}", ret.error());
  } // if

  // If it is outside /tmp, move the binary
  if (auto result = ns_relocate::relocate(argv, FIM_RESERVED_OFFSET); not result)
  {
    ns_log::critical()("Failure to relocate binary: {}", result.error());
    return 125;
  } // if

  // Launch flatimage
  if (auto code = boot(argc, argv))
  {
    return code.value();
  } // if
  else
  {
    ns_log::critical()("Program exited with error: {}", code.error());
  } // else

  return 125;
}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
