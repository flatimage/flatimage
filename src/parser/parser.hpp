///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : parser
// @created     : Saturday Jan 20, 2024 23:29:45 -03
///

#pragma once

#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <ranges>
#include <set>
#include <string>
#include <expected>
#include <print>


#include "../filesystems/filesystems.hpp"
#include "../db/environment.hpp"
#include "../std/vector.hpp"
#include "../lib/match.hpp"
#include "../macro.hpp"
#include "../reserved/notify.hpp"
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


// parse() {{{
inline Expected<CmdType> parse(int argc , char** argv) noexcept
{
  if ( argc < 2 or not std::string_view{argv[1]}.starts_with("fim-"))
  {
    return CmdNone{};
  } // if

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
  };
  
  VecArgs args(argv+1, argv+argc);

  return Expect(ns_match::match(args.pop_front("Missing fim- command"),
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
      CmdPermsOp op = CmdPermsOp(Expect(args.pop_front("Missing op for fim-perms (add,del,list,set)")));
      qreturn_if(op == CmdPermsOp::NONE, Unexpected("Invalid operation on permissions"));
      // Check if is list
      qreturn_if(op == CmdPermsOp::LIST,  CmdType(CmdPerms{ .op = op, .permissions = {} }));
      // Check if is other command with valid args
      qreturn_if(args.empty(), Unexpected("No arguments for '{}' command"_fmt(op)));
      // Dispatch command
      CmdPerms cmd_perms;
      cmd_perms.op = op;
      cmd_perms.permissions = args.data();
      return CmdType(cmd_perms);
    },
    // Configure environment
    ns_match::equal("fim-env") >>= [&] -> Expected<CmdType>
    {
      // Get op
      CmdEnvOp op = CmdEnvOp(Expect(args.pop_front("Missing op for 'fim-env' (add,del,list,set)")));
      qreturn_if(op == CmdEnvOp::NONE, Unexpected("Invalid operation on environment"));
      // Check if is list
      qreturn_if(op == CmdEnvOp::LIST,  CmdType(CmdEnv{ .op = op, .environment = {} }));
      // Check if is other command with valid args
      qreturn_if(argc < 4, Unexpected("Missing arguments for '{}'"_fmt(op)));
      return CmdType(CmdEnv({
        .op = op,
        .environment = args.data()
      }));
    },
    // Configure environment
    ns_match::equal("fim-desktop") >>= [&] -> Expected<CmdType>
    {
      // Check if is other command with valid args
      CmdDesktop cmd;
      // Get operation
      cmd.op = CmdDesktopOp(Expect(args.pop_front("Missing op for 'fim-desktop' (enable,setup)")));
      qreturn_if( cmd.op == CmdDesktopOp::NONE, Unexpected("Invalid desktop operation"));
      // Get operation specific arguments
      if ( cmd.op == CmdDesktopOp::SETUP )
      {
        cmd.arg = fs::path{Expect(args.pop_front("Missing argument from 'setup' (/path/to/file.json)"))};
      } // if
      else
      {
        // Get comma separated argument list
        std::vector<std::string> vec_items = args.data();
        qreturn_if(vec_items.empty(), Unexpected("Missing arguments for 'enable' (desktop,entry,mimetype)"))
        std::set<ns_desktop::IntegrationItem> set_enum;
        std::ranges::transform(vec_items, std::inserter(set_enum, set_enum.end()), [](auto&& e){ return ns_desktop::IntegrationItem(e); });
        cmd.arg = set_enum;
      } // else
      return CmdType(cmd);
    },
    // Manage layers
    ns_match::equal("fim-layer") >>= [&] -> Expected<CmdType>
    {
      // Create cmd
      CmdLayer cmd;
      // Get op
      cmd.op = CmdLayerOp(Expect(args.pop_front("Missing op for 'fim-layer' (create,add)")));
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
        ns_vector::push_back(cmd.args
          , Expect(args.pop_front(error_msg))
          , Expect(args.pop_front(error_msg))
        );
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
      cmd.op = CmdBindOp(Expect(args.pop_front("Missing op for 'fim-bind' command (add,del,list)")));
      qreturn_if( cmd.op == CmdBindOp::NONE, Unexpected("Invalid bind operation"));
      // Gather operation specific arguments
      using RetType = Expected<CmdBind::cmd_bind_data_t>;
      cmd.data = Expect(Expect(ns_match::match(cmd.op
        , ns_match::equal(CmdBindOp::ADD) >>= [&] -> RetType
        {
          std::string msg = "Incorrect number of arguments for 'add' (<ro,rw,dev> <src> <dst>)";
          CmdBind::cmd_bind_data_t data = CmdBind::cmd_bind_data_t(
            CmdBind::cmd_bind_t(Expect(args.pop_front(msg)),Expect(args.pop_front(msg)),Expect(args.pop_front(msg)))
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
        , ns_match::equal(CmdBindOp::LIST) >>= RetType(CmdBind::cmd_bind_data_t(std::false_type{}))
        , ns_match::equal(CmdBindOp::NONE) >>= RetType(Unexpected("Invalid operation for bind"))
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
        CmdNotifyOp(Expect(args.pop_front(msg)))
      });
      qreturn_if(not args.empty(), Unexpected(msg));
      return cmd_notify;
    },
    // Enables or disable ignore case for paths (useful for wine)
    ns_match::equal("fim-casefold") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Incorrect number of arguments for 'fim-casefold' (<on|off>)";
      auto cmd_casefold = CmdType(CmdCaseFold{
        CmdCaseFoldOp(Expect(args.pop_front(msg)))
      });
      qreturn_if(args.empty(), Unexpected(msg));
      return cmd_casefold;
    },
    // Set the default startup command
    ns_match::equal("fim-boot", "fim-cmd") >>= [&] -> Expected<CmdType>
    {
      return CmdType(CmdBoot(Expect(args.pop_front("Incorrect number of arguments for 'fim-boot' (<program> [args...])"))
        , args.data()
      ));
    },
    // Run a command in an existing instance
    ns_match::equal("fim-instance") >>= [&] -> Expected<CmdType>
    {
      std::string msg = "Missing op for 'fim-instance' (<exec|list>)";
      CmdInstanceOp op(Expect(args.pop_front(msg)));
      // List
      if(op == CmdInstanceOp::LIST)
      {
        return CmdType(CmdInstance(CmdInstanceOp::LIST, -1, {}));
      }
      // Exec
      std::string str_id = Expect(args.pop_front("Missing 'id' argument for 'fim-instance'"));
      qreturn_if(not std::ranges::all_of(str_id, ::isdigit)
        , Unexpected("Id argument must be a digit")
      );
      qreturn_if(args.empty(), Unexpected("Missing 'cmd' argument for 'fim-instance'"));
      return CmdType(CmdInstance(CmdInstanceOp::EXEC
        , std::stoi(str_id)
        , args.data()
      ));
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
        ns_match::equal("exec")     >>=  ns_cmd::ns_help::exec_usage(),
        ns_match::equal("root")     >>=  ns_cmd::ns_help::root_usage(),
        ns_match::equal("perms")    >>=  ns_cmd::ns_help::perms_usage(),
        ns_match::equal("env")      >>=  ns_cmd::ns_help::env_usage(),
        ns_match::equal("desktop")  >>=  ns_cmd::ns_help::desktop_usage(),
        ns_match::equal("layer")    >>=  ns_cmd::ns_help::layer_usage(),
        ns_match::equal("bind")     >>=  ns_cmd::ns_help::bind_usage(),
        ns_match::equal("commit")   >>=  ns_cmd::ns_help::commit_usage(),
        ns_match::equal("notify")   >>=  ns_cmd::ns_help::notify_usage(),
        ns_match::equal("casefold") >>=  ns_cmd::ns_help::casefold_usage(),
        ns_match::equal("boot")     >>=  ns_cmd::ns_help::boot_usage(),
        ns_match::equal("instance") >>=  ns_cmd::ns_help::instance_usage()
      );
      std::cout << message.value_or("Invalid argument for help command") << '\n';
      return CmdType(CmdExit{});
    }
  ));
} // parse() }}}

// parse_cmds() {{{
inline Expected<int> parse_cmds(ns_config::FlatimageConfig& config, int argc, char** argv) noexcept
{
  // Parse args
  CmdType variant_cmd = Expect(ns_parser::parse(argc, argv));

  // Initialize permissions
  ns_bwrap::ns_permissions::Permissions permissions(config.path_file_binary);

  auto f_bwrap_impl = [&](auto&& program, auto&& args)
  {
    // Mount filesystems
    auto mount = ns_filesystems::Filesystems(config);
    // Execute specified command
    auto environment = ns_exception::or_default([&]{ return ns_config::ns_environment::get(config.path_file_config_environment); });
    // Read permissions
    auto bits_permissions = permissions.get();
    elog_if(not bits_permissions, bits_permissions.error());
    // Check if should use bwrap native overlayfs
    std::optional<ns_bwrap::Overlay> bwrap_overlay = ( config.overlay_type == ns_config::OverlayType::BWRAP )?
        std::make_optional(ns_bwrap::Overlay
        {
            .vec_path_dir_layer = ns_config::get_mounted_layers(config.path_dir_mount_layers)
          , .path_dir_upper = config.path_dir_upper_overlayfs
          , .path_dir_work = config.path_dir_work_overlayfs
        })
      : std::nullopt;
    // Create bwrap command
    ns_bwrap::Bwrap bwrap = ns_bwrap::Bwrap(config.is_root
      , bwrap_overlay
      , config.path_dir_mount_overlayfs
      , config.path_file_bashrc
      , program()
      , args()
      , environment);
    // Include root binding and custom user-defined bindings
    std::ignore = bwrap
      .with_bind_ro("/", config.path_dir_runtime_host)
      .with_binds_from_file(config.path_file_config_bindings);
    // Check if should enable GPU
    if ( bits_permissions->gpu )
    {
      std::ignore = bwrap.with_bind_gpu(config.path_dir_upper_overlayfs, config.path_dir_runtime_host);
    }
    // Run bwrap
    return bwrap.run(*bits_permissions, config.path_dir_app_bin);
  };

  auto f_bwrap = [&]<typename T, typename U>(T&& program, U&& args)
  {
    // Run bwrap
    ns_bwrap::bwrap_run_ret_t bwrap_run_ret = f_bwrap_impl(program, args);
    // Log bwrap errors
    elog_if(bwrap_run_ret.errno_nr > 0
      , "Bwrap failed syscall '{}' with errno '{}'"_fmt(bwrap_run_ret.syscall_nr, bwrap_run_ret.errno_nr)
    );
    // Retry with fallback if bwrap overlayfs failed
    if ( config.overlay_type == ns_config::OverlayType::BWRAP and bwrap_run_ret.syscall_nr == SYS_mount )
    {
      ns_log::error()("Bwrap failed SYS_mount, retrying with fuse-unionfs...");
      config.overlay_type = ns_config::OverlayType::FUSE_UNIONFS;
      bwrap_run_ret = f_bwrap_impl(program, args);
    } // if
    return bwrap_run_ret.code;
  };
  
  // Get command
  EnumCmd enum_cmd = EnumCmd::UNDEFINED;
  std::visit([&](auto&& e) { enum_cmd = e.cmd; }, variant_cmd);
  qreturn_if(enum_cmd == EnumCmd::UNDEFINED, Unexpected("Undefined command"));

  // Execute a command as a regular user
  if ( auto cmd = std::get_if<ns_parser::CmdExec>(&variant_cmd) )
  {
    return f_bwrap([&]{ return cmd->program; }, [&]{ return cmd->args; });
  } // if
  // Execute a command as root
  else if ( auto cmd = std::get_if<ns_parser::CmdRoot>(&variant_cmd) )
  {
    config.is_root = true;
    return f_bwrap([&]{ return cmd->program; }, [&]{ return cmd->args; });
  } // if
  // Configure permissions
  else if ( auto cmd = std::get_if<ns_parser::CmdPerms>(&variant_cmd) )
  {
    // Determine open mode
    switch( cmd->op )
    {
      case ns_parser::CmdPermsOp::ADD: Expect(permissions.add(cmd->permissions)); break;
      case ns_parser::CmdPermsOp::SET: Expect(permissions.set(cmd->permissions)); break;
      case ns_parser::CmdPermsOp::DEL: Expect(permissions.del(cmd->permissions)); break;
      case ns_parser::CmdPermsOp::LIST:
        std::ranges::copy(permissions.to_vector_string()
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
      case ns_parser::CmdEnvOp::ADD: ns_config::ns_environment::add(config.path_file_config_environment, cmd->environment); break;
      case ns_parser::CmdEnvOp::SET: ns_config::ns_environment::set(config.path_file_config_environment, cmd->environment); break;
      case ns_parser::CmdEnvOp::DEL: ns_config::ns_environment::del(config.path_file_config_environment, cmd->environment); break;
      case ns_parser::CmdEnvOp::LIST:
        std::ranges::copy(ns_config::ns_environment::get(config.path_file_config_environment)
          , std::ostream_iterator<std::string>(std::cout, "\n")
        );
        break;
      case ns_parser::CmdEnvOp::NONE: return Unexpected("Invalid operation for environment");
    } // switch
  } // if
  // Configure desktop integration
  else if ( auto cmd = std::get_if<ns_parser::CmdDesktop>(&variant_cmd) )
  {
    // Determine open mode
    switch( cmd->op )
    {
      case ns_parser::CmdDesktopOp::ENABLE:
      {
        auto ptr_is_enable = std::get_if<std::set<ns_desktop::IntegrationItem>>(&cmd->arg);
        qreturn_if(ptr_is_enable == nullptr, Unexpected("Could not get items to configure desktop integration"));
        ns_desktop::enable(config,*ptr_is_enable);
      }
      break;
      case ns_parser::CmdDesktopOp::SETUP:
      {
        auto ptr_path_file_src_json = std::get_if<fs::path>(&cmd->arg);
        qreturn_if(ptr_path_file_src_json == nullptr, Unexpected("Could not convert variant value to fs::path"));
        ns_desktop::setup(config, *ptr_path_file_src_json);
      } // case
      break;
      case ns_parser::CmdDesktopOp::NONE: return Unexpected("Invalid desktop operation");
    } // switch
  } // if
  // Manager layers
  else if ( auto cmd = std::get_if<ns_parser::CmdLayer>(&variant_cmd) )
  {
    switch(cmd->op)
    {
      case CmdLayerOp::ADD:
        ns_layers::add(config.path_file_binary, cmd->args.front());
        break;
      case CmdLayerOp::CREATE:
        ns_layers::create(cmd->args.at(0)
          , cmd->args.at(1)
          , config.path_dir_host_config_tmp / "compression.list"
          , config.layer_compression_level
        );
        break;
      case CmdLayerOp::NONE:
        return Unexpected("Invalid desktop operation");
    }
  } // else if
  // Bind a device or file to the flatimage
  else if ( auto cmd = std::get_if<ns_cmd::ns_bind::CmdBind>(&variant_cmd) )
  {
    // Perform selected op
    switch(cmd->op)
    {
      case CmdBindOp::ADD: ns_cmd::ns_bind::add(config.path_file_config_bindings, *cmd); break;
      case CmdBindOp::DEL: ns_cmd::ns_bind::del(config.path_file_config_bindings, *cmd); break;
      case CmdBindOp::LIST: ns_cmd::ns_bind::list(config.path_file_config_bindings); break;
      case CmdBindOp::NONE: return Unexpected("Invalid bind operation");
    } // switch
  } // else if
  // Commit changes as a novel layer into the flatimage
  else if ( auto cmd = std::get_if<ns_parser::CmdCommit>(&variant_cmd) )
  {
    // Set source directory and target compressed file
    fs::path path_file_layer_tmp = config.path_dir_host_config_tmp / "layer.tmp";
    fs::path path_file_list_tmp = config.path_dir_host_config_tmp / "compression.list";
    // Create filesystem based on the contents of src
    ns_layers::create(config.path_dir_upper_overlayfs
      , path_file_layer_tmp
      , path_file_list_tmp
      , config.layer_compression_level
    );
    // Include filesystem in the image
    ns_layers::add(config.path_file_binary, path_file_layer_tmp);
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
    // Open database
    auto db = ns_db::read_file(config.path_file_config_casefold).value_or(ns_db::Db());
    // Update fields
    db("enable") = cmd->op;
    // Write fields
    auto result_write = ns_db::write_file(config.path_file_config_casefold, db);
    qreturn_if(not result_write, Unexpected(result_write.error()));
  } // else if
  // Update default command on database
  else if ( auto cmd = std::get_if<ns_parser::CmdBoot>(&variant_cmd) )
  {
    // Mount filesystem as RW
    [[maybe_unused]] auto mount = ns_filesystems::Filesystems(config);
    // Open database
    auto db = ns_db::read_file(config.path_file_config_boot).value_or(ns_db::Db());
    // Update fields
    db("program") = cmd->program;
    db("args") = cmd->args;
    // Write fields
    auto result_write = ns_db::write_file(config.path_file_config_boot, db);
    qreturn_if(not result_write, Unexpected(result_write.error()));
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
  // Update default command on database
  else if ( auto cmd = std::get_if<ns_parser::CmdNone>(&variant_cmd) )
  {
    // Execute default command
    return f_bwrap([&]
      {
        // Read boot configuration file
        auto db = ns_db::read_file(config.path_file_config_boot);
        dreturn_if(not db, db.error(), std::string{"bash"});
        // Read startup program
        auto program = (*db)("program").template value<std::string>();
        dreturn_if(not program, program.error(), std::string{"bash"});
        // Expand program if it is an environment variable or shell expression
        return ns_env::expand(program.value()).value_or(program.value());
      }
      ,
      [&]
      {
        using Args = std::vector<std::string>;
        // Read boot configuration file
        auto db = ns_db::read_file(config.path_file_config_boot);
        dreturn_if(not db, db.error(), Args{});
        // Read arguments
        auto args = (*db)("args").template value<Args>();
        dreturn_if(not args, args.error(), Args{});
        // Append arguments from argv
        std::copy(argv+1, argv+argc, std::back_inserter(args.value()));
        return args.value();
      }
    );
  } // else if

  return EXIT_SUCCESS;
} // parse_cmds() }}}

} // namespace ns_parser

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
