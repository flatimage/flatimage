/**
 * @file portal.hpp
 * @author Ruan Formigoni
 * @brief Defines a class that manages FlatImage's portal configuration
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <filesystem>
#include <unistd.h>

#include "../../std/expected.hpp"
#include "../../std/enum.hpp"
#include "../db.hpp"

namespace ns_db::ns_portal::ns_daemon
{

namespace
{
namespace fs = std::filesystem;
} // anonymous namespace

namespace ns_log
{

class Logs
{
  private:
    fs::path m_path_dir_log;
    fs::path m_path_file_parent;
    fs::path m_path_file_child;
    fs::path m_path_file_grand;

    Logs();

  public:
    explicit Logs(fs::path const& path_dir_log);

    [[maybe_unused]] fs::path const& get_path_dir_log() const { return m_path_dir_log; }
    [[maybe_unused]] fs::path const& get_path_file_parent() const { return m_path_file_parent; }
    [[maybe_unused]] fs::path const& get_path_file_child() const { return m_path_file_child; }
    [[maybe_unused]] fs::path const& get_path_file_grand() const { return m_path_file_grand; }

    friend Value<Logs> deserialize(std::string_view str_raw_json) noexcept;
    friend Value<std::string> serialize(Logs const& logs) noexcept;
};

/**
 * @brief Construct a new Logs:: Logs object
 */
inline Logs::Logs()
  : m_path_dir_log()
  , m_path_file_parent()
  , m_path_file_child()
  , m_path_file_grand()
{}

/**
 * @brief Construct a new Logs:: Logs object with a path
 *
 * @param path_dir_log The log directory path
 */
inline Logs::Logs(fs::path const& path_dir_log)
  : m_path_dir_log(path_dir_log)
  , m_path_file_parent(path_dir_log / "parent.log")
  , m_path_file_child(path_dir_log / "child.log")
  , m_path_file_grand(path_dir_log / "grand.log")
{
  fs::create_directories(m_path_file_parent.parent_path());
  fs::create_directories(m_path_file_child.parent_path());
  fs::create_directories(m_path_file_grand.parent_path());
}


/**
 * @brief Deserializes a json string into a `Logs` class
 *
 * @param str_raw_json The json string which to deserialize
 * @return The `Logs` class or the respective error
 */
[[maybe_unused]] [[nodiscard]] inline Value<Logs> deserialize(std::string_view str_raw_json) noexcept
{
  return_if(str_raw_json.empty(), Error("D::Empty json data"));
  // Open DB
  auto db = Pop(ns_db::from_string(str_raw_json));
  // Parse log directory path (optional)
  fs::path path_dir_log = Pop(db("path_dir_log").template value<std::string>());
  // Create Logs object with the parsed path
  Logs logs(path_dir_log);
  // Parse log file paths (optional)
  logs.m_path_file_parent = Pop(db("path_file_parent").template value<std::string>());
  logs.m_path_file_child = Pop(db("path_file_child").template value<std::string>());
  logs.m_path_file_grand = Pop(db("path_file_grand").template value<std::string>());
  return logs;
}

/**
 * @brief Serializes a `Logs` class into a json string
 *
 * @param logs The `Logs` object to serialize
 * @return The serialized json data
 */
[[maybe_unused]] [[nodiscard]] inline Value<std::string> serialize(Logs const& logs) noexcept
{
  auto db = ns_db::Db();
  db("path_dir_log") = logs.m_path_dir_log.string();
  db("path_file_parent") = logs.m_path_file_parent.string();
  db("path_file_child") = logs.m_path_file_child.string();
  db("path_file_grand") = logs.m_path_file_grand.string();
  return db.dump();
}

} // namespace ns_log


// daemon mode enum
ENUM(Mode, HOST, GUEST);

class Daemon
{
  private:
    Mode m_mode;
    pid_t m_pid_reference;
    fs::path m_path_bin_daemon;
    fs::path path_fifo_listen;

    Daemon();

  public:
    Daemon(Mode mode, fs::path const& path_bin_daemon, fs::path const& path_dir_fifo);

    [[maybe_unused]] pid_t get_pid_reference() const { return m_pid_reference; }
    [[maybe_unused]] fs::path const& get_path_bin_daemon() const { return m_path_bin_daemon; }
    [[maybe_unused]] fs::path const& get_path_fifo_listen() const { return path_fifo_listen; }
    [[maybe_unused]] Mode get_mode() const { return m_mode; }

    friend Value<Daemon> deserialize(std::string_view str_raw_json) noexcept;
    friend Value<std::string> serialize(Daemon const& daemon) noexcept;
};

/**
 * @brief Construct a new Daemon:: Daemon object
 */
inline Daemon::Daemon()
  : m_mode(Mode::HOST)
  , m_pid_reference(0)
  , m_path_bin_daemon()
  , path_fifo_listen()
{}

/**
 * @brief Construct a new Daemon:: Daemon object
 */
inline Daemon::Daemon(Mode mode, fs::path const& path_bin_daemon, fs::path const& path_dir_fifo)
  : m_mode(mode)
  , m_pid_reference(getpid())
  , m_path_bin_daemon(path_bin_daemon)
  , path_fifo_listen(path_dir_fifo / std::format("daemon.{}.fifo", m_mode.lower()))
{}

/**
 * @brief Deserializes a json string into a `Daemon` class
 *
 * @param str_raw_json The json string which to deserialize
 * @return The `Daemon` class or the respective error
 */
[[maybe_unused]] [[nodiscard]] inline Value<Daemon> deserialize(std::string_view str_raw_json) noexcept
{
  Daemon daemon;
  return_if(str_raw_json.empty(), Error("D::Empty json data"));
  // Open DB
  auto db = Pop(ns_db::from_string(str_raw_json));
  // Parse pid_reference (optional, defaults to current PID)
  std::string pid_reference = Pop(db("pid_reference").template value<std::string>());
  daemon.m_pid_reference = Try(std::stoi(pid_reference));
  // Parse path_bin_daemon (optional, defaults to empty path)
  daemon.m_path_bin_daemon = Pop(db("path_bin_daemon").template value<std::string>());
  // Parse paths (optional, defaults to empty paths)
  daemon.path_fifo_listen = Pop(db("path_fifo_listen").template value<std::string>());
  // Parse mode (optional, defaults to Host)
  daemon.m_mode = Pop(Mode::from_string(Pop(db("mode").template value<std::string>())));
  return daemon;
}

/**
 * @brief Serializes a `Daemon` class into a json string
 *
 * @param daemon The `Daemon` object to serialize
 * @return The serialized json data;
 */
[[maybe_unused]] [[nodiscard]] inline Value<std::string> serialize(Daemon const& daemon) noexcept
{
  auto db = ns_db::Db();
  db("pid_reference") = std::to_string(daemon.m_pid_reference);
  db("path_bin_daemon") = daemon.m_path_bin_daemon.string();
  db("path_fifo_listen") = daemon.path_fifo_listen.string();
  db("mode") = std::string(daemon.m_mode);
  return db.dump();
}

} // namespace ns_db::ns_portal::ns_daemon

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
