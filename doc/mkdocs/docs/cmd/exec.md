# Execute Commands as Regular User

## What is it?

The `fim-exec` command executes programs inside the FlatImage container as a regular user (non-root). This is the primary way to run applications and commands within the containerized environment, providing a secure execution context with your normal user privileges.

**Use Cases:**

- Running GUI applications (browsers, editors, games)
- Executing command-line tools and utilities
- Testing software in an isolated environment
- Running scripts and automated tasks
- Accessing containerized development tools
- Launching applications with access to your home directory (when permitted)

**Difference from `fim-root`:**

- `fim-exec` - Runs as regular user (your UID/GID), suitable for applications
- `fim-root` - Runs as root (UID/GID 0), suitable for system administration

## How to Use

You can use `./app.flatimage fim-help exec` to get the following usage details:

```txt
fim-exec : Executes a command as a regular user
Usage: fim-exec <program> [args...]
  <program> : Name of the program to execute, it can be the name of a binary or the full path
  <args...> : Arguments for the executed program
Example: fim-exec echo -e "hello\nworld"
```

### Running Commands

Execute any program installed in the container:

```bash
# Run a simple command
./app.flatimage fim-exec echo "Hello from FlatImage"
# Output: Hello from FlatImage

# Run with arguments
./app.flatimage fim-exec ls -la /usr/bin

# Execute a script (requires home and/or media permissions)
./app.flatimage fim-exec bash myscript.sh
```

### Using Shell Commands

Execute shell commands and pipelines:

```bash
# Run a shell command with -c
./app.flatimage fim-exec sh -c 'echo "Current user: $(whoami)"'
# Output: Current user: <your-username>

# Complex shell operations
./app.flatimage fim-exec bash -c 'for i in 1 2 3; do echo "Count: $i"; done'
# Output:
# Count: 1
# Count: 2
# Count: 3

# Pipelines and redirections
./app.flatimage fim-exec sh -c 'ls /usr/bin | grep python | head -5'
```

**Note:** When using shell commands with special characters, wrap them in single quotes and use the `-c` flag.

### Environment Variables

Commands executed with `fim-exec` inherit environment variables set with `fim-env`:

```bash
# Set environment variables
./app.flatimage fim-env add 'MY_VAR=test_value'

# Access in executed commands
./app.flatimage fim-exec sh -c 'echo $MY_VAR'
# Output: test_value

# Use in applications
./app.flatimage fim-exec myapp
# myapp can read MY_VAR environment variable
```

### Exit Codes

`fim-exec` preserves the exit code of the executed command:

```bash
# Successful command (exit code 0)
./app.flatimage fim-exec true
echo $?
# Output: 0

# Failed command (non-zero exit code)
./app.flatimage fim-exec false
echo $?
# Output: 1

# Command not found (exit code 127)
./app.flatimage fim-exec nonexistent-command
echo $?
# Output: 127
```

This allows proper integration with shell scripts and automation tools.

### Combining Multiple Commands

Execute complex command sequences:

```bash
# Sequential commands
./app.flatimage fim-exec sh -c 'cd /tmp && mkdir test && cd test && pwd'
# Output: /tmp/test

# Conditional execution
./app.flatimage fim-exec sh -c 'test -f /file.txt && echo "File exists" || echo "File not found"'

# Background processes
./app.flatimage fim-exec sh -c 'sleep 10 &'
```

### Standard Input/Output/Error

`fim-exec` properly handles standard streams:

```bash
# Standard input
echo "test input" | ./app.flatimage fim-exec cat
# Output: test input

# Redirect output
./app.flatimage fim-exec echo "log entry" > output.txt

# Redirect stderr
./app.flatimage fim-exec sh -c 'echo error >&2' 2> errors.txt

# Pipes
./app.flatimage fim-exec ls /usr/bin | head -10
```

## How it Works

`fim-exec` uses [bubblewrap](https://github.com/containers/bubblewrap) to execute commands inside a containerized environment with your current user's identity.

**Technical Details:**

**User Identity:**

`fim-exec` passes your current UID and GID to bubblewrap:
```bash
--uid $(id -u)
--gid $(id -g)
```

This ensures:
- Files created have your ownership
- Permission checks match your user
- Home directory access (when permitted) works correctly

**Command Resolution:**

When you execute `fim-exec program`:
1. FlatImage checks if `program` is an absolute path
2. If not, searches directories in the container's `$PATH`
3. Executes the first match found
4. Returns error (exit code 127) if not found