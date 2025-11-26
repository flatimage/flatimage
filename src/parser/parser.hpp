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

/**
 * @namespace ns_parser
 * @brief FlatImage command parsing and execution engine
 *
 * Parses and executes FlatImage commands (fim-exec, fim-perms, fim-env, fim-desktop, etc.).
 * Handles argument parsing, command dispatch, and orchestrates the execution flow for all
 * user-facing FlatImage operations including boot, permissions, bindings, layers, recipes,
 * and desktop integration.
 */
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
  REMOTE,
  RECIPE,
  INSTANCE,
  OVERLAY,
  VERSION,
  HELP
};

/**
 * @brief Convert string to FimCommand enum
 *
 * @param str Command string (e.g., "fim-exec")
 * @return Value<FimCommand> The command enum or error
 */
[[nodiscard]] inline Value<FimCommand> fim_command_from_string(std::string_view str)
{
  if (str == "fim-bind")     return FimCommand::BIND;
  if (str == "fim-boot")     return FimCommand::BOOT;
  if (str == "fim-casefold") return FimCommand::CASEFOLD;
  if (str == "fim-desktop")  return FimCommand::DESKTOP;
  if (str == "fim-env")      return FimCommand::ENV;
  if (str == "fim-exec")     return FimCommand::EXEC;
  if (str == "fim-help")     return FimCommand::HELP;
  if (str == "fim-instance") return FimCommand::INSTANCE;
  if (str == "fim-layer")    return FimCommand::LAYER;
  if (str == "fim-notify")   return FimCommand::NOTIFY;
  if (str == "fim-overlay")  return FimCommand::OVERLAY;
  if (str == "fim-perms")    return FimCommand::PERMS;
  if (str == "fim-recipe")   return FimCommand::RECIPE;
  if (str == "fim-remote")   return FimCommand::REMOTE;
  if (str == "fim-root")     return FimCommand::ROOT;
  if (str == "fim-version")  return FimCommand::VERSION;

  return Error("C::Unknown command: {}", str);
}

using namespace ns_parser::ns_interface;

/**
 * @brief Vector-based argument container with pop operations
 *
 * Wraps a vector of strings to provide convenient argument parsing
 * with formatted error messages.
 */
class VecArgs
{
  private:
    std::vector<std::string> m_data;
  public:
    /**
     * @brief Constructs a VecArgs from an iterator range
     * @param begin Beginning of the argument range
     * @param end End of the argument range
     */
    VecArgs(char** begin, char** end)
    {
      if(begin != end)
      {
        m_data = std::vector<std::string>(begin,end);
      }
    }

    /**
     * @brief Pops the front element with formatted error message
     * @tparam Format Error message format string
     * @tparam Ts Types of format arguments
     * @param ts Format arguments
     * @return Value containing the popped string or error
     */
    template<ns_string::static_string Format, typename... Ts>
    Value<std::string> pop_front(Ts&&... ts)
    {
      if(m_data.empty())
      {
        if constexpr (sizeof...(Ts) > 0)
        {
          return Error(Format, ns_string::to_string(ts)...);
        }
        else
        {
          return Error(Format.data);
        }
      }
      std::string item = m_data.front();
      m_data.erase(m_data.begin());
      return item;
    }

    /**
     * @brief Returns a const reference to the underlying data
     * @return Const reference to the vector of strings
     */
    std::vector<std::string> const& data()
    {
      return m_data;
    }

    /**
     * @brief Returns the number of arguments
     * @return Size of the argument vector
     */
    size_t size()
    {
      return m_data.size();
    }

    /**
     * @brief Checks if the argument vector is empty
     * @return True if empty, false otherwise
     */
    bool empty()
    {
      return m_data.empty();
    }

    /**
     * @brief Clears all arguments
     */
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
 * @return Value<CmdType> The parsed command or the respective error
 */
[[nodiscard]] inline Value<CmdType> parse(int argc , char** argv)
{
  if ( argc < 2 or not std::string_view{argv[1]}.starts_with("fim-"))
  {
    return CmdNone{};
  }

  VecArgs args(argv+1, argv+argc);

  // Get command string and convert to enum
  std::string cmd_str = Pop(args.pop_front<"C::Missing fim- command">());
  FimCommand cmd = Pop(fim_command_from_string(cmd_str), "C::Invalid fim command");

  switch(cmd)
  {
    case FimCommand::EXEC:
    {
      return CmdType(CmdExec(Pop(args.pop_front<"C::Incorrect number of arguments for fim-exec">())
        ,  args.data()
      ));
    }

    case FimCommand::ROOT:
    {
      return CmdType(CmdRoot(Pop(args.pop_front<"C::Incorrect number of arguments for fim-root">())
        , args.data())
      );
    }

    // Configure permissions for the container
    case FimCommand::PERMS:
    {
      // Get op
      CmdPermsOp op = Pop(
        CmdPermsOp::from_string(Pop(args.pop_front<"C::Missing op for fim-perms (add,del,list,set,clear)">())), "C::Invalid perms operation"
      );
      auto f_process_permissions = [&] -> Value<std::set<CmdPerms::Permission>>
      {
        std::set<CmdPerms::Permission> permissions;
        for(auto arg : ns_vector::from_string(Pop(args.pop_front<"C::No arguments for '{}' command">(op)), ','))
        {
          permissions.insert(Pop(CmdPerms::Permission::from_string(arg), "C::Invalid permission"));
        }
        return permissions;
      };
      // Process Ops
      CmdPerms cmd_perms;
      switch(op)
      {
        case CmdPermsOp::SET:
        {
          cmd_perms.sub_cmd = CmdPerms::Set { .permissions = Pop(f_process_permissions(), "C::Failed to process permissions") };
        }
        break;
        case CmdPermsOp::ADD:
        {
          cmd_perms.sub_cmd = CmdPerms::Add { .permissions = Pop(f_process_permissions(), "C::Failed to process permissions") };
        }
        break;
        case CmdPermsOp::DEL:
        {
          cmd_perms.sub_cmd = CmdPerms::Del { .permissions = Pop(f_process_permissions(), "C::Failed to process permissions") };
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
          return Error("C::Invalid operation for permissions");
        }
      }
      // Check for trailing arguments
      return_if(not args.empty(), Error("C::Trailing arguments for fim-perms: {}", args.data()));
      return CmdType{cmd_perms};
    }

    // Configure environment
    case FimCommand::ENV:
    {
      // Get op
      CmdEnvOp op = Pop(
        CmdEnvOp::from_string(Pop(args.pop_front<"C::Missing op for 'fim-env' (add,del,list,set,clear)">())), "C::Invalid env operation"
      );
      // Gather arguments with key/values if any
      auto f_process_variables = [&] -> Value<std::vector<std::string>>
      {
        return_if(args.empty(), Error("C::Missing arguments for '{}'", op));
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
          cmd_env.sub_cmd = CmdEnv::Set { .variables = Pop(f_process_variables(), "C::Failed to process variables") };
        }
        break;
        case CmdEnvOp::ADD:
        {
          cmd_env.sub_cmd = CmdEnv::Add { .variables = Pop(f_process_variables(), "C::Failed to process variables") };
        }
        break;
        case CmdEnvOp::DEL:
        {
          cmd_env.sub_cmd = CmdEnv::Del { .variables = Pop(f_process_variables(), "C::Failed to process variables") };
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
          return Error("C::Invalid operation for environment");
        }
      }
      // Check for trailing arguments
      return_if(not args.empty(), Error("C::Trailing arguments for fim-env: {}", args.data()));
      // Check if is other command with valid args
      return CmdType{cmd_env};
    }

    // Configure desktop
    case FimCommand::DESKTOP:
    {
      // Check if is other command with valid args
      CmdDesktop cmd;
      // Get operation
      CmdDesktopOp op = Pop(CmdDesktopOp::from_string(
        Pop(args.pop_front<"C::Missing op for 'fim-desktop' (enable,setup,clean,dump)">())), "C::Invalid desktop operation"
      );
      // Get operation specific arguments
      switch(op)
      {
        case CmdDesktopOp::SETUP:
        {
          cmd.sub_cmd = CmdDesktop::Setup
          {
            .path_file_setup = Pop(args.pop_front<"C::Missing argument for 'setup' (/path/to/file.json)">())
          };
        }
        break;
        case CmdDesktopOp::ENABLE:
        {
          // Get comma separated argument list
          std::vector<std::string> vec_items =
              Pop(args.pop_front<"C::Missing arguments for 'enable' (entry,mimetype,icon,none)">())
            | std::views::split(',')
            | std::ranges::to<std::vector<std::string>>();
          // Create items
          std::set<ns_desktop::IntegrationItem> set_enable;
          for(auto&& item : vec_items)
          {
            set_enable.insert(Pop(ns_desktop::IntegrationItem::from_string(item), "C::Invalid integration item"));
          }
          // Check for 'none'
          if(set_enable.size() > 1 and set_enable.contains(ns_desktop::IntegrationItem::NONE))
          {
            return Error("C::'none' option should not be used with others");
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
          CmdDesktopDump op_dump = Pop(CmdDesktopDump::from_string(
            Pop(args.pop_front<"C::Missing arguments for 'dump' (entry,mimetype,icon)">())
          ), "C::Invalid dump operation");
          // Parse dump operation
          switch(op_dump)
          {
            case CmdDesktopDump::ICON:
            {
              cmd.sub_cmd = CmdDesktop::Dump { CmdDesktop::Dump::Icon {
                .path_file_icon = Pop(args.pop_front<"C::Missing argument for 'icon' /path/to/dump/file">())
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
            case CmdDesktopDump::NONE: return Error("C::Invalid desktop dump operation");
          }
        }
        break;
        case CmdDesktopOp::CLEAN:
        {
          cmd.sub_cmd = CmdDesktop::Clean{};
        }
        break;
        case CmdDesktopOp::NONE: return Error("C::Invalid desktop operation");
      }
      return_if(not args.empty(), Error("C::Trailing arguments for fim-desktop: {}", args.data()));
      return CmdType(cmd);
    }

    // Manage layers
    case FimCommand::LAYER:
    {
      // Create cmd
      CmdLayer cmd;
      // Get op
      CmdLayerOp op = Pop(
        CmdLayerOp::from_string(Pop(args.pop_front<"C::Missing op for 'fim-layer' (create,add,commit)">())), "C::Invalid layer operation"
      );
      // Process command
      switch(op)
      {
        case CmdLayerOp::ADD:
        {
          constexpr ns_string::static_string error_msg = "C::add requires exactly one argument (/path/to/file.layer)";
          cmd.sub_cmd = CmdLayer::Add
          {
            .path_file_src = (Pop(args.pop_front<error_msg>()))
          };
          return_if(not args.empty(), Error("C::{}", error_msg));
        }
        break;
        case CmdLayerOp::CREATE:
        {
          constexpr ns_string::static_string error_msg = "C::create requires exactly two arguments (/path/to/dir /path/to/file.layer)";
          cmd.sub_cmd = CmdLayer::Create
          {
            .path_dir_src = Pop(args.pop_front<error_msg>()),
            .path_file_target = Pop(args.pop_front<error_msg>())
          };
          return_if(not args.empty(), Error("C::{}", error_msg));
        }
        break;
        case CmdLayerOp::COMMIT:
        {
          CmdLayerCommitOp commit_op = Pop(
              CmdLayerCommitOp::from_string(Pop(args.pop_front<"C::Missing op for 'commit' (binary,layer,file)">()))
            , "C::Invalid commit operation"
          );
          CmdLayer::Commit cmd_commit;
          switch(commit_op)
          {
            case CmdLayerCommitOp::BINARY:
            {
              cmd_commit.sub_cmd = CmdLayer::Commit::Binary{};
            }
            break;
            case CmdLayerCommitOp::LAYER:
            {
              cmd_commit.sub_cmd = CmdLayer::Commit::Layer{};
            }
            break;
            case CmdLayerCommitOp::FILE:
            {
              cmd_commit.sub_cmd = CmdLayer::Commit::File{
                .path_file_dst = Pop(args.pop_front<"C::Missing path for 'file' operation">())
              };
            }
            break;
            case CmdLayerCommitOp::NONE: return Error("C::Invalid commit operation");
          }
          return_if(not args.empty(), Error("C::Trailing arguments for fim-layer commit: {}", args.data()));
          cmd.sub_cmd = cmd_commit;
        }
        break;
        case CmdLayerOp::NONE: return Error("C::Invalid layer operation");
      }
      return CmdType(cmd);
    }

    // Bind a path or device to inside the flatimage
    case FimCommand::BIND:
    {
      // Create command
      CmdBind cmd;
      // Check op
      CmdBindOp op = Pop(
        CmdBindOp::from_string(Pop(args.pop_front<"C::Missing op for 'fim-bind' command (add,del,list)">())) , "C::Invalid bind operation"
      );
      // Process command
      switch(op)
      {
        case CmdBindOp::ADD:
        {
          constexpr ns_string::static_string msg = "C::Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)";
          cmd.sub_cmd = CmdBind::Add
          {
            .type =  Pop(ns_db::ns_bind::Type::from_string(Pop(args.pop_front<msg>())), "C::Invalid bind type"),
            .path_src = Pop(args.pop_front<msg>()),
            .path_dst = Pop(args.pop_front<msg>())
          };
          return_if(not args.empty(), Error("C::{}", msg));
        }
        break;
        case CmdBindOp::DEL:
        {
          std::string str_index = Pop(args.pop_front<"C::Incorrect number of arguments for 'del' (<index>)">());
          return_if(not std::ranges::all_of(str_index, ::isdigit)
            , Error("C::Index argument for 'del' is not a number")
          );
          cmd.sub_cmd = CmdBind::Del
          {
            .index = Try(std::stoull(str_index), "C::Invalid index")
          };
          return_if(not args.empty(), Error("C::Incorrect number of arguments for 'del' (<index>)"));
        }
        break;
        case CmdBindOp::LIST:
        {
          cmd.sub_cmd = CmdBind::List{};
          return_if(not args.empty(), Error("C::'list' command takes no arguments"));
        }
        break;
        case CmdBindOp::NONE: return Error("C::Invalid operation for bind");
      }
      return CmdType(cmd);
    }

    // Notifies with notify-send when the program starts
    case FimCommand::NOTIFY:
    {
      constexpr ns_string::static_string msg = "C::Incorrect number of arguments for 'fim-notify' (<on|off>)";
      auto cmd_notify = CmdType(CmdNotify{
        Pop(CmdNotifySwitch::from_string(Pop(args.pop_front<msg>())), "C::Invalid notify switch")
      });
      return_if(not args.empty(), Error("C::{}", msg));
      return cmd_notify;
    }

    // Enables or disable ignore case for paths (useful for wine)
    case FimCommand::CASEFOLD:
    {
      constexpr ns_string::static_string msg = "C::Incorrect number of arguments for 'fim-casefold' (<on|off>)";
      return_if(args.empty(), Error("C::{}", msg));
      auto cmd_casefold = CmdType(CmdCaseFold{
        Pop(CmdCaseFoldSwitch::from_string(Pop(args.pop_front<msg>())), "C::Invalid casefold switch")
      });
      return_if(not args.empty(), Error("C::Trailing arguments for fim-casefold: {}", args.data()));
      return cmd_casefold;
    }

    // Set the default startup command
    case FimCommand::BOOT:
    {
      // Check op
      CmdBootOp op = Pop(CmdBootOp::from_string(
        Pop(args.pop_front<"C::Missing op for 'fim-boot' (<set|show|clear>)">())
      ), "C::Invalid boot operation");
      // Build command
      CmdBoot cmd_boot;
      switch(op)
      {
        case CmdBootOp::SET:
        {
          cmd_boot.sub_cmd = CmdBoot::Set {
            .program = Pop(args.pop_front<"C::Missing program for 'set' operation">()),
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
        case CmdBootOp::NONE: return Error("C::Invalid boot operation");
      }
      // Check for trailing arguments
      return_if(not args.empty(), Error("C::Trailing arguments for fim-boot: {}", args.data()));
      return cmd_boot;
    }

    // Set, show, or clear the remote URL
    case FimCommand::REMOTE:
    {
      // Check op
      CmdRemoteOp op = Pop(CmdRemoteOp::from_string(
        Pop(args.pop_front<"C::Missing op for 'fim-remote' (<set|show|clear>)">())
      ), "C::Invalid remote operation");
      // Build command
      CmdRemote cmd_remote;
      switch(op)
      {
        case CmdRemoteOp::SET:
        {
          cmd_remote.sub_cmd = CmdRemote::Set {
            .url = Pop(args.pop_front<"C::Missing URL for 'set' operation">())
          };
        }
        break;
        case CmdRemoteOp::SHOW:
        {
          cmd_remote.sub_cmd = CmdRemote::Show{};
        }
        break;
        case CmdRemoteOp::CLEAR:
        {
          cmd_remote.sub_cmd = CmdRemote::Clear{};
        }
        break;
        case CmdRemoteOp::NONE: return Error("C::Invalid remote operation");
      }
      // Check for trailing arguments
      return_if(not args.empty(), Error("C::Trailing arguments for fim-remote: {}", args.data()));
      return cmd_remote;
    }

    // Fetch and install recipes
    case FimCommand::RECIPE:
    {
      // Check op
      CmdRecipeOp op = Pop(CmdRecipeOp::from_string(
        Pop(args.pop_front<"C::Missing op for 'fim-recipe' (<fetch|info|install>)">())
      ), "C::Invalid recipe operation");
      // Build command
      CmdRecipe cmd_recipe;
      auto f_parse_recipes = [](auto& args) -> Value<std::vector<std::string>>
      {
          std::vector<std::string> recipes = Pop(args.template pop_front<"C::Missing recipe for operation">())
            | std::views::split(',')
            | std::ranges::to<std::vector<std::string>>();
          return_if(recipes.empty(), Error("C::Recipe argument is empty"));
          return recipes;
      };
      switch(op)
      {
        case CmdRecipeOp::FETCH:
        {
          cmd_recipe.sub_cmd = CmdRecipe::Fetch { .recipes = Pop(f_parse_recipes(args), "C::Invalid recipes") };
        }
        break;
        case CmdRecipeOp::INFO:
        {
          cmd_recipe.sub_cmd = CmdRecipe::Info { .recipes = Pop(f_parse_recipes(args), "C::Invalid recipes") };
        }
        break;
        case CmdRecipeOp::INSTALL:
        {
          cmd_recipe.sub_cmd = CmdRecipe::Install { .recipes = Pop(f_parse_recipes(args), "C::Invalid recipes") };
        }
        break;
        case CmdRecipeOp::NONE: return Error("C::Invalid recipe operation");
      }
      // Check for trailing arguments
      return_if(not args.empty(), Error("C::Trailing arguments for fim-recipe: {}", args.data()));
      return cmd_recipe;
    }

    // Run a command in an existing instance
    case FimCommand::INSTANCE:
    {
      constexpr ns_string::static_string msg = "C::Missing op for 'fim-instance' (<exec|list>)";
      CmdInstanceOp op = Pop(CmdInstanceOp::from_string(Pop(args.pop_front<msg>())), "C::Invalid instance operation");
      CmdInstance cmd;
      switch(op)
      {
        case CmdInstanceOp::EXEC:
        {
          std::string str_id = Pop(args.pop_front<"C::Missing 'id' argument for 'fim-instance'">());
          return_if(not std::ranges::all_of(str_id, ::isdigit), Error("C::Id argument must be a digit"));
          return_if(args.empty(), Error("C::Missing 'cmd' argument for 'fim-instance'"));
          cmd.sub_cmd = CmdInstance::Exec
          {
            .id = Try(std::stoi(str_id), "C::Invalid instance ID"),
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
        case CmdInstanceOp::NONE: return Error("C::Invalid instance operation");
      }
      return_if(not args.empty(), Error("C::Trailing arguments for fim-instance: {}", args.data()));
      return CmdType(cmd);
    }

    // Select or show the current overlay filesystem
    case FimCommand::OVERLAY:
    {
      constexpr ns_string::static_string msg = "C::Missing op for 'fim-overlay' (<set|show>)";
      // Get op
      CmdOverlayOp op = Pop(CmdOverlayOp::from_string(Pop(args.pop_front<msg>())), "C::Invalid overlay operation");
      // Build command
      CmdOverlay cmd;
      switch(op)
      {
        case CmdOverlayOp::SET:
        {
          cmd.sub_cmd = CmdOverlay::Set
          {
            .overlay = Pop(ns_reserved::ns_overlay::OverlayType::from_string(
              Pop(args.pop_front<"C::Missing argument for 'set'">())
            ), "C::Invalid overlay type")
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
          return Error("C::Invalid operation for fim-overlay");
        }
      }
      return_if(not args.empty(), Error("C::Trailing arguments for fim-overlay: {}", args.data()));
      return CmdType(cmd);
    }

    // Select or show the current overlay filesystem
    case FimCommand::VERSION:
    {
      constexpr ns_string::static_string msg = "C::Missing op for 'fim-version' (<short|full|deps>)";
      // Get op
      CmdVersionOp op = Pop(CmdVersionOp::from_string(Pop(args.pop_front<msg>())), "C::Invalid version operation");
      // Build command
      CmdVersion cmd;
      switch(op)
      {
        case CmdVersionOp::SHORT: cmd.sub_cmd = CmdVersion::Short{}; break;
        case CmdVersionOp::FULL: cmd.sub_cmd = CmdVersion::Full{}; break;
        case CmdVersionOp::DEPS: cmd.sub_cmd = CmdVersion::Deps{}; break;
        case CmdVersionOp::NONE: return Error("C::Invalid operation for fim-version");
      }
      return_if(not args.empty(), Error("C::Trailing arguments for fim-version: {}", args.data()));
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

      std::string help_topic = Pop(args.pop_front<"C::Missing argument for 'fim-help'">());
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
      else if (help_topic == "recipe")   { message = ns_cmd::ns_help::recipe_usage(); }
      else if (help_topic == "remote")   { message = ns_cmd::ns_help::remote_usage(); }
      else if (help_topic == "root")     { message = ns_cmd::ns_help::root_usage(); }
      else if (help_topic == "version")  { message = ns_cmd::ns_help::version_usage(); }
      else
      {
        return Error("C::Invalid argument for help command: {}", help_topic);
      }

      std::cout << message;
      return CmdType(CmdExit{});
    }

    case FimCommand::NONE:
    default:
      return Error("C::Unknown command");
  }
}

} // namespace ns_parser

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
