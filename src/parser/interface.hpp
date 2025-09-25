/**
 * @file interface.hpp
 * @author Ruan Formigoni
 * @brief Interfaces of FlatImage commands
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <set>
#include <string>
#include <vector>

#include "../std/enum.hpp"
#include "cmd/desktop.hpp"
#include "cmd/bind.hpp"
#include "../reserved/permissions.hpp"

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

ENUM(CmdLayerOp,ADD,CREATE);
struct CmdLayer
{
  struct Add
  {
    fs::path path_file_src;
  };
  struct Create
  {
    fs::path path_dir_src;
    fs::path path_file_target;
  };
  std::variant<Add,Create> sub_cmd;
};

ENUM(CmdBindOp,ADD,DEL,LIST);
struct CmdBind
{
  struct Add
  {
    ns_cmd::ns_bind::CmdBindType type;
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

struct CmdCommit
{
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
  , CmdCommit
  , CmdNotify
  , CmdCaseFold
  , CmdBoot
  , CmdInstance
  , CmdOverlay
  , CmdNone
  , CmdExit
>;

}