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

#include "../std/expected.hpp"
#include "../lib/log.hpp"
#include "../lib/env.hpp"
#include "../macro.hpp"
#include "../db/portal/message.hpp"
#include "../db/portal/daemon.hpp"
#include "config.hpp"
#include "child.hpp"
#include "fifo.hpp"

extern char** environ;

namespace fs = std::filesystem;
namespace ns_message = ns_db::ns_portal::ns_message;
namespace ns_daemon = ns_db::ns_portal::ns_daemon;

// https://stackoverflow.com/questions/24931456/how-does-sig-atomic-t-actually-work
// https://en.cppreference.com/w/cpp/utility/program/sig_atomic_t.html
// https://man7.org/linux/man-pages/man7/signal-safety.7.html
// https://www.cs.wm.edu/~smherwig/courses/csci415-common/signals/sig_atomic/index.html
volatile std::sig_atomic_t G_CONTINUE = 1;

/**
 * @brief Signal handler for daemon cleanup
 *
 * Sets the G_CONTINUE flag to 0 to signal the daemon's main loop to exit.
 *
 * @param sig Signal number (unused)
 */
void cleanup(int sig)
{
  G_CONTINUE = 0;
}

/**
 * @brief Entry point for the portal daemon
 *
 * @return int Exit code (0 for success, non-zero for failure)
 */
int main()
{
  // Register cleanup signal handler
  signal(SIGTERM, cleanup);
  // Ignore SIGPIPE - when parent dies and pipe readers close, we can still cleanup
  signal(SIGPIPE, SIG_IGN);

  auto __expected_fn = [](auto&& e){ logger("E::{}", e.error()); return EXIT_FAILURE; };
  // Notify
  logger("D::Started host daemon");

  // Retrieve daemon configuration from environment variables or command-line arguments
  std::string daemon_cfg_str = Pop(ns_env::get_expected("FIM_DAEMON_CFG"));
  std::string daemon_log_str = Pop(ns_env::get_expected("FIM_DAEMON_LOG"));

  // Parse arguments
  auto args_cfg = Pop(ns_daemon::deserialize(daemon_cfg_str));
  auto args_log = Pop(ns_daemon::ns_log::deserialize(daemon_log_str));

  // Configure logger file
  ns_log::set_sink_file(args_log.get_path_file_parent());
  logger("D::Initialized portal daemon in {} mode", args_cfg.get_mode().lower());

  // Create a fifo to receive commands from
  fs::path path_fifo_in = Pop(ns_portal::ns_fifo::create(args_cfg.get_path_fifo_listen()));
  int fd_fifo = ::open(path_fifo_in.c_str(), O_RDONLY | O_NONBLOCK);
  return_if(fd_fifo < 0
    , EXIT_FAILURE
    , "E::Could not open file '{}': {}", path_fifo_in, strerror(errno)
  );
  logger("D::Listening fifo {}", path_fifo_in);

  // Create dummy writter to keep fifo open
  [[maybe_unused]] int fd_dummy = ::open(path_fifo_in.c_str(), O_WRONLY);
  return_if(fd_dummy < 0
    , EXIT_FAILURE
    , "E::Could not open dummy writer in '{}', {}", path_fifo_in, strerror(errno)
  );

  // Get reference pid to the main flatimage program
  pid_t pid_reference = args_cfg.get_pid_reference();

  // ::read is non-blocking due to O_NONBLOCK, but make sure that any other
  // operation that might hang do not leave the daemon running after the parent
  // dies
  [[maybe_unused]] auto thread_sig = std::jthread([pid_reference,pid=getpid()]
  {
    while(kill(pid_reference, 0) == 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
    kill(pid, SIGTERM);
  });

  // Recover messages
  for(char buffer[16384];  G_CONTINUE;)
  {
    ssize_t bytes_read = ::read(fd_fifo, &buffer, SIZE_BUFFER_READ);
    // Check if the read was success full, should try again, or stop
    // == 0 -> EOF
    // <  0 -> error (possible retry, because of non-blocking operation)
    // >  0 -> Possibly valid data to parse
    if (bytes_read == 0)
    {
      break;
    }
    else if (bytes_read < 0)
    {
      if (errno == EAGAIN or errno == EWOULDBLOCK)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        continue;
      }
      else
      {
        break;
      }
    }
    // Create a safe view over read data
    std::string_view msg{buffer, static_cast<size_t>(bytes_read)};
    logger("D::Recovered message: {}", msg);
    // Validate and deserialize message
    auto message = ns_message::deserialize(msg);
    continue_if(not message, "E::Could not parse message: {}", message.error());
    // Spawn child
    if(pid_t pid = fork(); pid < 0)
    {
      logger("E::Could not fork child");
    }
    else if (pid == 0)
    {
      ns_portal::ns_child::spawn(args_log, message.value()).discard("C::Could not spawn grandchild");
      _exit(0);
    }
  } // for

  logger("D::Portal daemon shutdown, G_CONTINUE={}", G_CONTINUE);

  close(fd_dummy);
  close(fd_fifo);

  return EXIT_SUCCESS;
}

/* vim: set expandtab fdm=marker ts=2 sw=2 tw=100 et :*/
