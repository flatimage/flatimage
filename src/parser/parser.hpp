/**
 * @file parser.hpp
 * @author Ruan Formigoni
 * @brief Parses FlatImage commands
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <ranges>
#include <set>
#include <string>
#include <expected>
#include <print>


#include "../filesystems/controller.hpp"
#include "../db/env.hpp"
#include "../lib/match.hpp"
#include "../macro.hpp"
#include "../reserved/overlay.hpp"
#include "../reserved/notify.hpp"
#include "../reserved/casefold.hpp"
#include "../reserved/boot.hpp"
#include "../bwrap/bwrap.hpp"
#include "../config.hpp"
#include "interface.hpp"
#include "cmd/layers.hpp"
#include "cmd/desktop.hpp"
#include "cmd/bind.hpp"
#include "cmd/help.hpp"

namespace ns_parser
{

namespace
{

namespace fs = std::filesystem;

} // namespace

using namespace ns_parser::ns_interface;


/**
 * @brief Parses FlatImage commands
 * 
 * @param argc Argument counter
 * @param argv Argument vector
 * @return Expected<CmdType> The parsed command or the respective error
 */
[[nodiscard]] inline Expected<CmdType> parse(int argc , char** argv)
{
  if ( argc < 2 or not std::string_view{argv[1]}.starts_with("fim-"))
  {
    return CmdNone{};
  }

  class VecArgs
  {
    private:
      std::vector<std::string> m_data;
    public:
      VecArgs(char** begin, char** end)
      {
        if(begin != end)
        {
          m_data = std::vector<std::string>(begin,end);
        }
      }
      Expected<std::string> pop_front(std::string const& msg)
      {
        if(m_data.empty()) { return Unexpected(msg); }
        std::string item = m_data.front();
        m_data.erase(m_data.begin());
        return item;
      }
      std::vector<std::string> const& data()
      {
        return m_data;
      }
      size_t size()
      {
        return m_data.size();
      }
      bool empty()
      {
        return m_data.empty();
      }
      void clear()
      {
        m_data.clear();
      }
  };
  
  VecArgs args(argv+1, argv+argc);

  return Expect(ns_match::match(Expect(args.pop_front("Missing fim- command")),
    ns_match::equal("fim-exec") >>= [&] -> Expected<CmdType>
    {
      return CmdType(CmdExec(Expect(args.pop_front("Incorrect number of arguments for fim-exec"))
        ,  args.data()
      ));
    },
    ns_match::equal("fim-root") >>= [&] -> Expected<CmdType>
    {
      return CmdType(CmdRoot(Expect(args.pop_front("Incorrect number of arguments for fim-root"))
        , args.data())
      );
    },
    // Configure permissions for the container
    ns_match::equal("fim-perms") >>= [&] -> Expected<CmdType>
    {
      // Get op
      CmdPermsOp op = Expect(
        CmdPermsOp::from_string(Expect(args.pop_front("Missing op for fim-perms (add,del,list,set,clear)")))
      );
      auto f_process_permissions = [&] -> Expected<std::set<CmdPerms::Permission>>
      {
        std::set<CmdPerms::Permission> permissions;
        for(auto arg : ns_vector::from_string(Expect(args.pop_front("No arguments for '{}' command"_fmt(op))), ','))
        {
          permissions.insert(Expect(CmdPerms::Permission::from_string(arg)));
        }
        return permissions;
      };
      // Process Ops
      CmdPerms cmd_perms;
      switch(op)
      {
        case CmdPermsOp::SET:
        {
          cmd_perms.sub_cmd = CmdPerms::Set { .permissions = Expect(f_process_permissions()) };
        }
        break;
        case CmdPermsOp::ADD:
        {
          cmd_perms.sub_cmd = CmdPerms::Add { .permissions = Expect(f_process_permissions()) };
        }
        break;
        case CmdPermsOp::DEL:
        {
          cmd_perms.sub_cmd = CmdPerms::Del { .permissions = Expect(f_process_permissions()) };
        }
        break;
        case CmdPermsOp::LIST:
        {
          cmd_perms.sub_cmd = CmdPerms::List{};
        }
        break;
        case CmdPermsOp::CLEAR:
        {
          cmd_perms.sub_cmd = CmdPerms::Clear{};
        }
        break;
        case CmdPermsOp::NONE:
        {
          return Unexpected("Invalid operation on permissions");
        }
      }
      // Check for trailing arguments
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-perms: {}"_fmt(args.data())));
      return CmdType{cmd_perms};
    },
    // Configure environment
    ns_match::equal("fim-env") >>= [&] -> Expected<CmdType>
    {
      // Get op
      CmdEnvOp op = Expect(
        CmdEnvOp::from_string(Expect(args.pop_front("Missing op for 'fim-env' (add,del,list,set,clear)")))
      );
      // Gather arguments with key/values if any
      auto f_process_variables = [&] -> Expected<std::vector<std::string>>
      {
        qreturn_if(args.empty(), Unexpected("Missing arguments for '{}'"_fmt(op)));
        std::vector<std::string> out = args.data();
        args.clear();
        return out;
      };
      // Process command
      CmdEnv cmd_env;
      switch(op)
      {
        case CmdEnvOp::SET:
        {
          cmd_env.sub_cmd = CmdEnv::Set { .variables = Expect(f_process_variables()) };
        }
        break;
        case CmdEnvOp::ADD:
        {
          cmd_env.sub_cmd = CmdEnv::Add { .variables = Expect(f_process_variables()) };
        }
        break;
        case CmdEnvOp::DEL:
        {
          cmd_env.sub_cmd = CmdEnv::Del { .variables = Expect(f_process_variables()) };
        }
        break;
        case CmdEnvOp::LIST:
        {
          cmd_env.sub_cmd = CmdEnv::List{};
        }
        break;
        case CmdEnvOp::CLEAR:
        {
          cmd_env.sub_cmd = CmdEnv::Clear{};
        }
        break;
        case CmdEnvOp::NONE:
        {
          return Unexpected("Invalid operation on permissions");
        }
      }
      // Check for trailing arguments
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-env: {}"_fmt(args.data())));
      // Check if is other command with valid args
      return CmdType{cmd_env};
    },
    // Configure environment
    ns_match::equal("fim-desktop") >>= [&] -> Expected<CmdType>
    {
      // Check if is other command with valid args
      CmdDesktop cmd;
      // Get operation
      CmdDesktopOp op = Expect(CmdDesktopOp::from_string(
        Expect(args.pop_front("Missing op for 'fim-desktop' (enable,setup,clean,dump)"))
      ));
      // Get operation specific arguments
      switch(op)
      {
        case CmdDesktopOp::SETUP:
        {
          cmd.sub_cmd = CmdDesktop::Setup
          {
            .path_file_setup = Expect(args.pop_front("Missing argument from 'setup' (/path/to/file.json)"))
          };
        }
        break;
        case CmdDesktopOp::ENABLE:
        {
          // Get comma separated argument list
          std::vector<std::string> vec_items =
              Expect(args.pop_front("Missing arguments for 'enable' (desktop,entry,mimetype,none)"))
            | std::views::split(',')
            | std::ranges::to<std::vector<std::string>>();
          // Create items
          std::set<ns_desktop::IntegrationItem> set_enable;
          for(auto&& item : vec_items)
          {
            set_enable.insert(Expect(ns_desktop::IntegrationItem::from_string(item)));
          }
          // Check for 'none'
          if(set_enable.size() > 1 and set_enable.contains(ns_desktop::IntegrationItem::NONE))
          {
            return Unexpected("'none' option should not be used with others");
          }
          cmd.sub_cmd = CmdDesktop::Enable
          {
            .set_enable = set_enable
          };
        }
        break;
        case CmdDesktopOp::DUMP:
        {
          // Get dump operation
          CmdDesktopDump op_dump = Expect(CmdDesktopDump::from_string(
            Expect(args.pop_front("Missing arguments for 'dump' (desktop,icon)"))
          ));
          // Parse dump operation
          switch(op_dump)
          {
            case CmdDesktopDump::ICON:
            {
              cmd.sub_cmd = CmdDesktop::Dump { CmdDesktop::Dump::Icon {
                .path_file_icon = Expect(args.pop_front("Missing argument for 'icon' /path/to/dump/file"))
              }};
            }
            break;
            case CmdDesktopDump::ENTRY:
            {
              cmd.sub_cmd = CmdDesktop::Dump { CmdDesktop::Dump::Entry{} };
            }
            break;
            case CmdDesktopDump::MIMETYPE:
            {
              cmd.sub_cmd = CmdDesktop::Dump { CmdDesktop::Dump::MimeType{} };
            }
            break;
            case CmdDesktopDump::NONE: return Unexpected("Invalid desktop dump operation");
          }
        }
        break;
        case CmdDesktopOp::CLEAN:
        {
          cmd.sub_cmd = CmdDesktop::Clean{};
        }
        break;
        case CmdDesktopOp::NONE: return Unexpected("Invalid desktop operation");
      }
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-desktop: {}"_fmt(args.data())));
      return CmdType(cmd);
    },
    // Manage layers
    ns_match::equal("fim-layer") >>= [&] -> Expected<CmdType>
    {
      // Create cmd
      CmdLayer cmd;
      // Get op
      CmdLayerOp op = Expect(
        CmdLayerOp::from_string(Expect(args.pop_front("Missing op for 'fim-layer' (create,add)")))
      );
      // Process command
      switch(op)
      {
        case CmdLayerOp::ADD:
        {
          std::string error_msg = "add requires exactly one argument (/path/to/file.layer)";
          cmd.sub_cmd = CmdLayer::Add { .path_file_src = (Expect(args.pop_front(error_msg))) };
          qreturn_if(not args.empty(), Unexpected(error_msg));
        }
        break;
        case CmdLayerOp::CREATE:
        {
          std::string error_msg = "add requires exactly two arguments (/path/to/dir /path/to/file.layer)";
          cmd.sub_cmd = CmdLayer::Create
          {
            .path_dir_src = Expect(args.pop_front(error_msg)),
            .path_file_target = Expect(args.pop_front(error_msg))
          };
          qreturn_if(not args.empty(), Unexpected(error_msg));
        }
        break;
        case CmdLayerOp::COMMIT:
        {
          cmd.sub_cmd = CmdLayer::Commit{};
        }
        break;
        case CmdLayerOp::NONE: return Unexpected("Invalid layer operation");
      }
      return CmdType(cmd);
    },
    // Bind a path or device to inside the flatimage
    ns_match::equal("fim-bind") >>= [&] -> Expected<CmdType>
    {
      // Create command
      CmdBind cmd;
      // Check op
      CmdBindOp op = Expect(
        CmdBindOp::from_string(Expect(args.pop_front("Missing op for 'fim-bind' command (add,del,list)")))
      );
      // Process command
      switch(op)
      {
        case CmdBindOp::ADD:
        {
          std::string msg = "Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)";
          cmd.sub_cmd = CmdBind::Add
          {
            .type =  Expect(ns_db::ns_bind::Type::from_string(Expect(args.pop_front(msg)))),
            .path_src = Expect(args.pop_front(msg)),
            .path_dst = Expect(args.pop_front(msg))
          };
          qreturn_if(not args.empty(), Unexpected(msg));
        }
        break;
        case CmdBindOp::DEL:
        {
          std::string str_index = Expect(args.pop_front("Incorrect number of arguments for 'del' (<index>)"));
          qreturn_if(not std::ranges::all_of(str_index, ::isdigit)
            , Unexpected("Index argument for 'del' is not a number")
          );
          cmd.sub_cmd = CmdBind::Del
          {
            .index = std::stoull(str_index)
          };
          qreturn_if(not args.empty(), Unexpected("Incorrect number of arguments for 'del' (<index>)"));
        }
        break;
        case CmdBindOp::LIST:
        {
          cmd.sub_cmd = CmdBind::List{};
          qreturn_if(not args.empty(), Unexpected("'list' command takes no arguments"));
        }
        break;
        case CmdBindOp::NONE: return Unexpected("Invalid operation for bind");
      }
      return CmdType(cmd);
    },
    // Notifies with notify-send when the program starts
    ns_match::equal("fim-notify") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Incorrect number of arguments for 'fim-notify' (<on|off>)";
      auto cmd_notify = CmdType(CmdNotify{
        Expect(CmdNotifySwitch::from_string(Expect(args.pop_front(msg))))
      });
      qreturn_if(not args.empty(), Unexpected(msg));
      return cmd_notify;
    },
    // Enables or disable ignore case for paths (useful for wine)
    ns_match::equal("fim-casefold") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Incorrect number of arguments for 'fim-casefold' (<on|off>)";
      qreturn_if(args.empty(), Unexpected(msg));
      auto cmd_casefold = CmdType(CmdCaseFold{
        Expect(CmdCaseFoldSwitch::from_string(Expect(args.pop_front(msg))))
      });
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-casefold: {}"_fmt(args.data())));
      return cmd_casefold;
    },
    // Set the default startup command
    ns_match::equal("fim-boot") >>= [&] -> Expected<CmdType>
    {
      // Check op
      CmdBootOp op = Expect(CmdBootOp::from_string(
        Expect(args.pop_front("Invalid operation for 'fim-boot' (<set|show|clear>)"))
      ));
      // Build command
      CmdBoot cmd_boot;
      switch(op)
      {
        case CmdBootOp::SET:
        {
          cmd_boot.sub_cmd = CmdBoot::Set {
            .program = Expect(args.pop_front("Missing program for 'set' operation")),
            .args = args.data()
          };
          args.clear();
        }
        break;
        case CmdBootOp::SHOW:
        {
          cmd_boot.sub_cmd = CmdBoot::Show{};
        }
        break;
        case CmdBootOp::CLEAR:
        {
          cmd_boot.sub_cmd = CmdBoot::Clear{};
        }
        break;
        case CmdBootOp::NONE: return Unexpected("Invalid boot operation");
      }
      // Check for trailing arguments
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-boot: {}"_fmt(args.data())));
      return cmd_boot;
    },
    // Run a command in an existing instance
    ns_match::equal("fim-instance") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Missing op for 'fim-instance' (<exec|list>)";
      CmdInstanceOp op = Expect(CmdInstanceOp::from_string(Expect(args.pop_front(msg))));
      CmdInstance cmd;
      switch(op)
      {
        case CmdInstanceOp::EXEC:
        {
          std::string str_id = Expect(args.pop_front("Missing 'id' argument for 'fim-instance'"));
          qreturn_if(not std::ranges::all_of(str_id, ::isdigit), Unexpected("Id argument must be a digit"));
          qreturn_if(args.empty(), Unexpected("Missing 'cmd' argument for 'fim-instance'"));
          cmd.sub_cmd = CmdInstance::Exec
          {
            .id = std::stoi(str_id),
            .args = args.data(),
          };
          args.clear();
        }
        break;
        case CmdInstanceOp::LIST:
        {
          cmd.sub_cmd = CmdInstance::List{};
        }
        break;
        case CmdInstanceOp::NONE: return Unexpected("Invalid instance operation");
      }
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-instance: {}"_fmt(args.data())));
      return CmdType(cmd);
    },
    // Select or show the current overlay filesystem
    ns_match::equal("fim-overlay") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Missing op for 'fim-overlay' (<set|show>)";
      // Get op
      CmdOverlayOp op = Expect(CmdOverlayOp::from_string(Expect(args.pop_front(msg))));
      // Build command
      CmdOverlay cmd;
      switch(op)
      {
        case CmdOverlayOp::SET:
        {
          cmd.sub_cmd = CmdOverlay::Set
          {
            .overlay = Expect(ns_reserved::ns_overlay::OverlayType::from_string(           
              Expect(args.pop_front("Missing argument for 'set'"))
            ))
          };
        }
        break;
        case CmdOverlayOp::SHOW:
        {
          cmd.sub_cmd = CmdOverlay::Show{};
        }
        break;
        case CmdOverlayOp::NONE:
        {
          return Unexpected("Invalid operation for fim-overlay");
        }
      }
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-overlay: {}"_fmt(args.data())));
      return CmdType(cmd);
    },
    // Select or show the current overlay filesystem
    ns_match::equal("fim-version") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Missing op for 'fim-version' (<short|full|deps>)";
      // Get op
      CmdVersionOp op = Expect(CmdVersionOp::from_string(Expect(args.pop_front(msg))));
      // Build command
      CmdVersion cmd;
      switch(op)
      {
        case CmdVersionOp::SHORT: cmd.sub_cmd = CmdVersion::Short{}; break;
        case CmdVersionOp::FULL: cmd.sub_cmd = CmdVersion::Full{}; break;
        case CmdVersionOp::DEPS: cmd.sub_cmd = CmdVersion::Deps{}; break;
        case CmdVersionOp::NONE: return Unexpected("Invalid operation for fim-overlay");
      }
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-overlay: {}"_fmt(args.data())));
      return CmdType(cmd);
    },
    // Use the default startup command
    ns_match::equal("fim-help") >>= [&]
    {
      if (args.empty())
      {
        std::cerr << ns_cmd::ns_help::help_usage() << '\n';
        return CmdType(CmdExit{});
      }
      auto message = ns_match::match(args.pop_front("Missing argument for 'fim-help'"),
        ns_match::equal("bind")     >>=  ns_cmd::ns_help::bind_usage(),
        ns_match::equal("boot")     >>=  ns_cmd::ns_help::boot_usage(),
        ns_match::equal("casefold") >>=  ns_cmd::ns_help::casefold_usage(),
        ns_match::equal("desktop")  >>=  ns_cmd::ns_help::desktop_usage(),
        ns_match::equal("env")      >>=  ns_cmd::ns_help::env_usage(),
        ns_match::equal("exec")     >>=  ns_cmd::ns_help::exec_usage(),
        ns_match::equal("instance") >>=  ns_cmd::ns_help::instance_usage(),
        ns_match::equal("layer")    >>=  ns_cmd::ns_help::layer_usage(),
        ns_match::equal("notify")   >>=  ns_cmd::ns_help::notify_usage(),
        ns_match::equal("overlay")   >>=  ns_cmd::ns_help::overlay_usage(),
        ns_match::equal("perms")    >>=  ns_cmd::ns_help::perms_usage(),
        ns_match::equal("root")     >>=  ns_cmd::ns_help::root_usage(),
        ns_match::equal("version")  >>=  ns_cmd::ns_help::version_usage()
      );
      std::cout << message.value_or("Invalid argument for help command\n");
      return CmdType(CmdExit{});
    }
  ));
}

/**
 * @brief Parses and executes a command
 * 
 * @param config FlatImage configuration object
 * @param argc Argument counter
 * @param argv Argument vector
 * @return Expected<int> The exit code on success or the respective error
 */
[[nodiscard]] inline Expected<int> parse_cmds(ns_config::FlatimageConfig& config, int argc, char** argv)
{
  // Parse args
  CmdType variant_cmd = Expect(ns_parser::parse(argc, argv));

  auto f_bwrap_impl = [&](auto&& program, auto&& args) -> Expected<ns_bwrap::bwrap_run_ret_t>
  {
    // Initialize permissions
    ns_reserved::ns_permissions::Permissions permissions(config.path_file_binary);
    // Mount filesystems
    [[maybe_unused]] auto filesystem_controller = ns_filesystems::ns_controller::Controller(config);
    // Execute specified command
    auto environment = ExpectedOrDefault(ns_db::ns_env::get(config.path_file_binary));
    // Check if should use bwrap native overlayfs
    std::optional<ns_bwrap::Overlay> bwrap_overlay = ( config.overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP )?
        std::make_optional(ns_bwrap::Overlay
        {
            .vec_path_dir_layer = ns_config::get_mounted_layers(config.path_dir_mount_layers)
          , .path_dir_upper = config.path_dir_upper_overlayfs
          , .path_dir_work = config.path_dir_work_overlayfs
        })
      : std::nullopt;
    fs::path path_dir_root = ( config.is_casefold and config.overlay_type != ns_reserved::ns_overlay::OverlayType::BWRAP )?
        config.path_dir_mount_ciopfs
      : config.path_dir_mount_overlayfs;
    // Create bwrap command
    ns_bwrap::Bwrap bwrap = ns_bwrap::Bwrap(config.is_root
      , bwrap_overlay
      , path_dir_root
      , config.path_file_bashrc
      , program
      , args
      , environment);
    // Include root binding and custom user-defined bindings
    std::ignore = bwrap
      .with_bind_ro("/", config.path_dir_runtime_host)
      .with_binds(Expect(ns_cmd::ns_bind::db_read(config.path_file_binary)));
    // Check if should enable GPU
    if (permissions.contains(ns_reserved::ns_permissions::Permission::GPU))
    {
      std::ignore = bwrap.with_bind_gpu(config.path_dir_upper_overlayfs, config.path_dir_runtime_host);
    }
    // Run bwrap
    return bwrap.run(permissions, config.path_dir_app_bin);
  };


  auto f_bwrap = [&]<typename T, typename U>(T&& program, U&& args) -> Expected<int>
  {
    // Setup desktop integration, permissive
    if(auto ret = ns_desktop::integrate(config); not ret)
    {
      ns_log::error()("Could not perform desktop integration: {}", ret.error());
    }
    // Run bwrap
    ns_bwrap::bwrap_run_ret_t bwrap_run_ret = Expect(f_bwrap_impl(program, args));
    // Log bwrap errors
    elog_if(bwrap_run_ret.errno_nr > 0
      , "Bwrap failed syscall '{}' with errno '{}'"_fmt(bwrap_run_ret.syscall_nr, bwrap_run_ret.errno_nr)
    );
    // Retry with fallback if bwrap overlayfs failed
    if ( config.overlay_type == ns_reserved::ns_overlay::OverlayType::BWRAP and bwrap_run_ret.syscall_nr == SYS_mount )
    {
      ns_log::error()("Bwrap failed SYS_mount, retrying with fuse-unionfs...");
      config.overlay_type = ns_reserved::ns_overlay::OverlayType::UNIONFS;
      bwrap_run_ret = Expect(f_bwrap_impl(program, args));
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
      Expect(permissions.add(cmd_add->permissions));
    }
    else if(std::get_if<CmdPerms::Clear>(&(cmd->sub_cmd)))
    {
      Expect(permissions.set_all(false));
    }
    else if(auto cmd_del = std::get_if<CmdPerms::Del>(&(cmd->sub_cmd)))
    {
      Expect(permissions.del(cmd_del->permissions));
    }
    else if(std::get_if<CmdPerms::List>(&(cmd->sub_cmd)))
    {
      std::ranges::copy(Expect(permissions.to_strings())
        , std::ostream_iterator<std::string>(std::cout, "\n")
      );
    }
    else if(auto cmd_set = std::get_if<CmdPerms::Set>(&(cmd->sub_cmd)))
    {
      Expect(permissions.set(cmd_set->permissions));
    }
    else
    {
      return Unexpected("Invalid permissions sub-command");
    }
  }
  // Configure environment
  else if ( auto cmd = std::get_if<ns_parser::CmdEnv>(&variant_cmd) )
  {
    if(auto cmd_add = std::get_if<CmdEnv::Add>(&(cmd->sub_cmd)))
    {
      Expect(ns_db::ns_env::add(config.path_file_binary, cmd_add->variables));
    }
    else if(std::get_if<CmdEnv::Clear>(&(cmd->sub_cmd)))
    {
      Expect(ns_db::ns_env::set(config.path_file_binary, std::vector<std::string>()));
    }
    else if(auto cmd_del = std::get_if<CmdEnv::Del>(&(cmd->sub_cmd)))
    {
      Expect(ns_db::ns_env::del(config.path_file_binary, cmd_del->variables));
    }
    else if(std::get_if<CmdEnv::List>(&(cmd->sub_cmd)))
    {
      std::ranges::copy(Expect(ns_db::ns_env::get(config.path_file_binary))
        , std::ostream_iterator<std::string>(std::cout, "\n")
      );
    }
    else if(auto cmd_set = std::get_if<CmdEnv::Set>(&(cmd->sub_cmd)))
    {
      Expect(ns_db::ns_env::set(config.path_file_binary, cmd_set->variables));
    }
    else
    {
      return Unexpected("Invalid environment sub-command");
    }
  }
  // Configure desktop integration
  else if (auto cmd = std::get_if<CmdDesktop>(&variant_cmd))
  {
    if(auto cmd_setup = std::get_if<CmdDesktop::Setup>(&(cmd->sub_cmd)))
    {
      Expect(ns_desktop::setup(config, cmd_setup->path_file_setup));
    }
    else if(auto cmd_enable = std::get_if<CmdDesktop::Enable>(&(cmd->sub_cmd)))
    {
      Expect(ns_desktop::enable(config, cmd_enable->set_enable));
    }
    else if(std::get_if<CmdDesktop::Clean>(&(cmd->sub_cmd)))
    {
      Expect(ns_desktop::clean(config));
    }
    else if(auto cmd_dump = std::get_if<CmdDesktop::Dump>(&(cmd->sub_cmd)))
    {
      if(auto cmd_icon = std::get_if<CmdDesktop::Dump::Icon>(&(cmd_dump->sub_cmd)))
      {
        Expect(ns_desktop::dump_icon(config, cmd_icon->path_file_icon));
      }
      else if(std::get_if<CmdDesktop::Dump::Entry>(&(cmd_dump->sub_cmd)))
      {
        std::println("{}", Expect(ns_desktop::dump_entry(config)));
      }
      else if(std::get_if<CmdDesktop::Dump::MimeType>(&(cmd_dump->sub_cmd)))
      {
        std::println("{}", Expect(ns_desktop::dump_mimetype(config)));
      }
      else
      {
        return Unexpected("Invalid dump sub-command");
      }
    }
    else
    {
      return Unexpected("Invalid desktop sub-command");
    }
  }
  // Manager layers
  else if ( auto cmd = std::get_if<ns_parser::CmdLayer>(&variant_cmd) )
  {
    if(auto cmd_add = std::get_if<CmdLayer::Add>(&(cmd->sub_cmd)))
    {
      Expect(ns_layers::add(config.path_file_binary, cmd_add->path_file_src));
    }
    else if(std::get_if<CmdLayer::Commit>(&(cmd->sub_cmd)))
    {
      Expect(ns_layers::commit(config.path_file_binary
        , config.path_dir_upper_overlayfs
        , config.path_dir_host_config_tmp / "layer.tmp"
        , config.path_dir_host_config_tmp / "compression.list"
        , config.layer_compression_level
      ));
    }
    else if(auto cmd_create = std::get_if<CmdLayer::Create>(&(cmd->sub_cmd)))
    {
      Expect(ns_layers::create(cmd_create->path_dir_src
        , cmd_create->path_file_target
        , config.path_dir_host_config_tmp / "compression.list"
        , config.layer_compression_level
      ));
    }
    else
    {
      return Unexpected("Invalid layer operation");
    }
  }
  // Bind a device or file to the flatimage
  else if ( auto cmd = std::get_if<ns_parser::CmdBind>(&variant_cmd) )
  {
    if(auto cmd_add = std::get_if<CmdBind::Add>(&(cmd->sub_cmd)))
    {
      Expect(ns_cmd::ns_bind::add(config.path_file_binary
        , cmd_add->type
        , cmd_add->path_src
        , cmd_add->path_dst)
      );
    }
    else if(auto cmd_del = std::get_if<CmdBind::Del>(&(cmd->sub_cmd)))
    {
      Expect(ns_cmd::ns_bind::del(config.path_file_binary, cmd_del->index));
    }
    else if(std::get_if<CmdBind::List>(&(cmd->sub_cmd)))
    {
      Expect(ns_cmd::ns_bind::list(config.path_file_binary));
    }
    else
    {
      return Unexpected("Invalid bind operation");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdNotify>(&variant_cmd) )
  {
    Expect(ns_reserved::ns_notify::write(config.path_file_binary, cmd->status == CmdNotifySwitch::ON));
  } // else if
  // Enable or disable casefold (useful for wine)
  else if ( auto cmd = std::get_if<ns_parser::CmdCaseFold>(&variant_cmd) )
  {
    Expect(ns_reserved::ns_casefold::write(config.path_file_binary, cmd->status == CmdCaseFoldSwitch::ON));
  } // else if
  // Update default command on database
  else if ( auto cmd = std::get_if<ns_parser::CmdBoot>(&variant_cmd) )
  {
    if(std::get_if<CmdBoot::Clear>(&(cmd->sub_cmd)))
    {
      // Create empty database
      ns_db::Db db;
      // Write empty database
      Expect(ns_reserved::ns_boot::write(config.path_file_binary, db.dump()));
    }
    else if(auto cmd_set = std::get_if<CmdBoot::Set>(&(cmd->sub_cmd)))
    {
      // Create database
      ns_db::Db db;
      // Insert values
      db("program") = cmd_set->program;
      db("args") = cmd_set->args;
      // Write fields
      Expect(ns_reserved::ns_boot::write(config.path_file_binary, db.dump()));
    }
    else if(std::get_if<CmdBoot::Show>(&(cmd->sub_cmd)))
    {
      // Read database
      ns_db::Db db = ns_db::from_string(
        Expect(ns_reserved::ns_boot::read(config.path_file_binary))
      ).value_or(ns_db::Db());
      // Print database
      if(not db.empty())
      {
        std::cout << db.dump() << '\n';
      }
    }
    else
    {
      return Unexpected("Invalid boot sub-command");
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdInstance>(&variant_cmd) )
  {
    // List instances
    auto f_filename = [](auto&& e){ return e.path().filename().string(); };
    // Get instances
    auto instances = fs::directory_iterator(config.path_dir_app / "instance")
      | std::views::filter([&](auto&& e){ return fs::exists(fs::path{"/proc"} / f_filename(e)); })
      | std::views::filter([&](auto&& e){ return std::stoi(f_filename(e)) != getpid(); })
      | std::views::transform([](auto&& e){ return e.path(); })
      | std::ranges::to<std::vector<fs::path>>();
    // Sort by pid (filename is a directory named as the pid of that instance)
    std::ranges::sort(instances, {}, [](auto&& e){ return std::stoi(e.filename().string()); });
    if(auto cmd_exec = std::get_if<CmdInstance::Exec>(&(cmd->sub_cmd)))
    {
      qreturn_if(instances.size() == 0, Unexpected("No instances are running"));
      qreturn_if(cmd_exec->id < 0 or static_cast<size_t>(cmd_exec->id) >= instances.size()
        , Unexpected("Instance index out of bounds")
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
      return Unexpected("Invalid instance operation");     
    }
  }
  else if ( auto cmd = std::get_if<ns_parser::CmdOverlay>(&variant_cmd) )
  {
    if(auto cmd_set = std::get_if<CmdOverlay::Set>(&(cmd->sub_cmd)))
    {
      Expect(ns_reserved::ns_overlay::write(config.path_file_binary, cmd_set->overlay));
    }
    else if(std::get_if<CmdOverlay::Show>(&(cmd->sub_cmd)))
    {
      std::println("{}", std::string{config.overlay_type});
    }
    else
    {
      return Unexpected("Invalid operation for fim-overlay");
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
      std::println("{}", cmd_full->dump());
    }
    else if(auto cmd_deps = std::get_if<CmdVersion::Deps>(&(cmd->sub_cmd)))
    {
      std::println("{}", Expect(cmd_deps->dump()));
    }
    else
    {
      return Unexpected("Invalid operation for fim-version");
    }    
  }
  // Update default command on database
  else if ( std::get_if<ns_parser::CmdNone>(&variant_cmd) )
  {
    auto db = ns_db::from_string(Expect(ns_reserved::ns_boot::read(config.path_file_binary)))
      .value_or(ns_db::Db());
    // Fetch default command
    fs::path program = [&] -> Expected<fs::path> {
      // Read startup program
      auto program = Expect(db("program").template value<std::string>());
      // Expand program if it is an environment variable or shell expression
      return ns_env::expand(program).value_or(program);
    }().value_or("bash");
    // Fetch default arguments
    std::vector<std::string> args = [&] -> Expected<std::vector<std::string>> {
      using Args = std::vector<std::string>;
      // Read arguments
      Args args = Expect(db("args").template value<Args>());
      // Append arguments from argv
      std::copy(argv+1, argv+argc, std::back_inserter(args));
      return Expected<Args>(args);
    }().value_or(std::vector<std::string>{});
    // Run Bwrap
    return f_bwrap(program, args);
  } // else if
  else if ( std::get_if<ns_parser::CmdExit>(&variant_cmd) )
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return Unexpected("Unknown command");
  }

  return EXIT_SUCCESS;
}

} // namespace ns_parser

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
