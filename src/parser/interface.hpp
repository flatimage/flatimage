///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : interface
///

#pragma once

#include <string>
#include <vector>

#include "../std/enum.hpp"
#include "cmd/desktop.hpp"

namespace ns_parser::ns_interface
{

enum class EnumCmd
{
  ROOT,
  EXEC,
  PERMS,
  ENV,
  DESKTOP,
  BOOT,
  LAYER,
  BIND,
  COMMIT,
  NOTIFY,
  CASEFOLD,
  INSTANCE,
  NONE,
  EXIT,
  UNDEFINED,
};

// Cmds {{{
struct CmdRoot
{
  constexpr static EnumCmd cmd = EnumCmd::ROOT;
  std::string program;
  std::vector<std::string> args;
};

struct CmdExec
{
  constexpr static EnumCmd cmd = EnumCmd::EXEC;
  std::string program;
  std::vector<std::string> args;
};

ENUM(CmdPermsOp,SET,ADD,DEL,LIST);
struct CmdPerms
{
  constexpr static EnumCmd cmd = EnumCmd::PERMS;
  CmdPermsOp op;
  std::vector<std::string> permissions;
};

ENUM(CmdEnvOp,SET,ADD,DEL,LIST);
struct CmdEnv
{
  constexpr static EnumCmd cmd = EnumCmd::ENV;
  CmdEnvOp op;
  std::vector<std::string> environment;
};

ENUM(CmdDesktopOp,SETUP,ENABLE);
struct CmdDesktop
{
  constexpr static EnumCmd cmd = EnumCmd::DESKTOP;
  CmdDesktopOp op;
  std::variant<std::filesystem::path,std::set<ns_desktop::IntegrationItem>> arg;
};

struct CmdBoot
{
  constexpr static EnumCmd cmd = EnumCmd::BOOT;
  std::string program;
  std::vector<std::string> args;
};

ENUM(CmdLayerOp,CREATE,ADD);
struct CmdLayer
{
  constexpr static EnumCmd cmd = EnumCmd::LAYER;
  CmdLayerOp op;
  std::vector<std::string> args;
};

ENUM(CmdBindOp,ADD,DEL,LIST);
ENUM(CmdBindType,RO,RW,DEV);
struct CmdBind
{
  using cmd_bind_index_t = uint64_t;
  using cmd_bind_t = struct { CmdBindType type; std::string src; std::string dst; };
  using cmd_bind_data_t = std::variant<cmd_bind_index_t,cmd_bind_t,std::false_type>;
  constexpr static EnumCmd cmd = EnumCmd::BIND;
  CmdBindOp op;
  cmd_bind_data_t data;
};

struct CmdCommit
{
  constexpr static EnumCmd cmd = EnumCmd::COMMIT;
};

ENUM(CmdNotifyOp,ON,OFF);
struct CmdNotify
{
  constexpr static EnumCmd cmd = EnumCmd::NOTIFY;
  CmdNotifyOp op;
};

ENUM(CmdCaseFoldOp,ON,OFF);
struct CmdCaseFold
{
  constexpr static EnumCmd cmd = EnumCmd::CASEFOLD;
  CmdCaseFoldOp op;
};

ENUM(CmdInstanceOp,LIST,EXEC);
struct CmdInstance
{
  constexpr static EnumCmd cmd = EnumCmd::INSTANCE;
  CmdInstanceOp op;
  int32_t id;
  std::vector<std::string> args;
};

struct CmdNone
{
  constexpr static EnumCmd cmd = EnumCmd::NONE;
};

struct CmdExit
{
  constexpr static EnumCmd cmd = EnumCmd::EXIT;
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
  , CmdNone
  , CmdExit
>;

}