/**
 * @file executor.hpp
 * @author Ruan Formigoni
 * @brief Executes parsed FlatImage commands
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iterator>
#include <set>
#include <string>
#include <expected>
#include <print>

#include "../filesystems/controller.hpp"
#include "../filesystems/utils.hpp"
#include "../db/env.hpp"
#include "../db/remote.hpp"
#include "../db/boot.hpp"
#include "../macro.hpp"
#include "../reserved/overlay.hpp"
#include "../reserved/notify.hpp"
#include "../reserved/casefold.hpp"
#include "../reserved/boot.hpp"
#include "../bwrap/bwrap.hpp"
#include "../config.hpp"
#include "../lib/subprocess.hpp"
#include "../portal/portal.hpp"
#include "interface.hpp"
#include "parser.hpp"
#include "cmd/layers.hpp"
#include "cmd/desktop.hpp"
#include "cmd/bind.hpp"
#include "cmd/recipe.hpp"
#include "cmd/unshare.hpp"

namespace ns_parser
{

namespace
{

namespace fs = std::filesystem;

} // namespace

namespace ns_dispatcher = ns_db::ns_portal::ns_dispatcher;
namespace ns_daemon = ns_db::ns_portal::ns_daemon;

using namespace ns_parser::ns_interface;

/**
 * @brief Executes a parsed FlatImage command
 *
 * @param config FlatImage configuration object
 * @param argc Argument counter
 * @param argv Argument vector
 * @return Value<int> The exit code on success or the respective error
 */
[[nodiscard]] inline Value<int> execute_command(ns_config::FlatImage& fim, int argc, char** argv)
{
  auto& fuse = fim.config.fuse;

  // Parse args
  CmdType variant_cmd = Pop(ns_parser::parse(argc, argv), "C::Could not parse arguments");

  auto f_bwrap_impl = [&](auto&& program, auto&& args) -> Value<ns_bwrap::bwrap_run_ret_t>
  {
    // Check if the data directory is busy
    Pop(ns_filesystems::ns_utils::wait_busy(fim.path.dir.host_data, std::chrono::seconds(60)));
    // Mount filesystems
    [[maybe_unused]] auto filesystem_controller =
      ns_filesystems::ns_controller::Controller(fim.logs.filesystems
        , fuse
      );
    // Execute specified command
    auto environment = ns_db::ns_env::get(fim.path.bin.self).or_default();
    // Get path to root directory
    fs::path path_dir_root = ( fim.flags.is_casefold and fuse.overlay_type != ns_reserved::ns_overlay::OverlayType::BWRAP )?
        fuse.path_dir_ciopfs
      : fuse.path_dir_mount;
    logger("D::Bwrap root: {}", path_dir_root);
    // Bubblewrap user data
    ns_bwrap::ns_proxy::User user = Pop(fim.configure_bwrap());
    logger("D::User: {}", std::string{user.data});
    // Bwrap wrapper
    ns_bwrap::Bwrap bwrap = ns_bwrap::Bwrap(fim.logs.bwrap
      , user
      , path_dir_root
      , program
      , args
      , environment
    );
    // Check for an overlapping data directory
    // Optionally user bwrap overlays
    if(fuse.overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP)
    {
      bwrap.set_overlay(ns_bwrap::ns_proxy::Overlay
      {
          .vec_path_dir_layer = ns_filesystems::ns_utils::get_mounted_layers(fuse.path_dir_layers)
        , .path_dir_upper = fuse.path_dir_upper
        , .path_dir_work = fuse.path_dir_work
      });
    }
    // Include root binding and custom user-defined bindings
    std::ignore = bwrap
      .with_bind_ro("/", fim.path.dir.runtime_host)
      .with_binds(Pop(ns_cmd::ns_bind::db_read(fim.path.bin.self), "E::Failed to configure bindings"));
    // Retrieve permissions
    ns_reserved::ns_permissions::Permissions permissions(fim.path.bin.self);
    // Retrieve unshare options
    ns_reserved::ns_unshare::Unshares unshares(fim.path.bin.self);
    // Check if should enable GPU
    if (permissions.contains(ns_reserved::ns_permissions::Permission::GPU))
    {
      std::ignore = bwrap.with_bind_gpu(fuse.path_dir_upper, fim.path.dir.runtime_host);
    }
    // Build the dispatcher object pointing it to the fifo of the host daemon
    ns_dispatcher::Dispatcher dispatcher(fim.pid
      , ns_daemon::Mode::HOST
      , fim.path.dir.app
      , fim.logs.dispatcher.path_dir_log
    );
    // Start host portal, permissive
    [[maybe_unused]] auto portal = ns_portal::spawn(fim.config.daemon.host
      , fim.logs.daemon_host
    ).forward("E::Could not start portal daemon");
    // Run the portal program with the guest dispatcher configuration
    // Run bwrap
    return bwrap.run(permissions
      , unshares
      , fim.path.bin.portal_daemon
      , dispatcher
      , fim.config.daemon.guest
      , fim.logs.daemon_guest
    );
  };


  auto f_bwrap = [&]<typename T, typename U>(T&& program, U&& args) -> Value<int>
  {
    // Setup desktop integration, permissive
    ns_desktop::integrate(fim).discard("W::Could not perform desktop integration");
    // Run bwrap
    ns_bwrap::bwrap_run_ret_t bwrap_run_ret = Pop(f_bwrap_impl(program, args), "E::Failed to execute bwrap");
    // Log bwrap errors
    log_if(bwrap_run_ret.errno_nr > 0, "E::Bwrap failed syscall '{}' with errno '{}'", bwrap_run_ret.syscall_nr, bwrap_run_ret.errno_nr);
    // Retry with fallback if bwrap overlayfs failed
    if ( fuse.overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP and bwrap_run_ret.syscall_nr == SYS_mount )
    {
      logger("E::Bwrap failed SYS_mount, retrying with fuse-unionfs...");
      fuse.overlay_type = ns_reserved::ns_overlay::OverlayType::UNIONFS;
      bwrap_run_ret = Pop(f_bwrap_impl(program, args), "E::Failed to execute bwrap");
    } // if
    return bwrap_run_ret.code;
  };

  // Execute a command as a regular user
  if ( auto cmd = std::get_if<ns_parser::CmdExec>(&variant_cmd) )
  {
    return f_bwrap(cmd->program, cmd->args);
  } // if
  // Execute a command as root
  else if ( auto cmd = std::get_if<ns_parser::CmdRoot>(&variant_cmd) )
  {
    fim.flags.is_root = true;
    return f_bwrap(cmd->program, cmd->args);
  } // if
  // Configure permissions
  else if ( auto cmd = std::get_if<ns_parser::CmdPerms>(&variant_cmd) )
  {
    // Retrieve permissions
    ns_reserved::ns_permissions::Permissions permissions(fim.path.bin.self);
    // Determine permission operation
    if(auto cmd_add = std::get_if<CmdPerms::Add>(&(cmd->sub_cmd)))
    {
      Pop(permissions.add(cmd_add->permissions), "E::Failed to add permissions");
    }
    else if(std::get_if<CmdPerms::Clear>(&(cmd->sub_cmd)))
    {
      Pop(permissions.set_all(false), "E::Failed to clear permissions");
    }
    else if(auto cmd_del = std::get_if<CmdPerms::Del>(&(cmd->sub_cmd)))
    {
      Pop(permissions.del(cmd_del->permissions), "E::Failed to delete permissions");
    }
    else if(std::get_if<CmdPerms::List>(&(cmd->sub_cmd)))
    {
      std::ranges::copy(Pop(permissions.to_strings(), "E::Failed to stringfy permissions")
        , std::ostream_iterator<std::string>(std::cout, "\n")
      );
    }
    else if(auto cmd_set = std::get_if<CmdPerms::Set>(&(cmd->sub_cmd)))
    {
      Pop(permissions.set(cmd_set->permissions), "E::Failed to set permissions");
    }
    else
    {
      return Error("C::Invalid permissions sub-command");
    }
  }
  // Configure environment
  else if ( auto cmd = std::get_if<ns_parser::CmdEnv>(&variant_cmd) )
  {
    if(auto cmd_add = std::get_if<CmdEnv::Add>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_env::add(fim.path.bin.self, cmd_add->variables), "E::Failed to add variables");
    }
    else if(std::get_if<CmdEnv::Clear>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_env::set(fim.path.bin.self, std::vector<std::string>()), "E::Failed to clear variables");
    }
    else if(auto cmd_del = std::get_if<CmdEnv::Del>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_env::del(fim.path.bin.self, cmd_del->variables), "E::Failed to delete variables");
    }
    else if(std::get_if<CmdEnv::List>(&(cmd->sub_cmd)))
    {
      std::ranges::copy(Pop(ns_db::ns_env::get(fim.path.bin.self), "E::Failed to list variables")
        , std::ostream_iterator<std::string>(std::cout, "\n")
      );
    }
    else if(auto cmd_set = std::get_if<CmdEnv::Set>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_env::set(fim.path.bin.self, cmd_set->variables), "E::Failed to set variables");
    }
    else
    {
      return Error("C::Invalid environment sub-command");
    }
  }
  // Configure desktop integration
  else if (auto cmd = std::get_if<CmdDesktop>(&variant_cmd))
  {
    if(auto cmd_setup = std::get_if<CmdDesktop::Setup>(&(cmd->sub_cmd)))
    {
      Pop(ns_desktop::setup(fim, cmd_setup->path_file_setup), "E::Failed to setup desktop integration");
    }
    else if(auto cmd_enable = std::get_if<CmdDesktop::Enable>(&(cmd->sub_cmd)))
    {
      Pop(ns_desktop::enable(fim, cmd_enable->set_enable), "E::Failed to enable desktop integration");
    }
    else if(std::get_if<CmdDesktop::Clean>(&(cmd->sub_cmd)))
    {
      Pop(ns_desktop::clean(fim), "E::Failed to clean desktop integration");
    }
    else if(auto cmd_dump = std::get_if<CmdDesktop::Dump>(&(cmd->sub_cmd)))
    {
      if(auto cmd_icon = std::get_if<CmdDesktop::Dump::Icon>(&(cmd_dump->sub_cmd)))
      {
        Pop(ns_desktop::dump_icon(fim, cmd_icon->path_file_icon), "E::Failed to dump desktop icon");
      }
      else if(std::get_if<CmdDesktop::Dump::Entry>(&(cmd_dump->sub_cmd)))
      {
        std::println("{}", Pop(ns_desktop::dump_entry(fim), "E::Failed to dump desktop entry"));
      }
      else if(std::get_if<CmdDesktop::Dump::MimeType>(&(cmd_dump->sub_cmd)))
      {
        std::println("{}", Pop(ns_desktop::dump_mimetype(fim), "E::Failed to dump MIME type"));
      }
      else
      {
        return Error("C::Invalid dump sub-command");
      }
    }
    else
    {
      return Error("C::Invalid desktop sub-command");
    }
  }
  // Manager layers
  else if ( auto cmd = std::get_if<ns_parser::CmdLayer>(&variant_cmd) )
  {
    if(auto cmd_add = std::get_if<CmdLayer::Add>(&(cmd->sub_cmd)))
    {
      Pop(ns_layers::add(fim.path.bin.self, cmd_add->path_file_src), "E::Failed to add layer");
    }
    else if(auto cmd_commit = std::get_if<CmdLayer::Commit>(&(cmd->sub_cmd)))
    {
      // Determine commit mode and optional file path
      ns_layers::CommitMode mode;
      std::optional<fs::path> path_file_dst;
      if(std::get_if<CmdLayer::Commit::Binary>(&(cmd_commit->sub_cmd)))
      {
        mode = ns_layers::CommitMode::BINARY;
      }
      else if(std::get_if<CmdLayer::Commit::Layer>(&(cmd_commit->sub_cmd)))
      {
        mode = ns_layers::CommitMode::LAYER;
        path_file_dst = fim.path.dir.host_data_layers;
      }
      else if(auto cmd_file = std::get_if<CmdLayer::Commit::File>(&(cmd_commit->sub_cmd)))
      {
        mode = ns_layers::CommitMode::FILE;
        path_file_dst = cmd_file->path_file_dst;
      }
      else
      {
        return Error("E::Invalid commit sub-command");
      }
      // Call commit with the mode
      Pop(ns_layers::commit(fim.path.bin.self
        , fuse.path_dir_upper
        , fim.path.dir.host_data_tmp / "layer.tmp"
        , fim.path.dir.host_data_tmp / "compression.list"
        , fuse.compression_level
        , mode
        , path_file_dst
      ), "E::Failed to commit layer");
    }
    else if(auto cmd_create = std::get_if<CmdLayer::Create>(&(cmd->sub_cmd)))
    {
      Pop(ns_layers::create(cmd_create->path_dir_src
        , cmd_create->path_file_target
        , fim.path.dir.host_data_tmp / "compression.list"
        , fuse.compression_level
      ), "E::Failed to create layer");
      logger("I::Filesystem created without errors");
    }
    else if(std::get_if<CmdLayer::List>(&(cmd->sub_cmd)))
    {
      ns_layers::list(fuse.layers);
    }
    else
    {
      return Error("C::Invalid layer operation");
    }
  }
  // Bind a device or file to the flatimage
  else if ( auto cmd = std::get_if<ns_parser::CmdBind>(&variant_cmd) )
  {
    if(auto cmd_add = std::get_if<CmdBind::Add>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_bind::add(fim.path.bin.self
        , cmd_add->type
        , cmd_add->path_src
        , cmd_add->path_dst)
      , "E::Failed to add binding");
    }
    else if(auto cmd_del = std::get_if<CmdBind::Del>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_bind::del(fim.path.bin.self, cmd_del->index), "E::Failed to delete binding");
    }
    else if(std::get_if<CmdBind::List>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_bind::list(fim.path.bin.self), "E::Failed to list bindings");
    }
    else
    {
      return Error("C::Invalid bind operation");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdNotify>(&variant_cmd) )
  {
    Pop(ns_reserved::ns_notify::write(fim.path.bin.self, cmd->status == CmdNotifySwitch::ON), "E::Failed to write notify status");
  } // else if
  // Enable or disable casefold (useful for wine)
  else if ( auto cmd = std::get_if<ns_parser::CmdCaseFold>(&variant_cmd) )
  {
    Pop(ns_reserved::ns_casefold::write(fim.path.bin.self, cmd->status == CmdCaseFoldSwitch::ON), "E::Failed to write casefold status");
  } // else if
  // Update default command on database
  else if ( auto cmd = std::get_if<ns_parser::CmdBoot>(&variant_cmd) )
  {
    if(std::get_if<CmdBoot::Clear>(&(cmd->sub_cmd)))
    {
      // Create empty boot configuration
      ns_db::ns_boot::Boot boot;
      // Write empty boot configuration
      Pop(ns_reserved::ns_boot::write(fim.path.bin.self, Pop(ns_db::ns_boot::serialize(boot), "E::Failed to serialize boot configuration")), "E::Failed to clear boot configuration");
    }
    else if(auto cmd_set = std::get_if<CmdBoot::Set>(&(cmd->sub_cmd)))
    {
      // Create boot configuration
      ns_db::ns_boot::Boot boot;
      // Set values
      boot.set_program(cmd_set->program);
      boot.set_args(cmd_set->args);
      // Write boot configuration
      Pop(ns_reserved::ns_boot::write(fim.path.bin.self, Pop(ns_db::ns_boot::serialize(boot), "E::Failed to serialize boot configuration")), "E::Failed to set boot configuration");
    }
    else if(std::get_if<CmdBoot::Show>(&(cmd->sub_cmd)))
    {
      // Read json data
      std::string data = Pop(ns_reserved::ns_boot::read(fim.path.bin.self), "E::Failed to read boot configuration");
      // Deserialize boot configuration
      ns_db::ns_boot::Boot boot = ns_db::ns_boot::deserialize(data).or_default();
      // If no program was set, default to bash
      if (boot.get_program().empty())
      {
        boot.set_program("bash");
      }
      // Serialize and print
      std::cout << Pop(ns_db::ns_boot::serialize(boot), "E::Failed to serialize boot configuration") << '\n';
    }
    else
    {
      return Error("C::Invalid boot sub-command");
    }
  }
  // Configure remote URL
  else if ( auto cmd = std::get_if<ns_parser::CmdRemote>(&variant_cmd) )
  {
    if(std::get_if<CmdRemote::Clear>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_remote::clear(fim.path.bin.self), "E::Failed to clear remote URL");
    }
    else if(auto cmd_set = std::get_if<CmdRemote::Set>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_remote::set(fim.path.bin.self, cmd_set->url), "E::Failed to set remote URL");
    }
    else if(std::get_if<CmdRemote::Show>(&(cmd->sub_cmd)))
    {
      std::println("{}", Pop(ns_db::ns_remote::get(fim.path.bin.self), "E::Failed to get remote URL"));
    }
    else
    {
      return Error("C::Invalid remote sub-command");
    }
  }
  // Fetch and install recipes
  else if ( auto cmd = std::get_if<ns_parser::CmdRecipe>(&variant_cmd) )
  {
    auto f_fetch = [&fim](std::string const& recipe, bool use_existing) -> Value<std::vector<std::string>>
    {
      return ns_recipe::fetch(
          fim.distribution
        , Pop(ns_db::ns_remote::get(fim.path.bin.self), "E::Failed to get remote URL")
        , fim.path.dir.app_sbin / "wget"
        , fim.path.dir.host_data
        , recipe
        , use_existing
      );
    };

    if(auto cmd_fetch = std::get_if<CmdRecipe::Fetch>(&(cmd->sub_cmd)))
    {
      // Fetch recipes with dependencies, always download when fetch command is called directly
      for(auto const& recipe : cmd_fetch->recipes)
      {
        Pop(f_fetch(recipe, false), "E::Failed to fetch recipe");
      }
    }
    else if(auto cmd_info = std::get_if<CmdRecipe::Info>(&(cmd->sub_cmd)))
    {
      for(auto const& recipe : cmd_info->recipes)
      {
        Pop(ns_recipe::info(fim.distribution, fim.path.dir.host_data, recipe), "E::Failed to get recipe info");
      }
    }
    else if(auto cmd_install = std::get_if<CmdRecipe::Install>(&(cmd->sub_cmd)))
    {
      std::vector<std::string> all_recipes;
      // Fetch all recipes and dependencies, overwrite existing
      for(auto const& recipe : cmd_install->recipes)
      {
        std::vector<std::string> sub_recipes = Pop(f_fetch(recipe, true), "E::Failed to fetch recipe");
        std::ranges::copy(sub_recipes, std::back_inserter(all_recipes));
      }
      fim.flags.is_root = 1;
      // Install all packages and dependencies
      return ns_recipe::install(fim, fim.distribution, fim.path.dir.host_data, all_recipes, f_bwrap);
    }
    else
    {
      return Error("C::Invalid recipe sub-command");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdInstance>(&variant_cmd) )
  {
    using ns_filesystems::ns_utils::Instance;
    // Get instances
    auto instances = ns_filesystems::ns_utils::get_instances(fim.path.dir.app / "instance");
    // Process the exec command
    if(auto cmd_exec = std::get_if<CmdInstance::Exec>(&(cmd->sub_cmd)))
    {
      return_if(instances.size() == 0, Error("C::No instances are running"));
      return_if(cmd_exec->id < 0 or static_cast<size_t>(cmd_exec->id) >= instances.size()
        , Error("C::Instance index out of bounds")
      );
      // Get instance
      Instance instance = instances.at(cmd_exec->id);
      // Build the dispatcher object pointing it to the fifo of the guest daemon
      ns_dispatcher::Dispatcher dispatcher(instance.pid
        , ns_daemon::Mode::GUEST
        , fim.path.dir.app
        , fim.logs.dispatcher
      );
      // Run the portal program with the guest dispatcher configuration
      return Pop(ns_subprocess::Subprocess(fim.path.dir.app_bin / "fim_portal")
        .with_var("FIM_DISPATCHER_CFG", Pop(ns_dispatcher::serialize(dispatcher)))
        .with_args(cmd_exec->args)
        .spawn()->wait());
    }
    else if(std::get_if<CmdInstance::List>(&(cmd->sub_cmd)))
    {
      for(uint32_t i = 0; Instance const& instance : instances)
      {
        std::println("{}:{}", i++, instance.path.filename().string());
      }
    }
    else
    {
      return Error("C::Invalid instance operation");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdOverlay>(&variant_cmd) )
  {
    if(auto cmd_set = std::get_if<CmdOverlay::Set>(&(cmd->sub_cmd)))
    {
      Pop(ns_reserved::ns_overlay::write(fim.path.bin.self, cmd_set->overlay), "E::Failed to set overlay type");
    }
    else if(std::get_if<CmdOverlay::Show>(&(cmd->sub_cmd)))
    {
      std::println("{}", std::string{fuse.overlay_type});
    }
    else
    {
      return Error("C::Invalid operation for fim-overlay");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdUnshare>(&variant_cmd) )
  {
    if(auto cmd_set = std::get_if<CmdUnshare::Set>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_unshare::set(fim.path.bin.self, cmd_set->unshares), "E::Failed to set unshare options");
    }
    else if(auto cmd_add = std::get_if<CmdUnshare::Add>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_unshare::add(fim.path.bin.self, cmd_add->unshares), "E::Failed to add unshare options");
    }
    else if(auto cmd_del = std::get_if<CmdUnshare::Del>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_unshare::del(fim.path.bin.self, cmd_del->unshares), "E::Failed to delete unshare options");
    }
    else if(std::get_if<CmdUnshare::Clear>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_unshare::clear(fim.path.bin.self), "E::Failed to clear unshare options");
    }
    else if(std::get_if<CmdUnshare::List>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_unshare::list(fim.path.bin.self), "E::Failed to list unshare options");
    }
    else
    {
      return Error("C::Invalid operation for fim-unshare");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdVersion>(&variant_cmd) )
  {
    if(auto cmd_short = std::get_if<CmdVersion::Short>(&(cmd->sub_cmd)))
    {
      std::println("{}", cmd_short->dump());
    }
    else if(auto cmd_full = std::get_if<CmdVersion::Full>(&(cmd->sub_cmd)))
    {
      std::println("{}", Pop(cmd_full->dump(), "E::Failed to dump full version info"));
    }
    else if(auto cmd_deps = std::get_if<CmdVersion::Deps>(&(cmd->sub_cmd)))
    {
      std::println("{}", Pop(cmd_deps->dump(), "E::Failed to dump dependencies info"));
    }
    else
    {
      return Error("C::Invalid operation for fim-version");
    }
  }
  // Update default command on database
  else if ( std::get_if<ns_parser::CmdNone>(&variant_cmd) )
  {
    std::string data = Pop(ns_reserved::ns_boot::read(fim.path.bin.self), "E::Failed to read boot configuration");
    // De-serialize boot command
    auto boot = ns_db::ns_boot::deserialize(data).value_or(ns_db::ns_boot::Boot());
    // Retrive default program
    fs::path program = boot.get_program().empty() ?
        "bash"
      : ns_env::expand(boot.get_program()).value_or(boot.get_program());
    // Retrive default arguments
    std::vector<std::string> args = boot.get_args();
    // Append arguments from argv
    std::copy(argv+1, argv+argc, std::back_inserter(args));
    // Run Bwrap
    return f_bwrap(program, args);
  } // else if
  else if ( std::get_if<ns_parser::CmdExit>(&variant_cmd) )
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return Error("C::Unknown command");
  }

  return EXIT_SUCCESS;
}

} // namespace ns_parser

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
