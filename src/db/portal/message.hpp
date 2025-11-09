/**
 * @file message.hpp
 * @author Ruan Formigoni
 * @brief Defines a class that manages portal daemon message serialization/deserialization
 *
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "../../std/expected.hpp"
#include "../../std/filesystem.hpp"
#include "../db.hpp"

namespace ns_db::ns_portal::ns_message
{

namespace
{
namespace fs = std::filesystem;
} // anonymous namespace

class Message
{
  private:
    std::vector<std::string> m_command;
    fs::path m_stdin;
    fs::path m_stdout;
    fs::path m_stderr;
    fs::path m_exit;
    fs::path m_pid;
    fs::path m_log;
    std::vector<std::string> m_environment;

    Message();

  public:
    Message(pid_t pid
      , std::vector<std::string> const& command
      , fs::path const& path_dir_fifo
      , fs::path const& path_file_log
      , std::vector<std::string> const& environment
    );

    [[maybe_unused]] std::vector<std::string> const& get_command() const { return m_command; }
    [[maybe_unused]] fs::path get_stdin() const { return m_stdin; }
    [[maybe_unused]] fs::path get_stdout() const { return m_stdout; }
    [[maybe_unused]] fs::path get_stderr() const { return m_stderr; }
    [[maybe_unused]] fs::path get_exit() const { return m_exit; }
    [[maybe_unused]] fs::path get_pid() const { return m_pid; }
    [[maybe_unused]] fs::path const& get_log() const { return m_log; }
    [[maybe_unused]] std::vector<std::string> const& get_environment() const { return m_environment; }

    friend Value<Message> deserialize(std::string_view str_raw_json) noexcept;
    friend Value<std::string> serialize(Message const& message) noexcept;
};

/**
 * @brief Construct a new Message:: Message object
 */
inline Message::Message()
  : m_command()
  , m_stdin()
  , m_stdout()
  , m_stderr()
  , m_exit()
  , m_pid()
  , m_log()
  , m_environment()
{}


/**
 * @brief Construct a new Message:: Message object
 */
inline Message::Message(pid_t pid
    , std::vector<std::string> const& command
    , fs::path const& path_dir_fifo
    , fs::path const& path_file_log
    , std::vector<std::string> const& environment
  ) : m_command(command)
    , m_stdin(ns_fs::placeholders_replace(path_dir_fifo / "{}" / "stdin.fifo", pid))
    , m_stdout(ns_fs::placeholders_replace(path_dir_fifo / "{}" / "stdout.fifo", pid))
    , m_stderr(ns_fs::placeholders_replace(path_dir_fifo / "{}" / "stderr.fifo", pid))
    , m_exit(ns_fs::placeholders_replace(path_dir_fifo / "{}" / "exit.fifo", pid))
    , m_pid(ns_fs::placeholders_replace(path_dir_fifo / "{}" / "pid.fifo", pid))
    , m_log(path_file_log)
    , m_environment(environment)
{}

/**
 * @brief Deserializes a json string into a `Message` class
 *
 * @param str_raw_json The json string which to deserialize
 * @return The `Message` class or the respective error
 */
[[maybe_unused]] [[nodiscard]] inline Value<Message> deserialize(std::string_view str_raw_json) noexcept
{
  Message message;
  qreturn_if(str_raw_json.empty(), Error("D::Empty json data"));
  // Open DB
  auto db = Pop(ns_db::from_string(str_raw_json));
  // Parse command (required)
  message.m_command = Pop(db("command").template value<std::vector<std::string>>());
  // Parse FIFO paths (required)
  message.m_stdin = Pop(db("stdin").template value<std::string>());
  message.m_stdout = Pop(db("stdout").template value<std::string>());
  message.m_stderr = Pop(db("stderr").template value<std::string>());
  message.m_exit = Pop(db("exit").template value<std::string>());
  message.m_pid = Pop(db("pid").template value<std::string>());
  // Parse log file path (required)
  message.m_log = Pop(db("log").template value<std::string>());
  // Parse environment variables (required)
  message.m_environment = Pop(db("environment").template value<std::vector<std::string>>());
  return message;
}

/**
 * @brief Serializes a `Message` class into a json string
 *
 * @param message The `Message` object to serialize
 * @return The serialized json data;
 */
[[maybe_unused]] [[nodiscard]] inline Value<std::string> serialize(Message const& message) noexcept
{
  auto db = ns_db::Db();
  db("command") = message.m_command;
  db("stdin") = message.get_stdin().string();
  db("stdout") = message.get_stdout().string();
  db("stderr") = message.get_stderr().string();
  db("exit") = message.get_exit().string();
  db("pid") = message.get_pid().string();
  db("log") = message.get_log().string();
  db("environment") = message.get_environment();
  return db.dump();
}

} // namespace ns_db::ns_portal::ns_message

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
