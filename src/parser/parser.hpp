/**
 * @file parser.hpp
 * @author Ruan Formigoni
 * @brief Parses FlatImage commands
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <cstdlib>
#include <ranges>
#include <set>
#include <string>
#include <expected>
#include <print>


#include "../macro.hpp"
#include "interface.hpp"
#include "cmd/help.hpp"

namespace ns_parser
{

/**
 * @brief Enumeration of FlatImage command types
 */
enum class FimCommand
{
  NONE,
  EXEC,
  ROOT,
  PERMS,
  ENV,
  DESKTOP,
  LAYER,
  BIND,
  NOTIFY,
  CASEFOLD,
  BOOT,
  INSTANCE,
  OVERLAY,
  VERSION,
  HELP
};

/**
 * @brief Convert string to FimCommand enum
 *
 * @param str Command string (e.g., "fim-exec")
 * @return Expected<FimCommand> The command enum or error
 */
[[nodiscard]] inline Expected<FimCommand> fim_command_from_string(std::string_view str)
{
  if (str == "fim-exec")     return FimCommand::EXEC;
  if (str == "fim-root")     return FimCommand::ROOT;
  if (str == "fim-perms")    return FimCommand::PERMS;
  if (str == "fim-env")      return FimCommand::ENV;
  if (str == "fim-desktop")  return FimCommand::DESKTOP;
  if (str == "fim-layer")    return FimCommand::LAYER;
  if (str == "fim-bind")     return FimCommand::BIND;
  if (str == "fim-notify")   return FimCommand::NOTIFY;
  if (str == "fim-casefold") return FimCommand::CASEFOLD;
  if (str == "fim-boot")     return FimCommand::BOOT;
  if (str == "fim-instance") return FimCommand::INSTANCE;
  if (str == "fim-overlay")  return FimCommand::OVERLAY;
  if (str == "fim-version")  return FimCommand::VERSION;
  if (str == "fim-help")     return FimCommand::HELP;

  return Unexpected("C::Unknown command: {}", str);
}

using namespace ns_parser::ns_interface;

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
    template<ns_string::static_string Format, typename... Ts>
    Expected<std::string> pop_front(Ts&&... ts)
    {
      if(m_data.empty())
      {
        if constexpr (sizeof...(Ts) > 0)
        {
          return Unexpected(Format, ns_string::to_string(ts)...);
        }
        else
        {
          return Unexpected(Format.data);
        }
      }
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

  VecArgs args(argv+1, argv+argc);

  // Get command string and convert to enum
  std::string cmd_str = Expect(args.pop_front<"C::Missing fim- command">());
  FimCommand cmd = Expect(fim_command_from_string(cmd_str));

  switch(cmd)
  {
    case FimCommand::EXEC:
    {
      return CmdType(CmdExec(Expect(args.pop_front<"C::Incorrect number of arguments for fim-exec">())
        ,  args.data()
      ));
    }

    case FimCommand::ROOT:
    {
      return CmdType(CmdRoot(Expect(args.pop_front<"C::Incorrect number of arguments for fim-root">())
        , args.data())
      );
    }

    // Configure permissions for the container
    case FimCommand::PERMS:
    {
      // Get op
      CmdPermsOp op = Expect(
        CmdPermsOp::from_string(Expect(args.pop_front<"C::Missing op for fim-perms (add,del,list,set,clear)">()))
      );
      auto f_process_permissions = [&] -> Expected<std::set<CmdPerms::Permission>>
      {
        std::set<CmdPerms::Permission> permissions;
        for(auto arg : ns_vector::from_string(Expect(args.pop_front<"C::No arguments for '{}' command">(op)), ','))
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
          return Unexpected("C::Invalid operation on permissions");
        }
      }
      // Check for trailing arguments
      qreturn_if(not args.empty(), Unexpected("C::Trailing arguments for fim-perms: {}", args.data()));
      return CmdType{cmd_perms};
    }

    // Configure environment
    case FimCommand::ENV:
    {
      // Get op
      CmdEnvOp op = Expect(
        CmdEnvOp::from_string(Expect(args.pop_front<"C::Missing op for 'fim-env' (add,del,list,set,clear)">()))
      );
      // Gather arguments with key/values if any
      auto f_process_variables = [&] -> Expected<std::vector<std::string>>
      {
        qreturn_if(args.empty(), Unexpected("C::Missing arguments for '{}'", op));
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
          return Unexpected("C::Invalid operation on permissions");
        }
      }
      // Check for trailing arguments
      qreturn_if(not args.empty(), Unexpected("C::Trailing arguments for fim-env: {}", args.data()));
      // Check if is other command with valid args
      return CmdType{cmd_env};
    }

    // Configure desktop
    case FimCommand::DESKTOP:
    {
      // Check if is other command with valid args
      CmdDesktop cmd;
      // Get operation
      CmdDesktopOp op = Expect(CmdDesktopOp::from_string(
        Expect(args.pop_front<"C::Missing op for 'fim-desktop' (enable,setup,clean,dump)">())
      ));
      // Get operation specific arguments
      switch(op)
      {
        case CmdDesktopOp::SETUP:
        {
          cmd.sub_cmd = CmdDesktop::Setup
          {
            .path_file_setup = Expect(args.pop_front<"C::Missing argument from 'setup' (/path/to/file.json)">())
          };
        }
        break;
        case CmdDesktopOp::ENABLE:
        {
          // Get comma separated argument list
          std::vector<std::string> vec_items =
              Expect(args.pop_front<"C::Missing arguments for 'enable' (desktop,entry,mimetype,none)">())
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
            return Unexpected("C::'none' option should not be used with others");
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
            Expect(args.pop_front<"C::Missing arguments for 'dump' (desktop,icon)">())
          ));
          // Parse dump operation
          switch(op_dump)
          {
            case CmdDesktopDump::ICON:
            {
              cmd.sub_cmd = CmdDesktop::Dump { CmdDesktop::Dump::Icon {
                .path_file_icon = Expect(args.pop_front<"C::Missing argument for 'icon' /path/to/dump/file">())
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
            case CmdDesktopDump::NONE: return Unexpected("C::Invalid desktop dump operation");
          }
        }
        break;
        case CmdDesktopOp::CLEAN:
        {
          cmd.sub_cmd = CmdDesktop::Clean{};
        }
        break;
        case CmdDesktopOp::NONE: return Unexpected("C::Invalid desktop operation");
      }
      qreturn_if(not args.empty(), Unexpected("C::Trailing arguments for fim-desktop: {}", args.data()));
      return CmdType(cmd);
    }

    // Manage layers
    case FimCommand::LAYER:
    {
      // Create cmd
      CmdLayer cmd;
      // Get op
      CmdLayerOp op = Expect(
        CmdLayerOp::from_string(Expect(args.pop_front<"C::Missing op for 'fim-layer' (create,add)">()))
      );
      // Process command
      switch(op)
      {
        case CmdLayerOp::ADD:
        {
          constexpr ns_string::static_string error_msg = "C::add requires exactly one argument (/path/to/file.layer)";
          cmd.sub_cmd = CmdLayer::Add
          {
            .path_file_src = (Expect(args.pop_front<error_msg>()))
          };
          qreturn_if(not args.empty(), Unexpected("C::{}", error_msg));
        }
        break;
        case CmdLayerOp::CREATE:
        {
          constexpr ns_string::static_string error_msg = "C::create requires exactly two arguments (/path/to/dir /path/to/file.layer)";
          cmd.sub_cmd = CmdLayer::Create
          {
            .path_dir_src = Expect(args.pop_front<error_msg>()),
            .path_file_target = Expect(args.pop_front<error_msg>())
          };
          qreturn_if(not args.empty(), Unexpected("C::{}", error_msg));
        }
        break;
        case CmdLayerOp::COMMIT:
        {
          cmd.sub_cmd = CmdLayer::Commit{};
        }
        break;
        case CmdLayerOp::NONE: return Unexpected("C::Invalid layer operation");
      }
      return CmdType(cmd);
    }

    // Bind a path or device to inside the flatimage
    case FimCommand::BIND:
    {
      // Create command
      CmdBind cmd;
      // Check op
      CmdBindOp op = Expect(
        CmdBindOp::from_string(Expect(args.pop_front<"C::Missing op for 'fim-bind' command (add,del,list)">()))
      );
      // Process command
      switch(op)
      {
        case CmdBindOp::ADD:
        {
          constexpr ns_string::static_string msg = "C::Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)";
          cmd.sub_cmd = CmdBind::Add
          {
            .type =  Expect(ns_db::ns_bind::Type::from_string(Expect(args.pop_front<msg>()))),
            .path_src = Expect(args.pop_front<msg>()),
            .path_dst = Expect(args.pop_front<msg>())
          };
          qreturn_if(not args.empty(), Unexpected("C::{}", msg));
        }
        break;
        case CmdBindOp::DEL:
        {
          std::string str_index = Expect(args.pop_front<"C::Incorrect number of arguments for 'del' (<index>)">());
          qreturn_if(not std::ranges::all_of(str_index, ::isdigit)
            , Unexpected("C::Index argument for 'del' is not a number")
          );
          cmd.sub_cmd = CmdBind::Del
          {
            .index = std::stoull(str_index)
          };
          qreturn_if(not args.empty(), Unexpected("C::Incorrect number of arguments for 'del' (<index>)"));
        }
        break;
        case CmdBindOp::LIST:
        {
          cmd.sub_cmd = CmdBind::List{};
          qreturn_if(not args.empty(), Unexpected("C::'list' command takes no arguments"));
        }
        break;
        case CmdBindOp::NONE: return Unexpected("C::Invalid operation for bind");
      }
      return CmdType(cmd);
    }

    // Notifies with notify-send when the program starts
    case FimCommand::NOTIFY:
    {
      constexpr ns_string::static_string msg = "C::Incorrect number of arguments for 'fim-notify' (<on|off>)";
      auto cmd_notify = CmdType(CmdNotify{
        Expect(CmdNotifySwitch::from_string(Expect(args.pop_front<msg>())))
      });
      qreturn_if(not args.empty(), Unexpected("C::{}", msg));
      return cmd_notify;
    }

    // Enables or disable ignore case for paths (useful for wine)
    case FimCommand::CASEFOLD:
    {
      constexpr ns_string::static_string msg = "C::Incorrect number of arguments for 'fim-casefold' (<on|off>)";
      qreturn_if(args.empty(), Unexpected("C::{}", msg));
      auto cmd_casefold = CmdType(CmdCaseFold{
        Expect(CmdCaseFoldSwitch::from_string(Expect(args.pop_front<msg>())))
      });
      qreturn_if(not args.empty(), Unexpected("C::Trailing arguments for fim-casefold: {}", args.data()));
      return cmd_casefold;
    }

    // Set the default startup command
    case FimCommand::BOOT:
    {
      // Check op
      CmdBootOp op = Expect(CmdBootOp::from_string(
        Expect(args.pop_front<"C::Invalid operation for 'fim-boot' (<set|show|clear>)">())
      ));
      // Build command
      CmdBoot cmd_boot;
      switch(op)
      {
        case CmdBootOp::SET:
        {
          cmd_boot.sub_cmd = CmdBoot::Set {
            .program = Expect(args.pop_front<"C::Missing program for 'set' operation">()),
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
        case CmdBootOp::NONE: return Unexpected("C::Invalid boot operation");
      }
      // Check for trailing arguments
      qreturn_if(not args.empty(), Unexpected("C::Trailing arguments for fim-boot: {}", args.data()));
      return cmd_boot;
    }

    // Run a command in an existing instance
    case FimCommand::INSTANCE:
    {
      constexpr ns_string::static_string msg = "C::Missing op for 'fim-instance' (<exec|list>)";
      CmdInstanceOp op = Expect(CmdInstanceOp::from_string(Expect(args.pop_front<msg>())));
      CmdInstance cmd;
      switch(op)
      {
        case CmdInstanceOp::EXEC:
        {
          std::string str_id = Expect(args.pop_front<"C::Missing 'id' argument for 'fim-instance'">());
          qreturn_if(not std::ranges::all_of(str_id, ::isdigit), Unexpected("C::Id argument must be a digit"));
          qreturn_if(args.empty(), Unexpected("C::Missing 'cmd' argument for 'fim-instance'"));
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
        case CmdInstanceOp::NONE: return Unexpected("C::Invalid instance operation");
      }
      qreturn_if(not args.empty(), Unexpected("C::Trailing arguments for fim-instance: {}", args.data()));
      return CmdType(cmd);
    }

    // Select or show the current overlay filesystem
    case FimCommand::OVERLAY:
    {
      constexpr ns_string::static_string msg = "C::Missing op for 'fim-overlay' (<set|show>)";
      // Get op
      CmdOverlayOp op = Expect(CmdOverlayOp::from_string(Expect(args.pop_front<msg>())));
      // Build command
      CmdOverlay cmd;
      switch(op)
      {
        case CmdOverlayOp::SET:
        {
          cmd.sub_cmd = CmdOverlay::Set
          {
            .overlay = Expect(ns_reserved::ns_overlay::OverlayType::from_string(           
              Expect(args.pop_front<"C::Missing argument for 'set'">())
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
          return Unexpected("C::Invalid operation for fim-overlay");
        }
      }
      qreturn_if(not args.empty(), Unexpected("C::Trailing arguments for fim-overlay: {}", args.data()));
      return CmdType(cmd);
    }

    // Select or show the current overlay filesystem
    case FimCommand::VERSION:
    {
      constexpr ns_string::static_string msg = "C::Missing op for 'fim-version' (<short|full|deps>)";
      // Get op
      CmdVersionOp op = Expect(CmdVersionOp::from_string(Expect(args.pop_front<msg>())));
      // Build command
      CmdVersion cmd;
      switch(op)
      {
        case CmdVersionOp::SHORT: cmd.sub_cmd = CmdVersion::Short{}; break;
        case CmdVersionOp::FULL: cmd.sub_cmd = CmdVersion::Full{}; break;
        case CmdVersionOp::DEPS: cmd.sub_cmd = CmdVersion::Deps{}; break;
        case CmdVersionOp::NONE: return Unexpected("C::Invalid operation for fim-overlay");
      }
      qreturn_if(not args.empty(), Unexpected("C::Trailing arguments for fim-overlay: {}", args.data()));
      return CmdType(cmd);
    }

    // Use the default startup command
    case FimCommand::HELP:
    {
      if (args.empty())
      {
        std::cerr << ns_cmd::ns_help::help_usage() << '\n';
        return CmdType(CmdExit{});
      }

      std::string help_topic = Expect(args.pop_front<"C::Missing argument for 'fim-help'">());
      std::string message;

      if      (help_topic == "bind")     { message = ns_cmd::ns_help::bind_usage(); }
      else if (help_topic == "boot")     { message = ns_cmd::ns_help::boot_usage(); }
      else if (help_topic == "casefold") { message = ns_cmd::ns_help::casefold_usage(); }
      else if (help_topic == "desktop")  { message = ns_cmd::ns_help::desktop_usage(); }
      else if (help_topic == "env")      { message = ns_cmd::ns_help::env_usage(); }
      else if (help_topic == "exec")     { message = ns_cmd::ns_help::exec_usage(); }
      else if (help_topic == "instance") { message = ns_cmd::ns_help::instance_usage(); }
      else if (help_topic == "layer")    { message = ns_cmd::ns_help::layer_usage(); }
      else if (help_topic == "notify")   { message = ns_cmd::ns_help::notify_usage(); }
      else if (help_topic == "overlay")  { message = ns_cmd::ns_help::overlay_usage(); }
      else if (help_topic == "perms")    { message = ns_cmd::ns_help::perms_usage(); }
      else if (help_topic == "root")     { message = ns_cmd::ns_help::root_usage(); }
      else if (help_topic == "version")  { message = ns_cmd::ns_help::version_usage(); }
      else
      {
        return Unexpected("C::Invalid argument for help command: {}", help_topic);
      }

      std::cout << message;
      return CmdType(CmdExit{});
    }

    case FimCommand::NONE:
    default:
      return Unexpected("C::Unknown command");
  }
}

} // namespace ns_parser

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
