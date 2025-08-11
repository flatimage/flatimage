///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : boot
///

#include <elf.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>

#include "../cpp/lib/linux.hpp"
#include "../cpp/lib/env.hpp"
#include "../cpp/lib/log.hpp"

#include "config/config.hpp"
#include "parser/parser.hpp"
#include "portal.hpp"
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

// boot() {{{
std::expected<int, std::string> boot(int argc, char** argv, ns_config::FlatimageConfig config)
{
  // Set log file
  ns_log::set_sink_file(config.path_dir_mount.string() + ".boot.log");
  // Start host portal
  ns_portal::Portal portal = ns_portal::Portal(getpid(), "host");
  // Parse flatimage command if exists
  return expect(ns_parser::parse_cmds(config, argc, argv));
} // boot() }}}

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
  if(argc < 2 or std::string_view{argv[1]} == "fim-exec" or std::string_view{argv[1]} == "fim-root")
  {
    ns_log::set_level(ns_log::Level::CRITICAL);
    return;
  }
  // Otherwise, info mode is the default
    ns_log::set_level(ns_log::Level::INFO);
}

// main() {{{
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
  if ( auto code = boot(argc, argv, ns_config::FlatimageConfig(ns_config::config())) )
  {
    return code.value();
  } // if
  else
  {
    ns_log::critical()("Program exited with error: {}", code.error());
  } // else

  return 125;
} // main() }}}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
