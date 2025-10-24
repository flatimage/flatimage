# Configure Default Command

## What is it?

The `fim-boot` command sets what runs when you execute a FlatImage without any arguments. By default, FlatImage starts an interactive bash shell. You can change this to launch an application directly, making your FlatImage behave like a native executable.

**Use Cases:**

- Turn a FlatImage into a dedicated application launcher
- Create custom portable tools with specific default behavior
- Simplify usage by eliminating the need to specify the command every time
- Pre-configure arguments that should always be passed to a program

## How to Use

You can use `./app.flatimage fim-help boot` to get the following usage details:

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

### Set the Default Command

Configure what runs when you execute the FlatImage:

```bash
# Set Firefox as the default command
./app.flatimage fim-boot set firefox

# Now launching the FlatImage starts Firefox directly
./app.flatimage
```

**Example: Creating a portable text editor**

```bash
# Install vim in the container
./app.flatimage fim-perms add home,media,network
./app.flatimage fim-root apk add vim

# Set vim as the default command
./app.flatimage fim-boot set vim

# Commit and rename
./app.flatimage fim-layer commit
mv ./app.flatimage ./vim.flatimage

# Now it works like a portable vim executable
./vim.flatimage myfile.txt
```

### Set Command with Pre-configured Arguments

You can specify arguments that will always be prepended to user-supplied arguments:

```bash
# Configure echo to always print a prefix
./app.flatimage fim-boot set echo "PREFIX: "

# User arguments are appended to the pre-configured ones
./app.flatimage hello world
# Output: PREFIX: hello world
```

**Example: Creating a specialized tool**

```bash
# Set Python to always run in interactive mode with specific flags
./app.flatimage fim-boot set python3 -i -u

# Now these flags are always applied
./app.flatimage myscript.py
# Equivalent to: python3 -i -u myscript.py
```

**Example: Custom greeting command**

```bash
# Set a bash command with pre-configured message
./app.flatimage fim-boot set bash -c 'echo "Welcome to MyApp! Type 'help' for commands."; bash'

# Every time you run it, you get the greeting
./app.flatimage
# Output: Welcome to MyApp! Type 'help' for commands.
# [Opens interactive bash]
```

### Show Current Configuration

Display the currently configured boot command:

```bash
./app.flatimage fim-boot show
```

**Example output:**

```json
{
  "args": [
    "--no-remote",
    "--private-window"
  ],
  "program": "firefox"
}
```

The output shows:

- **program** - The command that will be executed
- **args** - Pre-configured arguments (if any)

**Example workflow:**

```bash
# Set a command with arguments
./app.flatimage fim-boot set firefox --no-remote --private-window

# Verify the configuration
./app.flatimage fim-boot show
{
  "args": [
    "--no-remote",
    "--private-window"
  ],
  "program": "firefox"
}

# Run it - launches Firefox with those arguments
./app.flatimage
```

### Clear the Boot Command

Revert to the default bash shell:

```bash
./app.flatimage fim-boot clear

# Now it opens a bash shell again
./app.flatimage
[flatimage] / >
```

**When to clear:**

- Converting a specialized FlatImage back to a general-purpose container
- Troubleshooting when the boot command isn't working as expected
- Returning to interactive mode for maintenance

### How Arguments Work

When you execute a FlatImage with arguments, they are passed to the boot command:

```bash
# Boot command is set to: firefox --no-remote
./app.flatimage fim-boot set firefox --no-remote

# User runs with additional arguments
./app.flatimage https://example.com

# Actual execution becomes:
# firefox --no-remote https://example.com
```

**Argument Order:**
1. Pre-configured arguments (from `fim-boot set`)
2. User-supplied arguments (from command line)

### Shell Command Behavior

You can use shell commands with proper quoting:

```bash
# Single command
./app.flatimage fim-boot set sh -c 'echo "Starting application..."; myapp'

# Complex shell script
./app.flatimage fim-boot set bash -c 'cd /workspace && source venv/bin/activate && python main.py'
```

**Note:** Use single quotes to prevent the host shell from expanding variables prematurely.

## How it Works

The boot command is stored directly in the FlatImage binary as embedded metadata. When FlatImage starts:

1. It reads the embedded configuration section
2. If a boot command is found and valid, it executes that command
3. If no boot command is set or it's corrupted, it falls back to bash
4. User-supplied command-line arguments are appended to the boot command

**Technical Details:**

- The configuration is written to a dedicated section in the ELF binary
- The data is stored as JSON for easy parsing
- FlatImage validates the configuration before execution
- Fallback to bash ensures the FlatImage remains usable even if the boot command fails

**Storage Location:**

The boot command is stored in the FlatImage binary itself, not in the filesystem layers. This means:

- It persists across layer changes
- It's part of the FlatImage's core configuration
- You can modify it without affecting the container filesystem