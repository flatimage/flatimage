///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : portal_guest
///


#include <chrono>
#include <csignal>
#include <cstdlib>
#include <expected>
#include <fcntl.h>
#include <filesystem>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

#include "../cpp/macro.hpp"
#include "../cpp/lib/env.hpp"
#include "../cpp/lib/log.hpp"
#include "../cpp/lib/db.hpp"
#include "../cpp/lib/ipc.hpp"
#include "../cpp/lib/linux.hpp"
#include "fifo.hpp"
#include "config.hpp"

extern char** environ;

namespace fs = std::filesystem;

std::optional<pid_t> opt_child = std::nullopt;

// signal_handler() {{{
void signal_handler(int sig)
{
  if(opt_child)
  {
    kill(opt_child.value(), sig);
  }
} // signal_handler() }}}

[[nodiscard]] std::expected<void, std::string> set_environment(fs::path const& path_file_env)
{
  qreturn_if(std::error_code ec; (fs::create_directories(path_file_env.parent_path(), ec), ec)
    , std::unexpected(std::format("Could not open file '{}': '{}'", path_file_env.string(), ec.message()))
  );
  std::ofstream ofile_env(path_file_env);
  qreturn_if(not ofile_env.good(), std::unexpected("Could not open file '{}'"_fmt(path_file_env)));
  for(char **env = environ; *env != NULL; ++env) { ofile_env << *env << '\n'; } // for
  ofile_env.close();
  return {};
}

[[nodiscard]] std::expected<void, std::string> send_message(fs::path const& path_daemon_fifo
  , std::vector<char*> command
  , std::unordered_map<std::string, fs::path> const& hash_name_fifo
  , fs::path const& path_file_log
  , fs::path const& path_file_env)
{
  ns_log::debug()("Sending message through pipe: {}", path_daemon_fifo);
  // Create command
  auto db = ns_db::Db();
  db("command") = command;
  db("stdin") = hash_name_fifo.at("stdin");
  db("stdout") = hash_name_fifo.at("stdout");
  db("stderr") = hash_name_fifo.at("stderr");
  db("exit") = hash_name_fifo.at("exit");
  db("pid") = hash_name_fifo.at("pid");
  db("log") = path_file_log.c_str();
  db("environment") = path_file_env;
  // Get json string
  std::string data = db.dump();
  ns_log::debug()(data);
  // Write to fifo
  ssize_t size_writen = ns_linux::open_write_with_timeout(path_daemon_fifo
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span(data.c_str(), data.length())
  );
  // Check for errors
  return (static_cast<size_t>(size_writen) != data.length())?
      std::unexpected("Could not write data to daemon({}): {}"_fmt(size_writen, strerror(errno)))
    : std::expected<void,std::string>{};
}

[[nodiscard]] std::expected<int, std::string> process_wait(std::unordered_map<std::string, fs::path> const& hash_name_fifo)
{
  pid_t pid_child;
  ssize_t bytes_read = ns_linux::open_read_with_timeout(hash_name_fifo.at("pid")
    , std::chrono::seconds(SECONDS_TIMEOUT)
    , std::span<pid_t>(&pid_child, 1)
  );
  qreturn_if(bytes_read != sizeof(pid_child), std::unexpected(strerror(errno)));
  opt_child = pid_child;
  ns_log::debug()("Child pid: {}", pid_child);
  // Connect to stdin, stdout, and stderr with fifos
  pid_t pid_stdin = redirect_fd_to_fifo(pid_child, STDIN_FILENO, hash_name_fifo.at("stdin"));
  pid_t pid_stdout = redirect_fifo_to_fd(pid_child, hash_name_fifo.at("stdout"), STDOUT_FILENO);
  pid_t pid_stderr = redirect_fifo_to_fd(pid_child, hash_name_fifo.at("stderr"), STDERR_FILENO);
  ns_log::debug()("Connected to stdin/stdout/stderr fifos");
  // Wait for processes to exit
  waitpid(pid_stdin, nullptr, 0);
  waitpid(pid_stdout, nullptr, 0);
  waitpid(pid_stderr, nullptr, 0);
  // Open exit code fifo and retrieve the exit code of the requested process
  int code_exit{};
  int bytes_exit = ns_linux::open_read_with_timeout(hash_name_fifo.at("exit").c_str()
    , std::chrono::seconds{SECONDS_TIMEOUT}
    , std::span<int>(&code_exit, 1)
  );
  qreturn_if(bytes_exit != sizeof(code_exit), std::unexpected("Incorrect number of bytes '{}' read"_fmt(bytes_exit)));
  return code_exit;
}

[[nodiscard]] std::expected<int, std::string> process_request(int argc, char** argv)
{
  std::vector<char*> args(argv+1, argv+argc);
  using namespace std::chrono_literals;
  // Forward signal to spawned child
  signal(SIGABRT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGCONT, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGIO, signal_handler);
  signal(SIGIOT, signal_handler);
  signal(SIGPIPE, signal_handler);
  signal(SIGPOLL, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGURG, signal_handler);
  signal(SIGUSR1, signal_handler);
  signal(SIGUSR2, signal_handler);
  signal(SIGVTALRM, signal_handler);
  // Set log level
  ns_log::set_level(ns_env::exists("FIM_DEBUG", "1")? ns_log::Level::DEBUG : ns_log::Level::ERROR);
  // Check args
  using get_expected_t = std::expected<std::string_view,std::string>;
  pid_t const pid_daemon = expect(expect(ns_env::get_expected("FIM_PID")
    .or_else([&](auto&& e) -> get_expected_t
    {
      ns_log::debug()("FIM_PID not defined ({}), trying argument instead...", e);
      if(args.empty()) { return std::unexpected<std::string>("PID argument is missing"); }
      std::string pid_str = args.front();
      args.erase(args.begin());
      return get_expected_t(pid_str);
    })
    .transform([](auto&& e){ return ns_exception::to_expected([&]{ return std::stoi(std::string{e}); }); })
  ));
  // Get instance directory
  fs::path path_dir_instance = expect(ns_env::get_expected("FIM_DIR_INSTANCE"));
  fs::path path_dir_portal = path_dir_instance / "portal";
  // Create define log file for child
  fs::path path_file_log = path_dir_portal / "cli_{}.log"_fmt(getpid());
  qreturn_if(std::error_code ec; (fs::create_directories(path_file_log.parent_path(), ec))
    , std::unexpected(std::format("Error to create log file: {}", ec.message()))
  );
  // Create fifos
  fs::path path_dir_fifo = path_dir_portal / "fifo";
  auto hash_name_fifo = expect(create_fifos(getpid(),
  {
      path_dir_fifo / "stdin"
    , path_dir_fifo / "stdout"
    , path_dir_fifo / "stderr"
    , path_dir_fifo / "exit"
    , path_dir_fifo / "pid"
  }));
  // Save environment
  fs::path path_file_env = path_dir_portal / "environments" / std::to_string(getpid());
  expect(set_environment(path_file_env));
  // Send message to daemon
  expect(send_message(path_dir_portal / "daemon_{}"_fmt(pid_daemon)
    , args
    , hash_name_fifo
    , path_file_log
    , path_file_env
  ));
  // Retrieve child pid
  return expect(process_wait(hash_name_fifo));
}

// main() {{{
int main(int argc, char** argv)
{
  auto result = process_request(argc, argv);
  // Reflect the original process return code
  if(result) { return result.value(); }
  // Error on process request
  ns_log::error()(result.error());
  return EXIT_FAILURE;
} // main() }}}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
