# Execute a command as a regular user

## What is it?

`fim-root` executes a command as the root user inside the flatimage container.

## How to Use

You can use `./app.flatimage fim-help root` to get the following usage details:

```txt
fim-root : Executes a command as the root user
Usage: fim-root <program> [args...]
   <program> : Name of the program to execute, it can be the name of a binary or the full path
   <args...> : Arguments for the executed program
Example: fim-root id -u
```

To install a program in the container:

```bash
# Allow network access
./app.flatimage fim-perms add network,audio,wayland,xorg
# Install firefox
./app.flatimage fim-root pacman -S firefox
```

## How it Works

FlatImage uses [bubblewrap](https://github.com/containers/bubblewrap) to execute
commands inside a containerized environment, setting both `--uid` and `--gid` options
to `0`.