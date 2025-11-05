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
#include <ranges>
#include <set>
#include <string>
#include <expected>
#include <print>

#include "../filesystems/controller.hpp"
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
#include "interface.hpp"
#include "parser.hpp"
#include "cmd/layers.hpp"
#include "cmd/desktop.hpp"
#include "cmd/bind.hpp"
#include "cmd/recipe.hpp"

namespace ns_parser
{

namespace
{

namespace fs = std::filesystem;

} // namespace

using namespace ns_parser::ns_interface;

/**
 * @brief Executes a parsed FlatImage command
 *
 * @param config FlatImage configuration object
 * @param argc Argument counter
 * @param argv Argument vector
 * @return Value<int> The exit code on success or the respective error
 */
[[nodiscard]] inline Value<int> execute_command(ns_config::FlatimageConfig& config, int argc, char** argv)
{
  // Parse args
  CmdType variant_cmd = Pop(ns_parser::parse(argc, argv), "C::Could not parse arguments");

  auto f_bwrap_impl = [&](auto&& program, auto&& args) -> Value<ns_bwrap::bwrap_run_ret_t>
  {
    // Initialize permissions
    ns_reserved::ns_permissions::Permissions permissions(config.path_file_binary);
    // Mount filesystems
    [[maybe_unused]] auto filesystem_controller = ns_filesystems::ns_controller::Controller(config);
    // Execute specified command
    auto environment = ns_db::ns_env::get(config.path_file_binary).or_default();
    auto hash_environment = ns_db::ns_env::map(environment);
    // Check if should use bwrap native overlayfs
    std::optional<ns_bwrap::Overlay> bwrap_overlay = ( config.overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP )?
        std::make_optional(ns_bwrap::Overlay
        {
            .vec_path_dir_layer = ns_config::get_mounted_layers(config.path_dir_mount_layers)
          , .path_dir_upper = config.path_dir_upper_overlayfs
          , .path_dir_work = config.path_dir_work_overlayfs
        })
      : std::nullopt;
    // Get path to root directory
    fs::path path_dir_root = ( config.is_casefold and config.overlay_type != ns_reserved::ns_overlay::OverlayType::BWRAP )?
        config.path_dir_mount_ciopfs
      : config.path_dir_mount_overlayfs;
    // Get uid and gid from config (checks environment database or uses defaults)
    ns_config::User user = Pop(config.configure_user(), "E::Failed to configure user data");
    ns_log::debug()("User: {}", user);
    // Create bwrap command
    ns_bwrap::Bwrap bwrap = ns_bwrap::Bwrap(
        user.name
      , user.id.uid
      , user.id.gid
      , bwrap_overlay
      , path_dir_root
      , user.path_dir_home
      , user.path_file_bashrc
      , user.path_file_passwd
      , user.path_file_shell
      , program
      , args
      , environment);
    // Include root binding and custom user-defined bindings
    std::ignore = bwrap
      .with_bind_ro("/", config.path_dir_runtime_host)
      .with_binds(Pop(ns_cmd::ns_bind::db_read(config.path_file_binary), "E::Failed to configure bindings"));
    // Check if should enable GPU
    if (permissions.contains(ns_reserved::ns_permissions::Permission::GPU))
    {
      std::ignore = bwrap.with_bind_gpu(config.path_dir_upper_overlayfs, config.path_dir_runtime_host);
    }
    // Run bwrap
    return bwrap.run(permissions, config.path_dir_app_bin);
  };


  auto f_bwrap = [&]<typename T, typename U>(T&& program, U&& args) -> Value<int>
  {
    // Setup desktop integration, permissive
    ns_desktop::integrate(config).discard("W::Could not perform desktop integration");
    // Run bwrap
    ns_bwrap::bwrap_run_ret_t bwrap_run_ret = Pop(f_bwrap_impl(program, args), "E::Failed to execute bwrap");
    // Log bwrap errors
    elog_if(bwrap_run_ret.errno_nr > 0
      , std::format("Bwrap failed syscall '{}' with errno '{}'", bwrap_run_ret.syscall_nr, bwrap_run_ret.errno_nr)
    );
    // Retry with fallback if bwrap overlayfs failed
    if ( config.overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP and bwrap_run_ret.syscall_nr == SYS_mount )
    {
      ns_log::error()("Bwrap failed SYS_mount, retrying with fuse-unionfs...");
      config.overlay_type = ns_reserved::ns_overlay::OverlayType::UNIONFS;
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
    config.is_root = true;
    return f_bwrap(cmd->program, cmd->args);
  } // if
  // Configure permissions
  else if ( auto cmd = std::get_if<ns_parser::CmdPerms>(&variant_cmd) )
  {
    // Retrieve permissions
    ns_reserved::ns_permissions::Permissions permissions(config.path_file_binary);
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
      Pop(ns_db::ns_env::add(config.path_file_binary, cmd_add->variables), "E::Failed to add variables");
    }
    else if(std::get_if<CmdEnv::Clear>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_env::set(config.path_file_binary, std::vector<std::string>()), "E::Failed to clear variables");
    }
    else if(auto cmd_del = std::get_if<CmdEnv::Del>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_env::del(config.path_file_binary, cmd_del->variables), "E::Failed to delete variables");
    }
    else if(std::get_if<CmdEnv::List>(&(cmd->sub_cmd)))
    {
      std::ranges::copy(Pop(ns_db::ns_env::get(config.path_file_binary), "E::Failed to list variables")
        , std::ostream_iterator<std::string>(std::cout, "\n")
      );
    }
    else if(auto cmd_set = std::get_if<CmdEnv::Set>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_env::set(config.path_file_binary, cmd_set->variables), "E::Failed to set variables");
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
      Pop(ns_desktop::setup(config, cmd_setup->path_file_setup), "E::Failed to setup desktop integration");
    }
    else if(auto cmd_enable = std::get_if<CmdDesktop::Enable>(&(cmd->sub_cmd)))
    {
      Pop(ns_desktop::enable(config, cmd_enable->set_enable), "E::Failed to enable desktop integration");
    }
    else if(std::get_if<CmdDesktop::Clean>(&(cmd->sub_cmd)))
    {
      Pop(ns_desktop::clean(config), "E::Failed to clean desktop integration");
    }
    else if(auto cmd_dump = std::get_if<CmdDesktop::Dump>(&(cmd->sub_cmd)))
    {
      if(auto cmd_icon = std::get_if<CmdDesktop::Dump::Icon>(&(cmd_dump->sub_cmd)))
      {
        Pop(ns_desktop::dump_icon(config, cmd_icon->path_file_icon), "E::Failed to dump desktop icon");
      }
      else if(std::get_if<CmdDesktop::Dump::Entry>(&(cmd_dump->sub_cmd)))
      {
        std::println("{}", Pop(ns_desktop::dump_entry(config), "E::Failed to dump desktop entry"));
      }
      else if(std::get_if<CmdDesktop::Dump::MimeType>(&(cmd_dump->sub_cmd)))
      {
        std::println("{}", Pop(ns_desktop::dump_mimetype(config), "E::Failed to dump MIME type"));
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
      Pop(ns_layers::add(config.path_file_binary, cmd_add->path_file_src), "E::Failed to add layer");
    }
    else if(std::get_if<CmdLayer::Commit>(&(cmd->sub_cmd)))
    {
      Pop(ns_layers::commit(config.path_file_binary
        , config.path_dir_upper_overlayfs
        , config.path_dir_host_config_tmp / "layer.tmp"
        , config.path_dir_host_config_tmp / "compression.list"
        , config.layer_compression_level
      ), "E::Failed to commit layer");
    }
    else if(auto cmd_create = std::get_if<CmdLayer::Create>(&(cmd->sub_cmd)))
    {
      Pop(ns_layers::create(cmd_create->path_dir_src
        , cmd_create->path_file_target
        , config.path_dir_host_config_tmp / "compression.list"
        , config.layer_compression_level
      ), "E::Failed to create layer");
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
      Pop(ns_cmd::ns_bind::add(config.path_file_binary
        , cmd_add->type
        , cmd_add->path_src
        , cmd_add->path_dst)
      , "E::Failed to add binding");
    }
    else if(auto cmd_del = std::get_if<CmdBind::Del>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_bind::del(config.path_file_binary, cmd_del->index), "E::Failed to delete binding");
    }
    else if(std::get_if<CmdBind::List>(&(cmd->sub_cmd)))
    {
      Pop(ns_cmd::ns_bind::list(config.path_file_binary), "E::Failed to list bindings");
    }
    else
    {
      return Error("C::Invalid bind operation");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdNotify>(&variant_cmd) )
  {
    Pop(ns_reserved::ns_notify::write(config.path_file_binary, cmd->status == CmdNotifySwitch::ON), "E::Failed to write notify status");
  } // else if
  // Enable or disable casefold (useful for wine)
  else if ( auto cmd = std::get_if<ns_parser::CmdCaseFold>(&variant_cmd) )
  {
    Pop(ns_reserved::ns_casefold::write(config.path_file_binary, cmd->status == CmdCaseFoldSwitch::ON), "E::Failed to write casefold status");
  } // else if
  // Update default command on database
  else if ( auto cmd = std::get_if<ns_parser::CmdBoot>(&variant_cmd) )
  {
    if(std::get_if<CmdBoot::Clear>(&(cmd->sub_cmd)))
    {
      // Create empty boot configuration
      ns_db::ns_boot::Boot boot;
      // Write empty boot configuration
      Pop(ns_reserved::ns_boot::write(config.path_file_binary, Pop(ns_db::ns_boot::serialize(boot), "E::Failed to serialize boot configuration")), "E::Failed to clear boot configuration");
    }
    else if(auto cmd_set = std::get_if<CmdBoot::Set>(&(cmd->sub_cmd)))
    {
      // Create boot configuration
      ns_db::ns_boot::Boot boot;
      // Set values
      boot.set_program(cmd_set->program);
      boot.set_args(cmd_set->args);
      // Write boot configuration
      Pop(ns_reserved::ns_boot::write(config.path_file_binary, Pop(ns_db::ns_boot::serialize(boot), "E::Failed to serialize boot configuration")), "E::Failed to set boot configuration");
    }
    else if(std::get_if<CmdBoot::Show>(&(cmd->sub_cmd)))
    {
      // Read json data
      std::string data = Pop(ns_reserved::ns_boot::read(config.path_file_binary), "E::Failed to read boot configuration");
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
      Pop(ns_db::ns_remote::clear(config.path_file_binary), "E::Failed to clear remote URL");
    }
    else if(auto cmd_set = std::get_if<CmdRemote::Set>(&(cmd->sub_cmd)))
    {
      Pop(ns_db::ns_remote::set(config.path_file_binary, cmd_set->url), "E::Failed to set remote URL");
    }
    else if(std::get_if<CmdRemote::Show>(&(cmd->sub_cmd)))
    {
      std::println("{}", Pop(ns_db::ns_remote::get(config.path_file_binary), "E::Failed to get remote URL"));
    }
    else
    {
      return Error("C::Invalid remote sub-command");
    }
  }
  // Fetch and install recipes
  else if ( auto cmd = std::get_if<ns_parser::CmdRecipe>(&variant_cmd) )
  {
    auto f_fetch = [&config](std::string const& recipe, bool use_existing) -> Value<std::vector<std::string>>
    {
      return ns_recipe::fetch(
          config.distribution
        , Pop(ns_db::ns_remote::get(config.path_file_binary), "E::Failed to get remote URL")
        , config.path_dir_app_sbin / "wget"
        , config.path_dir_host_config
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
        Pop(ns_recipe::info(config.distribution, config.path_dir_host_config, recipe), "E::Failed to get recipe info");
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
      config.is_root = 1;
      // Install all packages and dependencies
      return ns_recipe::install(config.distribution, config.path_dir_host_config, all_recipes, f_bwrap);
    }
    else
    {
      return Error("C::Invalid recipe sub-command");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdInstance>(&variant_cmd) )
  {
    // List instances
    auto f_filename = [](auto&& e){ return e.path().filename().string(); };
    // Get instances
    auto instances = fs::directory_iterator(config.path_dir_app / "instance")
      | std::views::filter([&](auto&& e){ return fs::exists(fs::path{"/proc"} / f_filename(e)); })
      | std::views::filter([&](auto&& e){
          auto pid_result = Catch(std::stoi(f_filename(e)));
          return pid_result && pid_result.value() != getpid();
        })
      | std::views::transform([](auto&& e){ return e.path(); })
      | std::ranges::to<std::vector<fs::path>>();
    // Sort by pid (filename is a directory named as the pid of that instance)
    std::ranges::sort(instances, {}, [](auto&& e){
      auto pid_result = Catch(std::stoi(e.filename().string()));
      return pid_result ? pid_result.value() : 0;
    });
    if(auto cmd_exec = std::get_if<CmdInstance::Exec>(&(cmd->sub_cmd)))
    {
      qreturn_if(instances.size() == 0, Error("C::No instances are running"));
      qreturn_if(cmd_exec->id < 0 or static_cast<size_t>(cmd_exec->id) >= instances.size()
        , Error("C::Instance index out of bounds")
      );
      return ns_subprocess::Subprocess(config.path_dir_app_bin / "fim_portal")
        .with_args("--connect", instances.at(cmd_exec->id))
        .with_args(cmd_exec->args)
        .spawn()
        .wait()
        .value_or(125);
    }
    else if(std::get_if<CmdInstance::List>(&(cmd->sub_cmd)))
    {
      for(uint32_t i = 0; fs::path const& instance : instances)
      {
        std::println("{}:{}", i++, instance.filename().string());
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
      Pop(ns_reserved::ns_overlay::write(config.path_file_binary, cmd_set->overlay), "E::Failed to set overlay type");
    }
    else if(std::get_if<CmdOverlay::Show>(&(cmd->sub_cmd)))
    {
      std::println("{}", std::string{config.overlay_type});
    }
    else
    {
      return Error("C::Invalid operation for fim-overlay");
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
    std::string data = Pop(ns_reserved::ns_boot::read(config.path_file_binary), "E::Failed to read boot configuration");
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
