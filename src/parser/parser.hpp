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
#include "../db/environment.hpp"
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
[[nodiscard]] inline Expected<CmdType> parse(int argc , char** argv) noexcept
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
      // Invalid op
      qreturn_if(op == CmdPermsOp::NONE, Unexpected("Invalid operation on permissions"));
      // Valid ops
      CmdType cmd_type;
      if(op == CmdPermsOp::LIST or op == CmdPermsOp::CLEAR)
      {
        cmd_type = CmdPerms{ .op = op, .permissions = {} };
      }
      else
      {
        CmdPerms cmd_perms;
        cmd_perms.op = op;
        cmd_perms.permissions = ns_vector::from_string(Expect(args.pop_front("No arguments for '{}' command"_fmt(op))), ',')
          | std::views::transform([](auto&& s){ return std::views::transform(s, ::tolower) | std::ranges::to<std::string>(); })
          | std::ranges::to<std::vector<std::string>>();
        cmd_type = cmd_perms;
      }
      // Check for trailing arguments
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-perms: {}"_fmt(args.data())));
      return cmd_type;
    },
    // Configure environment
    ns_match::equal("fim-env") >>= [&] -> Expected<CmdType>
    {
      // Get op
      CmdEnvOp op = Expect(
        CmdEnvOp::from_string(Expect(args.pop_front("Missing op for 'fim-env' (add,del,list,set,clear)")))
      );
      // Invalid op
      qreturn_if(op == CmdEnvOp::NONE, Unexpected("Invalid operation on environment"));
      // Valid Ops
      CmdType cmd_type;
      if(op == CmdEnvOp::LIST or op == CmdEnvOp::CLEAR)
      {
        cmd_type = CmdEnv{ .op = op, .environment = {} };
      }
      else
      {
        qreturn_if(args.empty(), Unexpected("Missing arguments for '{}'"_fmt(op)));
        cmd_type = CmdEnv({ .op = op, .environment = args.data() });
        args.clear();
      }
      // Check for trailing arguments
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-env: {}"_fmt(args.data())));
      // Check if is other command with valid args
      return cmd_type;
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
      cmd.op = Expect(
        CmdLayerOp::from_string(Expect(args.pop_front("Missing op for 'fim-layer' (create,add)")))
      );
      qreturn_if(cmd.op == CmdLayerOp::NONE, Unexpected("Invalid layer operation"));
      // Gather operation specific arguments
      if ( cmd.op == CmdLayerOp::ADD )
      {
        std::string error_msg = "add requires exactly one argument (/path/to/file.layer)";
        cmd.args.push_back(Expect(args.pop_front(error_msg)));
        qreturn_if(not args.empty(), Unexpected(error_msg));
      } // if
      else
      {
        std::string error_msg = "add requires exactly two arguments (/path/to/dir /path/to/file.layer)";
        cmd.args.push_back(Expect(args.pop_front(error_msg)));
        cmd.args.push_back(Expect(args.pop_front(error_msg)));
        qreturn_if(not args.empty(), Unexpected(error_msg));
      } // else
      return CmdType(cmd);
    },
    // Bind a path or device to inside the flatimage
    ns_match::equal("fim-bind") >>= [&] -> Expected<CmdType>
    {
      // Create command
      CmdBind cmd;
      // Check op
      cmd.op = Expect(
        CmdBindOp::from_string(Expect(args.pop_front("Missing op for 'fim-bind' command (add,del,list)")))
      );
      qreturn_if( cmd.op == CmdBindOp::NONE, Unexpected("Invalid bind operation"));
      // Gather operation specific arguments
      using RetType = Expected<CmdBind::cmd_bind_data_t>;
      cmd.data = Expect(Expect(ns_match::match(cmd.op
        , ns_match::equal(CmdBindOp::ADD) >>= [&] -> RetType
        {
          std::string msg = "Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)";
          CmdBind::cmd_bind_data_t data = CmdBind::cmd_bind_data_t(
            CmdBind::cmd_bind_t(
                Expect(CmdBindType::from_string(Expect(args.pop_front(msg))))
              , Expect(args.pop_front(msg))
              , Expect(args.pop_front(msg)))
          );
          qreturn_if(not args.empty(), Unexpected(msg));
          return data;
        }
        , ns_match::equal(CmdBindOp::DEL) >>= [&] -> RetType
        {
          std::string str_index = Expect(args.pop_front("Incorrect number of arguments for 'del' (<index>)"));
          qreturn_if(not args.empty(), Unexpected("Incorrect number of arguments for 'del' (<index>)"));
          qreturn_if(not std::ranges::all_of(str_index, ::isdigit)
            , Unexpected("Index argument for 'del' is not a number")
          );
          return CmdBind::cmd_bind_data_t(CmdBind::cmd_bind_index_t(std::stoi(str_index)));
        }
        , ns_match::equal(CmdBindOp::LIST) >>= [&] -> RetType
        {
          return args.empty()? RetType(CmdBind::cmd_bind_data_t(std::false_type{}))
            : Unexpected("'list' command takes no arguments");
        }
        , ns_match::equal(CmdBindOp::NONE) >>= RetType(std::unexpected("Invalid operation for bind"))
      )));
      return CmdType(cmd);
    },
    // Commit current files to a novel compressed layer
    ns_match::equal("fim-commit") >>= [&] -> Expected<CmdType>
    {
      qreturn_if(not args.empty(), Unexpected("'fim-commit' does not take arguments"));
      return CmdType(CmdCommit{});
    },
    // Notifies with notify-send when the program starts
    ns_match::equal("fim-notify") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Incorrect number of arguments for 'fim-notify' (<on|off>)";
      auto cmd_notify = CmdType(CmdNotify{
        Expect(CmdNotifyOp::from_string(Expect(args.pop_front(msg))))
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
        Expect(CmdCaseFoldOp::from_string(Expect(args.pop_front(msg))))
      });
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-casefold: {}"_fmt(args.data())));
      return cmd_casefold;
    },
    // Set the default startup command
    ns_match::equal("fim-boot") >>= [&] -> Expected<CmdType>
    {
      CmdBoot cmd_boot;
      // Check op
      cmd_boot.op = Expect(CmdBootOp::from_string(
        Expect(args.pop_front("Invalid operation for 'fim-boot' (<set|show|clear>)"))
      ));
      // Process ops
      switch(cmd_boot.op)
      {
        case CmdBootOp::SET:
          cmd_boot.program = Expect(args.pop_front("Missing program for 'set' operation"));
          cmd_boot.args = args.data();
          args.clear();
        break;
        case CmdBootOp::SHOW: break;
        case CmdBootOp::CLEAR: break;
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
      CmdType cmd_type;
      // List
      if(op == CmdInstanceOp::LIST)
      {
        cmd_type = CmdInstance(op, -1, {});
      }
      // Exec
      else
      {
        std::string str_id = Expect(args.pop_front("Missing 'id' argument for 'fim-instance'"));
        qreturn_if(not std::ranges::all_of(str_id, ::isdigit)
          , Unexpected("Id argument must be a digit")
        );
        qreturn_if(args.empty(), Unexpected("Missing 'cmd' argument for 'fim-instance'"));
        cmd_type = CmdInstance(op
          , std::stoi(str_id)
          , args.data()
        );
        args.clear();
      }
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-instance: {}"_fmt(args.data())));
      return cmd_type;
    },
    // Select or show the current overlay filesystem
    ns_match::equal("fim-overlay") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Missing op for 'fim-overlay' (<set|show>)";
      // Get op
      CmdOverlayOp op = Expect(CmdOverlayOp::from_string(Expect(args.pop_front(msg))));
      // Build command
      CmdType cmd_type;
      switch(op)
      {
        case CmdOverlayOp::SET:
        {
          ns_reserved::ns_overlay::OverlayType overlay = Expect(ns_reserved::ns_overlay::OverlayType::from_string(
            Expect(args.pop_front("Missing argument for 'set'"))
          ));
          cmd_type = CmdOverlay(op, overlay);
        }
        break;
        case CmdOverlayOp::SHOW:
        {
          cmd_type = CmdOverlay(op, ns_reserved::ns_overlay::OverlayType::NONE);
        }
        break;
        case CmdOverlayOp::NONE:
        {
          return Unexpected("Invalid operation for fim-overlay");
        }
      }
      qreturn_if(not args.empty(), Unexpected("Trailing arguments for fim-overlay: {}"_fmt(args.data())));
      return cmd_type;
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
        ns_match::equal("commit")   >>=  ns_cmd::ns_help::commit_usage(),
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
[[nodiscard]] inline Expected<int> parse_cmds(ns_config::FlatimageConfig& config, int argc, char** argv) noexcept
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
    auto environment = ExpectedOrDefault(ns_db::ns_environment::get(config.path_file_config_environment));
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
      .with_binds_from_file(config.path_file_config_bindings);
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
    using Permission = ns_reserved::ns_permissions::Permission;
    // Create permissions from input strings
    std::set<Permission> set_permissions;
    for(auto arg : cmd->permissions)
    {
      set_permissions.insert(Expect(Permission::from_string(arg)));
    }
    // Retrieve permissions
    ns_reserved::ns_permissions::Permissions permissions(config.path_file_binary);
    // Determine permission operation
    switch( cmd->op )
    {
      case ns_parser::CmdPermsOp::ADD: Expect(permissions.add(set_permissions)); break;
      case ns_parser::CmdPermsOp::SET: Expect(permissions.set(set_permissions)); break;
      case ns_parser::CmdPermsOp::DEL: Expect(permissions.del(set_permissions)); break;
      case ns_parser::CmdPermsOp::CLEAR: Expect(permissions.set_all(false)); break;
      case ns_parser::CmdPermsOp::LIST:
        std::ranges::copy(Expect(permissions.to_strings())
          , std::ostream_iterator<std::string>(std::cout, "\n")
        );
        break;
      case ns_parser::CmdPermsOp::NONE: return Unexpected("Invalid operation for permissions");
    } // switch
  } // if
  // Configure environment
  else if ( auto cmd = std::get_if<ns_parser::CmdEnv>(&variant_cmd) )
  {
    // Determine open mode
    switch( cmd->op )
    {
      case ns_parser::CmdEnvOp::ADD: Expect(ns_db::ns_environment::add(config.path_file_config_environment, cmd->environment)); break;
      case ns_parser::CmdEnvOp::SET: Expect(ns_db::ns_environment::set(config.path_file_config_environment, cmd->environment)); break;
      case ns_parser::CmdEnvOp::DEL: Expect(ns_db::ns_environment::del(config.path_file_config_environment, cmd->environment)); break;
      case ns_parser::CmdEnvOp::CLEAR: Expect(ns_db::ns_environment::set(config.path_file_config_environment, cmd->environment)); break;
      case ns_parser::CmdEnvOp::LIST:
        std::ranges::copy(Expect(ns_db::ns_environment::get(config.path_file_config_environment))
          , std::ostream_iterator<std::string>(std::cout, "\n")
        );
        break;
      case ns_parser::CmdEnvOp::NONE: return Unexpected("Invalid operation for environment");
    } // switch
  }
  // Configure desktop integration
  else if (auto cmd = std::get_if<CmdDesktop>(&variant_cmd))
  {
    if(auto cmd_desktop = std::get_if<CmdDesktop::Setup>(&(cmd->sub_cmd)))
    {
      Expect(ns_desktop::setup(config, cmd_desktop->path_file_setup));
    }
    else if(auto cmd_desktop = std::get_if<CmdDesktop::Enable>(&(cmd->sub_cmd)))
    {
      Expect(ns_desktop::enable(config, cmd_desktop->set_enable));
    }
    else if(std::get_if<CmdDesktop::Clean>(&(cmd->sub_cmd)))
    {
      Expect(ns_desktop::clean(config));
    }
    else if(auto cmd_desktop = std::get_if<CmdDesktop::Dump>(&(cmd->sub_cmd)))
    {
      if(auto cmd_dump = std::get_if<CmdDesktop::Dump::Icon>(&(cmd_desktop->sub_cmd)))
      {
        Expect(ns_desktop::dump_icon(config, cmd_dump->path_file_icon));
      }
      else if(std::get_if<CmdDesktop::Dump::Entry>(&(cmd_desktop->sub_cmd)))
      {
        std::println("{}", Expect(ns_desktop::dump_entry(config)));
      }
      else if(std::get_if<CmdDesktop::Dump::MimeType>(&(cmd_desktop->sub_cmd)))
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
    switch(cmd->op)
    {
      case CmdLayerOp::ADD:
        Expect(ns_layers::add(config.path_file_binary, cmd->args.front()));
        break;
      case CmdLayerOp::CREATE:
        Expect(ns_layers::create(cmd->args.at(0)
          , cmd->args.at(1)
          , config.path_dir_host_config_tmp / "compression.list"
          , config.layer_compression_level
        ));
        break;
      case CmdLayerOp::NONE:
        return Unexpected("Invalid desktop operation");
    }
  } // else if
  // Bind a device or file to the flatimage
  else if ( auto cmd = std::get_if<ns_cmd::ns_bind::CmdBind>(&variant_cmd) )
  {
    ns_cmd::ns_bind::CmdBind::cmd_bind_index_t cmd_bind_index;
    ns_cmd::ns_bind::CmdBind::cmd_bind_t cmd_bind_data;
    // Perform selected op
    switch(cmd->op)
    {
      case CmdBindOp::ADD:
        cmd_bind_data = std::get<ns_cmd::ns_bind::CmdBind::cmd_bind_t>(cmd->data);
        Expect(ns_cmd::ns_bind::add(config.path_file_config_bindings
          , cmd_bind_data.type
          , cmd_bind_data.src
          , cmd_bind_data.dst)
        );
        break;
      case CmdBindOp::DEL:
        cmd_bind_index = std::get<ns_cmd::ns_bind::CmdBind::cmd_bind_index_t>(cmd->data);
        Expect(ns_cmd::ns_bind::del(config.path_file_config_bindings, cmd_bind_index));
        break;
      case CmdBindOp::LIST:
        Expect(ns_cmd::ns_bind::list(config.path_file_config_bindings));
        break;
      case CmdBindOp::NONE:
        return Unexpected("Invalid bind operation");
    } // switch
  } // else if
  // Commit changes as a novel layer into the flatimage
  else if ( std::get_if<ns_parser::CmdCommit>(&variant_cmd) )
  {
    // Set source directory and target compressed file
    fs::path path_file_layer_tmp = config.path_dir_host_config_tmp / "layer.tmp";
    fs::path path_file_list_tmp = config.path_dir_host_config_tmp / "compression.list";
    // Create filesystem based on the contents of src
    Expect(ns_layers::create(config.path_dir_upper_overlayfs
      , path_file_layer_tmp
      , path_file_list_tmp
      , config.layer_compression_level
    ));
    // Include filesystem in the image
    Expect(ns_layers::add(config.path_file_binary, path_file_layer_tmp));
    // Remove layer file
    std::error_code ec;
    if(not fs::remove(path_file_layer_tmp))
    {
      ns_log::error()("Could not erase layer file '{}'", path_file_layer_tmp.string());
    }
    // Remove files from the compression list
    std::ifstream file_list(path_file_list_tmp);
    qreturn_if(not file_list.is_open(), Unexpected("Could not open file list for erasing files..."));
    std::string line;
    while(std::getline(file_list, line))
    {
      fs::path path_file_target = config.path_dir_upper_overlayfs / line;
      fs::path path_dir_parent = path_file_target.parent_path();
      // Remove target file
      if(not fs::remove(path_file_target, ec))
      {
        ns_log::error()("Could not remove file {}"_fmt(path_file_target));
      }
      // Remove empty directory
      if(fs::is_empty(path_dir_parent, ec) and not fs::remove(path_dir_parent, ec))
      {
        ns_log::error()("Could not remove directory {}"_fmt(path_dir_parent));
      }
    }
  } // else if
  else if ( auto cmd = std::get_if<ns_parser::CmdNotify>(&variant_cmd) )
  {
    Expect(ns_reserved::ns_notify::write(config.path_file_binary, cmd->op == CmdNotifyOp::ON));
  } // else if
  // Enable or disable casefold (useful for wine)
  else if ( auto cmd = std::get_if<ns_parser::CmdCaseFold>(&variant_cmd) )
  {
    Expect(ns_reserved::ns_casefold::write(config.path_file_binary, cmd->op == CmdCaseFoldOp::ON));
  } // else if
  // Update default command on database
  else if ( auto cmd = std::get_if<ns_parser::CmdBoot>(&variant_cmd) )
  {
    switch(cmd->op)
    {
      case CmdBootOp::SET:
      {
        // Create database
        ns_db::Db db;
        // Insert values
        db("program") = cmd->program;
        db("args") = cmd->args;
        // Write fields
        Expect(ns_reserved::ns_boot::write(config.path_file_binary, db.dump()));
      }
      break;
      case CmdBootOp::SHOW:
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
      break;
      case CmdBootOp::CLEAR:
      {
        // Create empty database
        ns_db::Db db;
        // Write empty database
        Expect(ns_reserved::ns_boot::write(config.path_file_binary, db.dump()));
      }
      break;
      case CmdBootOp::NONE: return Unexpected("Invalid boot operation");
    }
  } // else if
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
    switch(cmd->op)
    {
      case CmdInstanceOp::LIST:
      {
        for(uint32_t i = 0; fs::path const& instance : instances)
        {
          std::println("{}:{}", i++, instance.filename().string());
        }
      }
      break;
      case CmdInstanceOp::EXEC:
      {
        qreturn_if(instances.size() == 0, Unexpected("No instances are running"));
        qreturn_if(cmd->id < 0 or static_cast<size_t>(cmd->id) >= instances.size()
          , Unexpected("Instance index out of bounds")
        );
        return ns_subprocess::Subprocess(config.path_dir_app_bin / "fim_portal")
          .with_args("--connect", instances.at(cmd->id))
          .with_args(cmd->args)
          .spawn()
          .wait()
          .value_or(125);
      }
      break;
      case CmdInstanceOp::NONE: return Unexpected("Invalid instance operation");
    }
  } // else if
  else if ( auto cmd = std::get_if<ns_parser::CmdOverlay>(&variant_cmd) )
  {
    switch(cmd->op)
    {
      case ns_parser::CmdOverlayOp::SET:
      {
        Expect(ns_reserved::ns_overlay::write(config.path_file_binary, cmd->overlay));
      }
      break;
      case ns_parser::CmdOverlayOp::SHOW:
      {
        std::println("{}", std::string{config.overlay_type});
      }
      break;
      case ns_parser::CmdOverlayOp::NONE:
      {
        return Unexpected("Invalid operation for fim-overlay");
      }
      break;
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

  return EXIT_SUCCESS;
}

} // namespace ns_parser

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
