/**
 * @file portal_daemon.cpp
 * @author Ruan Formigoni
 * @brief Spawns a daemon that receives child process requests
 * 
 * @copyright Copyright (c) 2025 Ruan Formigoni
 */

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

#include "../std/filesystem.hpp"
#include "../std/expected.hpp"
#include "../lib/log.hpp"
#include "../lib/env.hpp"
#include "../macro.hpp"
#include "../db/portal/message.hpp"
#include "config.hpp"
#include "child.hpp"
#include "fifo.hpp"

namespace fs = std::filesystem;
namespace ns_message = ns_db::ns_portal::ns_message;

extern char** environ;

int main(int argc, char** argv)
{
  auto __expected_fn = [](auto&&){ return EXIT_FAILURE; };
  // Create directory for the portal data
  fs::path path_dir_instance = Pop(ns_env::get_expected("FIM_DIR_INSTANCE"));
  fs::path path_dir_portal = path_dir_instance / "portal";
  Pop(ns_fs::create_directories(path_dir_portal), "C::Could not create portal daemon directories: {}");

  // Retrieve referece pid argument, run the loop as long as it exists
  ereturn_if(argc < 3, "Missing PID argument", EXIT_FAILURE);
  pid_t pid_reference = Try(std::stoi(argv[1]));
  // Operation mode
  // host == Runs on host
  // guest == Runs on container
  std::string mode = (std::string_view{argv[2]} == "host")? "host" : "guest";

  // Configure logger file
  fs::path path_file_log = fs::path{path_dir_portal} / std::format("daemon.{}.log", mode);
  ns_log::set_sink_file(path_file_log);
  ns_log::set_level((ns_env::exists("FIM_DEBUG", "1"))? ns_log::Level::DEBUG : ns_log::Level::CRITICAL);
  // Create a fifo to receive commands from
  fs::path path_fifo_in = Pop(ns_portal::ns_fifo::create(path_dir_portal / std::format("daemon.{}.fifo", mode)));
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
    logger("I::Recovered message: {}", msg);
    // Validate and deserialize message
    auto message = ns_message::deserialize(msg);
    econtinue_if(not message, std::format("Could not parse message: {}", message.error()));
    // Spawn child
    if(pid_t pid = fork(); pid < 0)
    {
      logger("E::Could not fork child");
    }
    else if (pid == 0)
    {
      ns_portal::ns_child::spawn(message.value()).discard("C::Could not spawn child");
      _exit(1);
    }
  } // for

  close(fd_dummy);
  close(fd_fifo);

  return EXIT_SUCCESS;
}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
