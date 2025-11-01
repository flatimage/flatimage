/**
 * @file interface.hpp
 * @author Ruan Formigoni
 * @brief Interfaces of FlatImage commands
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "../reserved/permissions.hpp"
#include "../std/enum.hpp"
#include "../db/bind.hpp"
#include "cmd/desktop.hpp"

namespace ns_parser::ns_interface
{
  
namespace
{

namespace fs = std::filesystem;

}

struct CmdRoot
{
  std::string program;
  std::vector<std::string> args;
};

struct CmdExec
{
  std::string program;
  std::vector<std::string> args;
};

ENUM(CmdPermsOp,ADD,CLEAR,DEL,LIST,SET);
struct CmdPerms
{
  using Permission = ns_reserved::ns_permissions::Permission;
  struct Add
  {
    std::set<Permission> permissions;
  };
  struct Clear
  {
  };
  struct Del
  {
    std::set<Permission> permissions;
  };
  struct List
  {
  };
  struct Set
  {
    std::set<Permission> permissions;
  };
  std::variant<Add,Clear,Del,List,Set> sub_cmd;
};

ENUM(CmdEnvOp,ADD,CLEAR,DEL,LIST,SET);
struct CmdEnv
{
  struct Add
  {
    std::vector<std::string> variables;
  };
  struct Clear
  {
  };
  struct Del
  {
    std::vector<std::string> variables;
  };
  struct List
  {
  };
  struct Set
  {
    std::vector<std::string> variables;
  };
  std::variant<Add,Clear,Del,List,Set> sub_cmd;
};

ENUM(CmdDesktopOp,CLEAN,DUMP,ENABLE,SETUP);
ENUM(CmdDesktopDump,ENTRY,ICON,MIMETYPE);
struct CmdDesktop
{
  struct Clean
  {
  };
  struct Dump
  {
    struct Icon
    {
      fs::path path_file_icon;
    };
    struct Entry
    {
    };
    struct MimeType
    {
    };
    std::variant<Icon,Entry,MimeType> sub_cmd;
  };
  struct Enable
  {
    std::set<ns_desktop::IntegrationItem> set_enable;
  };
  struct Setup
  {
    std::filesystem::path path_file_setup;
  };
  std::variant<Clean,Dump,Enable,Setup> sub_cmd;
};

ENUM(CmdBootOp,SET,SHOW,CLEAR);
struct CmdBoot
{
  struct Clear
  {
  };
  struct Set
  {
    std::string program;
    std::vector<std::string> args;
  };
  struct Show
  {
  };
  std::variant<Clear,Set,Show> sub_cmd;
};

ENUM(CmdRemoteOp,SET,SHOW,CLEAR);
struct CmdRemote
{
  struct Clear
  {
  };
  struct Set
  {
    std::string url;
  };
  struct Show
  {
  };
  std::variant<Clear,Set,Show> sub_cmd;
};

ENUM(CmdRecipeOp,FETCH,INFO,INSTALL);
struct CmdRecipe
{
  struct Fetch
  {
    std::vector<std::string> recipes;
  };
  struct Info
  {
    std::vector<std::string> recipes;
  };
  struct Install
  {
    std::vector<std::string> recipes;
  };
  std::variant<Fetch,Info,Install> sub_cmd;
};

ENUM(CmdLayerOp,ADD,COMMIT,CREATE);
struct CmdLayer
{
  struct Add
  {
    fs::path path_file_src;
  };
  struct Commit
  {
  };
  struct Create
  {
    fs::path path_dir_src;
    fs::path path_file_target;
  };
  std::variant<Add,Commit,Create> sub_cmd;
};

ENUM(CmdBindOp,ADD,DEL,LIST);
struct CmdBind
{
  struct Add
  {
    ns_db::ns_bind::Type type;
    fs::path path_src;
    fs::path path_dst; 
  };
  struct Del
  {
    uint64_t index;
  };
  struct List
  {
  };
  std::variant<Add,Del,List> sub_cmd;
};

ENUM(CmdNotifySwitch,ON,OFF);
struct CmdNotify
{
  CmdNotifySwitch status;
};

ENUM(CmdCaseFoldSwitch,ON,OFF);
struct CmdCaseFold
{
  CmdCaseFoldSwitch status;
};

ENUM(CmdInstanceOp,EXEC,LIST);
struct CmdInstance
{
  struct Exec
  {
    int32_t id;
    std::vector<std::string> args;
  };
  struct List
  {
  };
  std::variant<Exec,List> sub_cmd;
};

ENUM(CmdOverlayOp,SET,SHOW);
struct CmdOverlay
{
  struct Set
  {
    ns_reserved::ns_overlay::OverlayType overlay;
  };
  struct Show
  {
  };
  std::variant<Set,Show> sub_cmd;
};

ENUM(CmdVersionOp,SHORT,FULL,DEPS);
struct CmdVersion
{
  struct Short
  {
    std::string dump()
    {
      return FIM_VERSION;
    }
  };
  struct Full
  {
    std::string dump()
    {
      ns_db::Db db;
      db("VERSION") = FIM_VERSION;
      db("COMMIT") = FIM_COMMIT;
      db("DISTRIBUTION") = FIM_DIST;
      db("TIMESTAMP") = FIM_TIMESTAMP;
      return db.dump();
    }
  };
  struct Deps
  {
    // This placeholder is replaced before compiling
    Expected<std::string> dump()
    {
      constexpr static char str_raw_json[] =
      {
        #embed FIM_FILE_META
      };
      return Expect(ns_db::from_string(str_raw_json)).dump();
    }
  };
  std::variant<Short,Full,Deps> sub_cmd;
};

struct CmdNone
{
};

struct CmdExit
{
};

using CmdType = std::variant<CmdRoot
  , CmdExec
  , CmdPerms
  , CmdEnv
  , CmdDesktop
  , CmdLayer
  , CmdBind
  , CmdNotify
  , CmdCaseFold
  , CmdBoot
  , CmdRemote
  , CmdRecipe
  , CmdInstance
  , CmdOverlay
  , CmdNone
  , CmdExit
  , CmdVersion
>;

}