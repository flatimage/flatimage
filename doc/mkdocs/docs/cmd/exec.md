# Execute a command as a regular user

## What is it?

`fim-exec` executes a command as a regular user inside the flatimage container,
if the `HOME` permission is set, it also allows access to the host home
directory.

## How to Use

You can use `./app.flatimage fim-help exec` to get the following usage details:

```txt
fim-exec : Executes a command as a regular user
Usage: fim-exec <program> [args...]
   <program> : Name of the program to execute, it can be the name of a binary or the full path
   <args...> : Arguments for the executed program
Example: fim-exec echo -e "hello\nworld"
```

To run a program installed in the container:

```bash
./app.flatimage fim-exec firefox
```

## How it Works

FlatImage uses [bubblewrap](https://github.com/containers/bubblewrap) to execute
commands inside a containerized environment, using the `uid` and `gid` from the
current user.