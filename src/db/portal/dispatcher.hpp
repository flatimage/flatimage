/**
 * @file dispatcher.hpp
 * @author Ruan Formigoni
 * @brief Defines a class that manages FlatImage's portal dispatcher configuration
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <filesystem>
#include <unistd.h>

#include "../../std/expected.hpp"
#include "../../std/filesystem.hpp"
#include "../db.hpp"
#include "daemon.hpp"

namespace ns_db::ns_portal::ns_dispatcher
{

namespace
{
namespace fs = std::filesystem;
} // anonymous namespace

using ns_db::ns_portal::ns_daemon::Mode;

struct Logs
{
  fs::path path_file_log;
  Logs(fs::path const& path_dir_log)
    : path_file_log(path_dir_log / "dispatcher.log")
  {}
};

class Dispatcher
{
  private:
    Mode m_mode;
    fs::path m_path_dir_fifo;
    fs::path m_path_fifo_daemon;
    fs::path m_path_file_log;

    Dispatcher();

  public:
    explicit Dispatcher(pid_t pid, Mode mode, fs::path const& path_dir_app, fs::path const& path_dir_log);

    [[maybe_unused]] fs::path get_path_dir_fifo() const { return m_path_dir_fifo; }
    [[maybe_unused]] fs::path get_path_fifo_daemon() const { return m_path_fifo_daemon; }
    [[maybe_unused]] fs::path get_path_file_log() const { return m_path_file_log; }
    // Serialization
    friend Value<Dispatcher> deserialize(std::string_view str_raw_json) noexcept;
    friend Value<std::string> serialize(Dispatcher const& dispatcher) noexcept;
};

/**
 * @brief Construct a new Dispatcher:: Dispatcher object
 */
inline Dispatcher::Dispatcher()
  : m_mode(Mode::HOST)
  , m_path_dir_fifo()
  , m_path_fifo_daemon()
  , m_path_file_log()
{}

/**
 * @brief Construct a new Dispatcher:: Dispatcher object with paths
 *
 * @param path_dir_fifo The FIFO directory path
 * @param path_fifo_daemon The path to the daemon FIFO
 * @param path_file_log The log file path
 */
inline Dispatcher::Dispatcher(pid_t pid, Mode mode, fs::path const& path_dir_app, fs::path const& path_dir_log)
  : m_mode(mode)
  , m_path_dir_fifo(ns_fs::path_placeholders_replace(path_dir_app  / "instance" / "{}" / "portal" / "fifo", pid))
  , m_path_fifo_daemon(m_path_dir_fifo / std::format("daemon.{}.fifo", mode.lower()))
  , m_path_file_log(path_dir_log / std::format("dispatcher.{}.{}.log", mode.lower(), pid))
{
  fs::create_directories(m_path_file_log.parent_path());
}


/**
 * @brief Deserializes a json string into a `Dispatcher` class
 *
 * @param str_raw_json The json string which to deserialize
 * @return The `Dispatcher` class or the respective error
 */
[[maybe_unused]] [[nodiscard]] inline Value<Dispatcher> deserialize(std::string_view str_raw_json) noexcept
{
  Dispatcher dispatcher;
  qreturn_if(str_raw_json.empty(), Error("D::Empty json data"));
  // Open DB
  auto db = Pop(ns_db::from_string(str_raw_json));
  // Parse path_dir_fifo
  dispatcher.m_path_dir_fifo = Pop(db("path_dir_fifo").template value<std::string>());
  // Parse path_fifo_daemon
  dispatcher.m_path_fifo_daemon = Pop(db("path_fifo_daemon").template value<std::string>());
  // Parse path_file_log
  dispatcher.m_path_file_log = Pop(db("path_file_log").template value<std::string>());
  return dispatcher;
}

/**
 * @brief Serializes a `Dispatcher` class into a json string
 *
 * @param dispatcher The `Dispatcher` object to serialize
 * @return The serialized json data
 */
[[maybe_unused]] [[nodiscard]] inline Value<std::string> serialize(Dispatcher const& dispatcher) noexcept
{
  auto db = ns_db::Db();
  db("path_dir_fifo") = dispatcher.get_path_dir_fifo().string();
  db("path_fifo_daemon") = dispatcher.get_path_fifo_daemon().string();
  db("path_file_log") = dispatcher.get_path_file_log().string();
  return db.dump();
}

} // namespace ns_db::ns_portal::ns_dispatcher

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
