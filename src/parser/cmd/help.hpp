/**
 * @file help.hpp
 * @author Ruan Formigoni
 * @brief Help strings for FlatImage commands
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace ns_cmd::ns_help
{

class HelpEntry
{
  private:
    std::string m_msg;
    std::string m_name;
  public:
    HelpEntry(std::string const& name)
      : m_msg("Flatimage - Portable Linux Applications\n")
      , m_name(name)
    {};
  HelpEntry& with_usage(std::string_view usage)
  {
    m_msg.append("Usage: ").append(usage).append("\n");
    return *this;
  }
  HelpEntry& with_example(std::string_view example)
  {
    m_msg.append("Example: ").append(example).append("\n");
    return *this;
  }
  HelpEntry& with_note(std::string_view note)
  {
    m_msg.append("Note: ").append(note).append("\n");
    return *this;
  }
  HelpEntry& with_description(std::string_view description)
  {
    m_msg.append(m_name).append(" : ").append(description).append("\n");
    return *this;
  }
  HelpEntry& with_args(std::vector<std::pair<std::string,std::string>> args)
  {
    std::ranges::for_each(args, [&](auto&& e)
    {
      m_msg += std::string{"   <"} + e.first + "> : " + e.second + '\n';
    });
    return *this;
  }
  std::string get()
  {
    return m_msg;
  }
};

inline std::string help_usage()
{
  return HelpEntry{"fim-help"}
    .with_description("See usage details for specified command")
    .with_usage("fim-help <cmd>")
    .with_args({
      { "cmd", "Name of the command to display help details" },
    })
    .with_note("Available commands: fim-{bind,boot,casefold,commit,desktop,env,exec,instance,layer,notify,perms,root,version}")
    .with_example(R"(fim-help bind")")
    .get();
}

inline std::string bind_usage()
{
  return HelpEntry{"fim-bind"}
    .with_description("Bind paths from the host to inside the container")
    .with_usage("fim-bind <add> <type> <src> <dst>")
    .with_args({
      { "add", "Create a novel binding of type <type> from <src> to <dst>" },
      { "type", "ro, rw, dev" },
      { "src" , "A file, directory, or device" },
      { "dst" , "A file, directory, or device" },
    })
    .with_usage("fim-bind <del> <index>")
    .with_args({
      { "del", "Deletes a binding with the specified index" },
      { "index" , "Index of the binding to erase" },
    })
    .with_usage("fim-bind <list>")
    .with_args({
      { "list", "Lists current bindings"}
    })
    .get();
}

inline std::string boot_usage()
{
  return HelpEntry{"fim-boot"}
    .with_description("Configure the default startup command")
    .with_usage("fim-boot <set> <command> [args...]")
    .with_args({
      { "set", "Execute <command> with optional [args] when FlatImage is launched" },
      { "command" , "Startup command" },
      { "args..." , "Arguments for the startup command" },
    })
    .with_example(R"(fim-boot set echo test)")
    .with_usage("fim-boot <show|clear>")
    .with_args({
      { "show" , "Displays the current startup command" },
      { "clear" , "Clears the set startup command" },
    })
    .get();
}

inline std::string casefold_usage()
{
  return HelpEntry{"fim-casefold"}
    .with_description("Enables casefold for the filesystem (ignore case)")
    .with_usage("fim-casefold <on|off>")
    .with_args({
      { "on", "Enables casefold" },
      { "off", "Disables casefold" },
    })
    .get();
}

inline std::string commit_usage()
{
  return HelpEntry{"fim-commit"}
    .with_description("Compresses current changes and inserts them into the FlatImage")
    .with_usage("fim-commit")
    .get();
}

inline std::string desktop_usage()
{
  return HelpEntry{"fim-desktop"}
    .with_description("Configure the desktop integration")
    .with_usage("fim-desktop <setup> <json-file>")
    .with_args({
      { "setup", "Sets up the desktop integration with an input json file" },
      { "json-file", "Path to the json file with the desktop configuration"},
    })
    .with_usage("fim-desktop <enable> [entry,mimetype,icon,none]")
    .with_args({
      { "enable", "Enables the desktop integration selectively" },
      { "entry", "Enables the start menu desktop entry"},
      { "mimetype", "Enables the mimetype"},
      { "icon", "Enables the icon for the file manager and desktop entry"},
      { "none", "Disables desktop integrations"},
    })
    .get();
}

inline std::string env_usage()
{
  return HelpEntry{"fim-env"}
    .with_description("Define environment variables in FlatImage")
    .with_usage("fim-env <add|set> <'key=value'...>")
    .with_args({
      { "add", "Include a novel environment variable" },
      { "set", "Redefines the environment variables as the input arguments" },
      { "'key=value'...", "List of variables to add or set" },
    })
    .with_example("fim-env add 'APP_NAME=hello-world' 'HOME=/home/my-app'")
    .with_usage("fim-env <clear>")
    .with_args({
      { "clear", "Clears configured environment variables" },
    })
    .with_usage("fim-env <list>")
    .with_args({
      { "list", "Lists configured environment variables" },
    })
    .get();
}

inline std::string exec_usage()
{
  return HelpEntry{"fim-exec"}
    .with_description("Executes a command as a regular user")
    .with_usage("fim-exec <program> [args...]")
    .with_args({
      { "program", "Name of the program to execute, it can be the name of a binary or the full path" },
      { "args...", "Arguments for the executed program" },
    })
    .with_example(R"(fim-exec echo -e "hello\nworld")")
    .get();
}

inline std::string instance_usage()
{
  return HelpEntry{"fim-instance"}
    .with_description("Manage running instances")
    .with_usage("fim-instance <exec> <id> [args...]")
    .with_args({
      { "exec", "Run a command in a running instance" },
      { "id" , "id of the instance in which to execute the command" },
      { "args" , "Arguments for the 'exec' command" },
    })
    .with_example("fim-instance exec 0 echo hello")
    .with_usage("fim-instance <list>")
    .with_args({
      { "list", "Lists current instances" },
    })
    .get();
}

inline std::string layer_usage()
{
  return HelpEntry{"fim-layer"}
    .with_description("Manage the layers of the current FlatImage")
    .with_usage("fim-layer <create> <in-dir> <out-file>")
    .with_args({
      { "create", "Creates a novel layer from <in-dir> and save in <out-file>" },
      { "in-dir", "Input directory to create a novel layer from"},
      { "out-file" , "Output file name of the layer file"},
    })
    .with_usage("fim-layer <add> <in-file>")
    .with_args({
      { "add", "Includes the novel layer <in-file> in the image in the top of the layer stack" },
      { "in-file", "Path to the layer file to include in the FlatImage"},
    })
    .get();
}

inline std::string notify_usage()
{
  return HelpEntry{"fim-notify"}
    .with_description("Notify with 'notify-send' when the program starts")
    .with_usage("fim-notify <on|off>")
    .with_args({
      { "on", "Turns on notify-send to signal the application start" },
      { "off", "Turns off notify-send to signal the application start" },
    })
    .get();
}

inline std::string perms_usage()
{
  return HelpEntry{"fim-perms"}
    .with_description("Edit current permissions for the flatimage")
    .with_note("Permissions: home,media,audio,wayland,xorg,dbus_user,dbus_system,udev,usb,input,gpu,network")
    .with_usage("fim-perms <add|del> <perms...>")
    .with_args({
      { "add", "Allow one or more permissions" },
      { "del", "Delete one or more permissions" },
      { "perms...", "One or more permissions" },
    })
    .with_example("fim-perms add home,network,gpu")
    .with_usage("fim-perms <list|clear>")
    .with_args({
      { "list", "Lists the current permissions" },
      { "clear", "Clears all permissions" },
    })
    .get();
}

inline std::string root_usage()
{
  return HelpEntry{"fim-root"}
    .with_description("Executes a command as the root user")
    .with_usage("fim-root <program> [args...]")
    .with_args({
      { "program", "Name of the program to execute, it can be the name of a binary or the full path" },
      { "args...", "Arguments for the executed program" },
    })
    .with_example(R"(fim-root id -u)")
    .get();
}

inline std::string version_usage()
{
  return HelpEntry{"fim-version"}
    .with_description("Displays version information of FlatImage")
    .with_usage("fim-version")
    .with_usage("fim-version-full")
    .with_args({
      { "fim-version", "Displays the version information of the FlatImage distribution" },
      { "fim-version-full", "Displays a detailed json output of the FlatImage distribution" },
    })
    .get();
}

} // namespace ns_cmd::ns_help

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
