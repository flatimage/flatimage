/**
 * @file interface.hpp
 * @author Ruan Formigoni
 * @brief Interfaces of FlatImage commands
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <vector>

#include "../std/enum.hpp"
#include "cmd/desktop.hpp"

namespace ns_parser::ns_interface
{

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

ENUM(CmdPermsOp,SET,ADD,DEL,LIST,CLEAR);
struct CmdPerms
{
  CmdPermsOp op;
  std::vector<std::string> permissions;
};

ENUM(CmdEnvOp,SET,ADD,DEL,LIST);
struct CmdEnv
{
  CmdEnvOp op;
  std::vector<std::string> environment;
};

ENUM(CmdDesktopOp,SETUP,ENABLE);
struct CmdDesktop
{
  CmdDesktopOp op;
  std::variant<std::filesystem::path,std::set<ns_desktop::IntegrationItem>> arg;
};

struct CmdBoot
{
  std::string program;
  std::vector<std::string> args;
};

ENUM(CmdLayerOp,CREATE,ADD);
struct CmdLayer
{
  CmdLayerOp op;
  std::vector<std::string> args;
};

ENUM(CmdBindOp,ADD,DEL,LIST);
ENUM(CmdBindType,RO,RW,DEV);
struct CmdBind
{
  using cmd_bind_index_t = int64_t;
  using cmd_bind_t = struct { CmdBindType type; std::string src; std::string dst; };
  using cmd_bind_data_t = std::variant<cmd_bind_index_t,cmd_bind_t,std::false_type>;
  CmdBindOp op;
  cmd_bind_data_t data;
};

struct CmdCommit
{
};

ENUM(CmdNotifyOp,ON,OFF);
struct CmdNotify
{
  CmdNotifyOp op;
};

ENUM(CmdCaseFoldOp,ON,OFF);
struct CmdCaseFold
{
  CmdCaseFoldOp op;
};

ENUM(CmdInstanceOp,LIST,EXEC);
struct CmdInstance
{
  CmdInstanceOp op;
  int32_t id;
  std::vector<std::string> args;
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
  , CmdNone
  , CmdExit
>;

}