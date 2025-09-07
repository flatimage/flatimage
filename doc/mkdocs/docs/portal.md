# Transparent Command Portal

## What is it?

Flatimage has a portal mechanism to execute commands in host machine, the output
and return code is returned in the guest as if the command was executed in it
(thus transparent).

## How to Use

Examples:

* Check if the host contains the thunar file manager: `fim_portal sh -c 'command -v thunar'`
* Open thunar in the host machine: `fim_portal thunar`
* Open thunar in the host machine (full path): `fim_portal /bin/thunar`
* Open the desktop folder with thunar on the host machine: `fim_portal thunar ~/Desktop`

## How it Works

<img src="https://raw.githubusercontent.com/flatimage/docs/master/docs/image/flatimage-portal.png"/>

Three FIFOs are created, for `stdout`, `stderr` and the `exit code`, the guest
connects to the `stdout` and `stderr` FIFOs as a reader. The command to be
executed in the host machine and the paths to the FIFOs are
serialized to the host daemon; the daemon creates a child process and replaces
its `stdout` and `stderr` pipes with the provided FIFOs. After the command is
finished, the exit code is sent through the `exit` pipe and the guest process
exits with the same code returned from the child host process.
