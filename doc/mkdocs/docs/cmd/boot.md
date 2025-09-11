# Select the default command

## What is it?

It specifies the default command to execute when no arguments are passed for the FlatImage.

## How to use

**Get the usage details** with `./app.flatimage fim-help boot`:

```txt
fim-boot : Configure the default startup command
Usage: fim-boot <set> <command> [args...]
  <set> : Execute <command> with optional [args] when FlatImage is launched
  <command> : Startup command
  <args...> : Arguments for the startup command
Example: fim-boot set echo test
Usage: fim-boot <show|clear>
  <show> : Displays the current startup command
  <clear> : Clears the set startup command
```

### Setting the default command

```bash
$ ./app.flatimage fim-boot set echo
```

Now FlatImage will call echo by default:

```bash
$ ./app.flatimage hello world
hello world
```

---

**Provide extra arguments** to the command, these will always be prepended to the cli ones:

```bash
$ ./app.flatimage fim-boot set echo "PREFIX: "
$ ./app.flatimage hello world
PREFIX: hello world
```

### Show the default command

To show the current command, use `fim-boot show`. This yields a json formatted output with the program and arguments as primary keys. 

```bash
# Set the default command
$ ./app.flatimage fim-boot set echo hello world
# Show the default command
$ ./app.flatimage fim-boot show
{
  "args": [
    "hello",
    "world"
  ],
  "program": "echo"
}
```

### Clear the default command

To clear the default command and revert back to bash, use `fim-boot clear`:

```bash
$ ./app.flatimage fim-boot clear
```

## How it works

The command is written to the FlatImage binary. FlatImage checks binary section before starting, looking for a default command and arguments. If this command is missing or corrupted, FlatImage falls back to bash.