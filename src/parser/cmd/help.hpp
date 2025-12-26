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

/**
 * @namespace ns_cmd
 * @brief FlatImage command implementations
 *
 * Contains implementations for all FlatImage commands (fim-bind, fim-help, fim-desktop, etc.).
 * Each command is organized in its own nested namespace providing specific functionality for
 * bind mounts, help text, desktop integration, filesystem layers, and package recipes.
 */

/**
 * @namespace ns_cmd::ns_help
 * @brief Help system and command documentation
 *
 * Provides comprehensive help text for all FlatImage commands including usage patterns,
 * option descriptions, and examples. Generates formatted help output for individual commands
 * (fim-exec, fim-perms, etc.) and general FlatImage usage information.
 */
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
      m_msg += std::string{"  <"} + e.first + "> : " + e.second + '\n';
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
    .with_note("Available commands: fim-{bind,boot,casefold,desktop,env,exec,instance,layer,notify,overlay,perms,recipe,remote,root,unshare,version}")
    .with_example(R"(fim-help bind)")
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
    .with_usage("fim-desktop <clean>")
    .with_args({
      { "clean", "Cleans the desktop integration files from XDG_DATA_HOME" },
    })
    .with_usage("fim-desktop <dump> <icon> <file>")
    .with_args({
      { "dump", "Dumps the selected integration data" },
      { "icon", "Dumps the desktop icon to a file" },
      { "file", "Path to the icon file, the extension is appended automatically if not specified" },
    })
    .with_usage("fim-desktop <dump> <entry|mimetype>")
    .with_args({
      { "dump", "Dumps the selected integration data" },
      { "entry", "The desktop entry of the application" },
      { "mimetype", "The mime type of the application" },
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
    .with_usage("fim-env <del> <keys...>")
    .with_args({
      { "del", "Delete one or more environment variables" },
      { "keys...", "List of variable names to delete" },
    })
    .with_example("fim-env del APP_NAME HOME")
    .with_usage("fim-env <list>")
    .with_args({
      { "list", "Lists configured environment variables" },
    })
    .with_usage("fim-env <clear>")
    .with_args({
      { "clear", "Clears configured environment variables" },
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
      { "id" , "ID of the instance in which to execute the command" },
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
    .with_usage("fim-layer <commit> <binary|layer|file> [path]")
    .with_args({
      { "commit", "Compresses current changes into a layer" },
      { "binary", "Appends the layer to the FlatImage binary" },
      { "layer", "Saves the layer to $FIM_DIR_DATA/layers with auto-increment naming" },
      { "file", "Saves the layer to the specified file path" },
      { "path", "File path (required when using 'file' mode)" },
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

inline std::string overlay_usage()
{
  return HelpEntry{"fim-overlay"}
    .with_description("Show or select the default overlay filesystem")
    .with_usage("fim-overlay <set> <overlayfs|unionfs|bwrap>")
    .with_args({
      { "set", "Sets the default overlay filesystem to use" },
      { "overlayfs", "Uses 'fuse-overlayfs' as the overlay filesystem" },
      { "unionfs", "Uses 'unionfs-fuse' as the overlay filesystem" },
      { "bwrap", "Uses 'bubblewrap' native overlay options as the overlay filesystem" },
    })
    .with_usage("fim-overlay <show>")
    .with_args({
      { "show", "Shows the current overlay filesystem" },
    })
    .get();
}

inline std::string perms_usage()
{
  return HelpEntry{"fim-perms"}
    .with_description("Edit current permissions for the flatimage")
    .with_note("Permissions: all,audio,dbus_system,dbus_user,dev,gpu,home,input,media,network,optical,shm,udev,usb,wayland,xorg")
    .with_usage("fim-perms <add|del|set> <perms...>")
    .with_args({
      { "add", "Allow one or more permissions" },
      { "del", "Delete one or more permissions" },
      { "set", "Replace all permissions with the specified set" },
      { "perms...", "One or more permissions" },
    })
    .with_example("fim-perms add home,network,gpu")
    .with_example("fim-perms set wayland,audio,network")
    .with_note("The 'all' permission sets all available permissions and cannot be combined with other permissions")
    .with_usage("fim-perms <list|clear>")
    .with_args({
      { "list", "Lists the current permissions" },
      { "clear", "Clears all permissions" },
    })
    .get();
}

inline std::string remote_usage()
{
  return HelpEntry{"fim-remote"}
    .with_description("Configure the remote URL for recipes")
    .with_usage("fim-remote <set> <url>")
    .with_args({
      { "set", "Set the remote URL" },
      { "url", "The remote URL to configure" },
    })
    .with_example("fim-remote set https://updates.example.com/repo")
    .with_usage("fim-remote <show>")
    .with_args({
      { "show", "Display the current remote URL" },
    })
    .with_usage("fim-remote <clear>")
    .with_args({
      { "clear", "Clear the configured remote URL" },
    })
    .get();
}

inline std::string recipe_usage()
{
  return HelpEntry{"fim-recipe"}
    .with_description("Fetch, inspect, and install recipes from a remote repository")
    .with_usage("fim-recipe <fetch> <recipes>")
    .with_args({
      { "fetch", "Download one or more recipes with their dependencies without installing packages" },
      { "recipes", "Name(s) of the recipe(s) to download (comma-separated for multiple)" },
    })
    .with_note("Recipes and all dependencies are downloaded from URL/DISTRO/latest/<recipe>.json to path_dir_host_config/recipes/DISTRO/latest/<recipe>.json")
    .with_example(R"(fim-recipe fetch gpu)")
    .with_example(R"(fim-recipe fetch gpu,audio,xorg)")
    .with_usage("fim-recipe <info> <recipes>")
    .with_args({
      { "info", "Display information about one or more locally cached recipes including dependencies" },
      { "recipes", "Name(s) of the recipe(s) to inspect (comma-separated for multiple)" },
    })
    .with_example(R"(fim-recipe info gpu)")
    .with_example(R"(fim-recipe info gpu,audio,xorg)")
    .with_usage("fim-recipe <install> <recipes>")
    .with_args({
      { "install", "Download recipes with dependencies, validate no cycles exist, and install all packages" },
      { "recipes", "Name(s) of the recipe(s) to install (comma-separated for multiple)" },
    })
    .with_note("The remote URL must be configured using 'fim-remote set <url>'")
    .with_note("Dependencies are resolved recursively and cyclic dependencies are detected")
    .with_example(R"(fim-recipe install gpu)")
    .with_example(R"(fim-recipe install gpu,audio,xorg)")
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

inline std::string unshare_usage()
{
  return HelpEntry{"fim-unshare"}
    .with_description("Configure namespace unsharing options for isolation")
    .with_note("Unshare options: all,user,ipc,pid,net,uts,cgroup")
    .with_note("USER and CGROUP use '-try' variants in bubblewrap for permissiveness")
    .with_usage("fim-unshare <add|del|set> <options...>")
    .with_args({
      { "add", "Enable one or more unshare options" },
      { "del", "Remove one or more unshare options" },
      { "set", "Replace all unshare options with the specified set" },
      { "options...", "One or more unshare options (comma-separated)" },
    })
    .with_example("fim-unshare add ipc,pid")
    .with_example("fim-unshare set user,ipc,net")
    .with_note("The 'all' option enables all available unshare options and cannot be combined with others")
    .with_usage("fim-unshare <list|clear>")
    .with_args({
      { "list", "Lists the current unshare options" },
      { "clear", "Clears all unshare options" },
    })
    .get();
}

inline std::string version_usage()
{
  return HelpEntry{"fim-version"}
    .with_description("Displays version information of FlatImage")
    .with_usage("fim-version <short|full|deps>")
    .with_args({
      { "short", "Displays the version as a string" },
      { "full", "Displays the version and build information in json" },
      { "deps", "Displays dependencies metadata in json" },
    })
    .get();
}

} // namespace ns_cmd::ns_help

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
