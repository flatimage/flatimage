# Multiple instances

## What is it?

FlatImage allows the execution of multiple instances of the same application.

## How to Use

You can use `./app.flatimage fim-help instance` to get the following usage details:

```txt
fim-instance : Manage running instances
Usage: fim-instance <exec> <id> [args...]
   <exec> : Run a command in a running instance
   <id> : id of the instance in which to execute the command
   <args> : Arguments for the 'exec' command
Example: fim-instance exec 0 echo hello
Usage: fim-instance <list>
   <list> : Lists current instances
```

Interact with multiple instances of the same FlatImage container using the command portal facility.

```bash
# Spawn two instances of the same container to the background
$ ./app.flatimage fim-exec sleep 1000 &
[1] 2179490
$ ./app.flatimage fim-root sleep 1000 &
[2] 2179708
# Execute a command in instance '0' (2179490)
$ ./app.flatimage fim-instance exec 0 id -u
1000
# Execute a command in instance '1' (2179708)
$ ./app.flatimage fim-instance exec 1 id -u
0
# List running instances
$ ./app.flatimage fim-instance list
0:2179490
1:2179708
```

## How it Works

There are two daemons that spawn with FlatImage:
1. One spawned in the host, to execute commands sent from the guest.
2. One spawned in the guest, to execute commands sent from the host.

The `fim-instance` command uses the second daemon, to send commands to the guest container.