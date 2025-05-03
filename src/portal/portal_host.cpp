///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : portal_host
///

#include <atomic>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <csignal>
#include <filesystem>
#include <unistd.h>

#include "../cpp/lib/log.hpp"
#include "../cpp/lib/ipc.hpp"
#include "../cpp/lib/fifo.hpp"
#include "../cpp/lib/db.hpp"
#include "../cpp/lib/env.hpp"
#include "../cpp/macro.hpp"

namespace fs = std::filesystem;

extern char** environ;

std::atomic_bool G_CONTINUE = true;

// signal_handler() {{{
void signal_handler(int)
{
  G_CONTINUE = false;
} // signal_handler() }}}

// search_path() {{{
std::optional<fs::path> search_path(fs::path query)
{
  const char* env_path = std::getenv("PATH");
  ereturn_if( env_path == nullptr, "PATH environment variable not found", std::nullopt);

  const char* env_dir_global_bin = std::getenv("FIM_DIR_GLOBAL_BIN");
  const char* env_dir_static = std::getenv("FIM_DIR_STATIC");

  if ( query.is_absolute() )
  {
    return_if_else(fs::exists(query), query, std::nullopt);
  } // if

  std::string path(env_path);
  std::istringstream istream_path(path);
  std::string str_getline;

  while (std::getline(istream_path, str_getline, ':'))
  {
    fs::path path_parent = str_getline;
    qcontinue_if(env_dir_static and path_parent == fs::path(env_dir_static));
    qcontinue_if(env_dir_global_bin and path_parent == fs::path(env_dir_global_bin));
    fs::path path_full = path_parent / query;
    qreturn_if(fs::exists(path_full), path_full);
  } // while

  return std::nullopt;
} // search_path() }}}

// fork_execve() {{{
// Fork & execve child
void fork_execve(std::string msg)
{
  auto db = ns_db::Db(msg);

  // Get command
  auto vec_argv = db.template value<std::vector<std::string>>("command").value();

  // Ignore on empty command
  if ( vec_argv.empty() ) { return; }

  // Create child
  pid_t ppid = getpid();
  pid_t pid = fork();

  // Failed to fork
  ereturn_if(pid < 0, "Failed to fork");

  // Is parent
  if (pid > 0)
  {
    auto pid = db("pid").template value<std::string>().value();
    // Write pid to fifo
    auto expected_bytes_pid = ns_fifo::open_and_write(pid.c_str(), std::span(&pid, sizeof(pid)));
    elog_if(not expected_bytes_pid, expected_bytes_pid.error());
    elog_if(*expected_bytes_pid != sizeof(pid), "Could not write pid");
    // Wait for child to finish
    int status;
    ereturn_if(waitpid(std::stoi(pid), &status, 0) < 0, "waitpid failed");
    // Get exit code
    int code = (not WIFEXITED(status))? 1 : WEXITSTATUS(status);
    ns_log::debug()("Exit code: {}", code);
    // Send exit code of child through a fifo
    auto code_exit = db("exit").template value<std::string>().value();
    auto expected_bytes_code = ns_fifo::open_and_write(code_exit, std::span(&code, sizeof(code)));
    ereturn_if(not expected_bytes_code, expected_bytes_code.error());
    ereturn_if(*expected_bytes_code != sizeof(code), "Could not write exit code");
    return;
  } // if

  // Die with daemon
  eabort_if(prctl(PR_SET_PDEATHSIG, SIGKILL) < 0, strerror(errno));
  eabort_if(::kill(ppid, 0) < 0, "Parent died, prctl will not have effect: {}"_fmt(strerror(errno)));
  ns_log::debug()("{} dies with {}", getpid(), ppid);

  // Open stdout/stderr FIFOs
  auto str_stdout = db("stdout").template value<std::string>().value();
  auto str_stderr = db("stderr").template value<std::string>().value();
  int fd_stdout = open(str_stdout.c_str(), O_WRONLY);
  int fd_stderr = open(str_stderr.c_str(), O_WRONLY);
  eabort_if(fd_stdout < 0 or fd_stderr < 0, strerror(errno));

  // Redirect stdout and stderr
  eabort_if(dup2(fd_stdout, STDOUT_FILENO) < 0, strerror(errno));
  eabort_if(dup2(fd_stderr, STDERR_FILENO) < 0, strerror(errno));

  // Close the original file descriptors
  close(fd_stdout);
  close(fd_stderr);

  // Search for command in PATH and replace vec_argv[0] with the full path to the binary
  auto opt_path_file_command = search_path(vec_argv[0]);
  eabort_if(not opt_path_file_command, "'{}' not found in PATH"_fmt(vec_argv[0]));
  vec_argv[0] = opt_path_file_command->c_str();

  // Create arguments for execve
  auto argv_custom = std::make_unique<const char*[]>(vec_argv.size()+1);
  // Set last arg to nullptr
  argv_custom[vec_argv.size()] = nullptr;
  // Copy entries
  for(size_t i = 0; i < vec_argv.size(); ++i)
  {
    argv_custom[i] = vec_argv[i].c_str();
  } // for

  // Fetch environment from db
  fs::path path_file_environment = db.template value<std::string>("environment").value();
  std::vector<std::string> vec_environment;
  std::ifstream file_environment(path_file_environment);
  eabort_if(not file_environment.is_open(), "Could not open {}"_fmt(path_file_environment));
  for (std::string entry; std::getline(file_environment, entry);)
  {
    vec_environment.push_back(entry);
  } // for
  file_environment.close();
  // Create environment for execve
  auto env_custom = std::make_unique<const char*[]>(vec_environment.size()+1);
  // Set last arg to nullptr
  env_custom[vec_environment.size()] = nullptr;
  // Copy entries
  for(size_t i = 0; i < vec_environment.size(); ++i)
  {
    env_custom[i] = vec_environment[i].c_str();
  } // for

  // Perform execve
  execve(argv_custom[0], (char**) argv_custom.get(), (char**) env_custom.get());

  // Child should stop here
  std::abort();
} // fork_execve() }}}

// validate() {{{
decltype(auto) validate(std::string_view msg) noexcept
{
  try
  {
    auto db = ns_db::Db(msg);
    return db.value("command").value().data().is_array()
      and db.value("stdout").value().data().is_string()
      and db.value("stderr").value().data().is_string()
      and db.value("exit").value().data().is_string()
      and db.value("pid").value().data().is_string()
      and db.value("environment").value().data().is_string();
  } // try
  catch(...)
  {
    return false;
  } // catch
} // validate() }}}

// main() {{{
int main(int argc, char** argv)
{
  // Configure logger
  fs::path path_file_log = std::string{ns_env::get_or_throw("FIM_DIR_MOUNT")} + ".portal.daemon.log";
  ns_log::set_sink_file(path_file_log);
  ns_log::set_level((ns_env::exists("FIM_DEBUG", "1"))? ns_log::Level::DEBUG : ns_log::Level::ERROR);

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  // Check args
  ereturn_if(argc != 2, "Incorrect number of arguments", EXIT_FAILURE);

  // Create ipc instance
  auto ipc = ns_ipc::Ipc::host(argv[1]);

  // Recover messages
  while (G_CONTINUE)
  {
    auto opt_msg = ipc.recv();

    ibreak_if(opt_msg == std::nullopt, "Empty message");

    ns_log::info()("Recovered message: {}", *opt_msg);

    econtinue_if(not validate(*opt_msg), "Failed to validate message");

    fork_execve(*opt_msg);
  } // while

  return EXIT_SUCCESS;
} // main() }}}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
