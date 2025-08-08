///
// @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
// @file        : portal_host
///

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <expected>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <thread>
#include <string>
#include <csignal>
#include <filesystem>
#include <unistd.h>
#include <unordered_map>

#include "../cpp/lib/log.hpp"
#include "../cpp/lib/ipc.hpp"
#include "../cpp/lib/db.hpp"
#include "../cpp/lib/env.hpp"
#include "../cpp/macro.hpp"
#include "config.hpp"
#include "child.hpp"
#include "fifo.hpp"

namespace fs = std::filesystem;

extern char** environ;

// validate() {{{
[[nodiscard]] std::expected<bool,std::string> validate(std::string_view msg) noexcept
{
  // Open database
  auto db = expect(ns_db::from_string(msg));
  // Define keys and types
  std::unordered_map<std::string, ns_db::KeyType> hash_key_type {{
      {"command", ns_db::KeyType::array}
    , {"stdin", ns_db::KeyType::string}
    , {"stdout", ns_db::KeyType::string}
    , {"stderr", ns_db::KeyType::string}
    , {"exit", ns_db::KeyType::string}
    , {"pid", ns_db::KeyType::string}
    , {"log", ns_db::KeyType::string}
    , {"environment", ns_db::KeyType::string}
  }};
  // Validate keys and types
  for(auto [key,type] : hash_key_type)
  {
    ereturn_if(not db.contains(key), std::format("Key {} is missing", key), false);
    ereturn_if(not (db(key).type() == type), std::format("Key {} is missing", key), false);
  }
  return true;
} // validate() }}}

// main() {{{
int main(int argc, char** argv)
{
  // Create directory for the portal data
  fs::path path_dir_instance = expect_map_error(ns_env::get_expected("FIM_DIR_INSTANCE")
    , [](auto&& e) { std::cerr << e << '\n'; return EXIT_FAILURE; }
  );
  fs::path path_dir_portal = path_dir_instance / "portal";
  ereturn_if(std::error_code ec;
      not fs::exists(path_dir_portal, ec) and not fs::create_directories(path_dir_portal, ec)
    , "Could not create portal daemon directories: {}"_fmt(ec.message())
    , EXIT_FAILURE
  );

  // Retrieve referece pid argument, run the loop as long as it exists
  ereturn_if(argc < 3, "Missing PID argument", EXIT_FAILURE);
  pid_t pid_reference = std::stoi(argv[1]);
  
  // Operation mode
  // host == Runs on host
  // guest == Runs on container
  std::string mode = (std::string_view{argv[2]} == "host")? "host" : "guest";

  // Configure logger file
  fs::path path_file_log = fs::path{path_dir_portal} / "daemon.{}.log"_fmt(mode);
  ns_log::set_sink_file(path_file_log);
  ns_log::set_level((ns_env::exists("FIM_DEBUG", "1"))? ns_log::Level::DEBUG : ns_log::Level::QUIET);

  // Create a fifo to receive commands from
  fs::path path_fifo_in = expect_map_error(
    create_fifo(path_dir_portal / "daemon.{}.fifo"_fmt(mode))
    , [](auto&& e){ std::cerr << e << '\n'; return EXIT_FAILURE; }
  );
  int fd_fifo = ::open(path_fifo_in.c_str(), O_RDONLY | O_NONBLOCK);
  ereturn_if(fd_fifo < 0, strerror(errno), EXIT_FAILURE);

  // Create dummy writter to keep fifo open
  [[maybe_unused]] int fd_dummy = ::open(path_fifo_in.c_str(), O_WRONLY);

  // Recover messages
  for(char buffer[16384]; kill(pid_reference, 0) == 0;)
  {
    ssize_t bytes_read = ::read(fd_fifo, &buffer, SIZE_BUFFER_READ);
    // Check if the read was success full, should try again, or stop
    // == 0 -> EOF
    // <  0 -> error (possible retry, because of non-blocking operation)
    // >  0 -> Possibly valid data to parse
    if (bytes_read == 0) { break; }
    if (bytes_read < 0)
    {
      if (errno != EAGAIN and errno != EWOULDBLOCK) { break; }
      else { std::this_thread::sleep_for(std::chrono::milliseconds{100}); continue; }
    }
    // Create a safe view over read data
    std::string_view msg{buffer, static_cast<size_t>(bytes_read)};
    ns_log::info()("Recovered message: {}", msg);
    // Validate json data
    auto validation = validate(msg);
    econtinue_if(not validation, "Could not perform the validation of the message: {}"_fmt(validation.error()));
    econtinue_if(not validation.value(), "Failed to validate message")
    // Spawn child
    if(pid_t pid = fork(); pid < 0)
    {
      ns_log::error()("Could not fork child");
    }
    else if (pid == 0)
    {
      elog_expected("Could not spawn child: {}", child::spawn(path_dir_portal, msg));
      _exit(1);
    }
  } // for

  close(fd_dummy);
  close(fd_fifo);

  return EXIT_SUCCESS;
} // main() }}}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
