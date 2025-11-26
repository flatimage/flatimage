# Environment Variables

## What is it?

The `fim-env` command allows you to define custom environment variables that will be available inside the FlatImage container. These variables persist across container restarts and are set automatically when the container starts.

**Use Cases:**

- Configure application behavior without modifying code
- Set runtime paths for libraries or data directories
- Define default settings for containerized applications
- Pass configuration to build tools or development environments
- Override system defaults with application-specific values
- Set locale, language, or timezone preferences

Environment variables defined with `fim-env` are separate from host environment variables and only exist within the container.

## How to Use

You can use `./app.flatimage fim-help env` to get the following usage details:

```txt
fim-env : Define environment variables in FlatImage
Usage: fim-env <add|set> <'key=value'...>
  <add> : Include a novel environment variable
  <set> : Redefines the environment variables as the input arguments
  <'key=value'...> : List of variables to add or set
Example: fim-env add 'APP_NAME=hello-world' 'HOME=/home/my-app'
Usage: fim-env <del> <keys...>
  <del> : Delete one or more environment variables
  <keys...> : List of variable names to delete
Example: fim-env del APP_NAME HOME
Usage: fim-env <list>
  <list> : Lists configured environment variables
Usage: fim-env <clear>
  <clear> : Clears configured environment variables
```

### Add Environment Variables

Add new environment variables while preserving existing ones:

```bash
# Add single variable
./app.flatimage fim-env add 'MY_NAME=user'

# Add multiple variables at once
./app.flatimage fim-env add 'MY_NAME=user' 'MY_STATE=happy'
```

**Important:** Use single quotes around the variable assignments to prevent your shell from expanding them prematurely.

**Example: Configure application settings**

```bash
# Set application configuration
./app.flatimage fim-env add 'APP_NAME=MyPortableApp' 'APP_VERSION=1.0.0'

# Set runtime paths
./app.flatimage fim-env add 'DATA_DIR=/opt/myapp/data' 'CONFIG_DIR=/etc/myapp'

# Use in application
./app.flatimage fim-exec sh -c 'echo "Running $APP_NAME version $APP_VERSION"'
# Output: Running MyPortableApp version 1.0.0
```

### Set Environment Variables (Replace All)

Replace all existing environment variables with a new set:

```bash
# This removes all existing variables and sets only these
./app.flatimage fim-env set 'NEW_VAR=value' 'ANOTHER_VAR=test'
```

Use `set` when you want to completely reconfigure the environment without keeping previous variables.

**Example: Reset environment configuration**

```bash
# Previous configuration
./app.flatimage fim-env add 'OLD_VAR=old' 'TEMP_VAR=temp'

# Replace with new configuration
./app.flatimage fim-env set 'PRODUCTION=true' 'DEBUG=false'

# Only PRODUCTION and DEBUG remain
./app.flatimage fim-env list
PRODUCTION=true
DEBUG=false
```

### List Environment Variables

Display all configured environment variables:

```bash
./app.flatimage fim-env list
```

**Example output:**

```
MY_NAME=user
MY_STATE=happy
APP_VERSION=1.0.0
```

The output shows one variable per line in `KEY=VALUE` format.

### Delete Specific Variables

Remove one or more environment variables by name:

```bash
# Delete single variable
./app.flatimage fim-env del MY_NAME

# Delete multiple variables
./app.flatimage fim-env del MY_NAME MY_STATE
```

**Example:**

```bash
# Before deletion
./app.flatimage fim-env list
MY_NAME=user
MY_STATE=happy
APP_VERSION=1.0.0

# Delete specific variables
./app.flatimage fim-env del MY_NAME MY_STATE

# After deletion
./app.flatimage fim-env list
APP_VERSION=1.0.0
```

### Clear All Variables

Remove all configured environment variables:

```bash
./app.flatimage fim-env clear
```

After clearing, no custom environment variables will be set in the container (though system defaults remain).

### Practical Examples

#### Example 1: Development Environment

Configure a development environment with build tools:

```bash
# Set development variables
./app.flatimage fim-env add 'CC=gcc' 'CXX=g++' 'MAKEFLAGS=-j4'
./app.flatimage fim-env add 'BUILD_TYPE=Release'

# Use in build process
./app.flatimage fim-exec sh -c 'echo "Building with $CC using $MAKEFLAGS"'
# Output: Building with gcc using -j4

# Run make inside container
./app.flatimage fim-exec make
```

#### Example 2: Application Configuration

Set runtime configuration for a containerized application:

```bash
# Configure application paths
./app.flatimage fim-env add 'MYAPP_DATA=/opt/myapp/data'
./app.flatimage fim-env add 'MYAPP_LOG=/var/log/myapp'
./app.flatimage fim-env add 'MYAPP_DEBUG=1'

# Set boot command to use these variables
./app.flatimage fim-boot set myapp

# Application reads configuration from environment
./app.flatimage
```

#### Example 3: Locale and Language Settings

Configure language and regional settings:

```bash
# Set locale variables
./app.flatimage fim-env add 'LANG=en_US.UTF-8'
./app.flatimage fim-env add 'LC_ALL=en_US.UTF-8'
./app.flatimage fim-env add 'TZ=America/New_York'

# Applications now use these locale settings
./app.flatimage fim-exec date
# Output shows America/New_York timezone
```

#### Example 4: Python Environment

Configure Python-specific environment variables:

```bash
# Set Python environment
./app.flatimage fim-env add 'PYTHONPATH=/opt/python/lib'
./app.flatimage fim-env add 'PYTHONUNBUFFERED=1'
./app.flatimage fim-env add 'PYTHONDONTWRITEBYTECODE=1'

# Install Python application
./app.flatimage fim-root apk add python3

# Python uses configured environment
./app.flatimage fim-exec python3 -c 'import sys; print(sys.path)'
```

#### Example 5: Wine Configuration

Set up Wine environment variables for Windows application compatibility:

```bash
# Configure Wine
./app.flatimage fim-env add 'WINEPREFIX=/opt/wine/prefix'
./app.flatimage fim-env add 'WINEDEBUG=-all'
./app.flatimage fim-env add 'WINEARCH=win64'

# Install Wine
./app.flatimage fim-root apk add wine

# Wine uses configured prefix
./app.flatimage fim-exec wine --version
```

### Using Variables in Commands

Environment variables are available in all container commands:

```bash
# Set variables
./app.flatimage fim-env add 'GREETING=Hello' 'USER_NAME=Alice'

# Use with fim-exec
./app.flatimage fim-exec sh -c 'echo "$GREETING, $USER_NAME!"'
# Output: Hello, Alice!

# Use with fim-root
./app.flatimage fim-root sh -c 'echo "Root sees: $GREETING"'
# Output: Root sees: Hello

# Use in default boot command
./app.flatimage fim-boot set sh -c 'echo "$GREETING from boot command"'
./app.flatimage
# Output: Hello from boot command
```

### Variable Expansion and Quoting

**Single quotes prevent host expansion:**

```bash
# Correct - variable set literally in container
./app.flatimage fim-env add 'MY_HOME=$HOME'

# Inside container, $HOME expands to container's HOME
./app.flatimage fim-exec sh -c 'echo $MY_HOME'
# Output: /root (or /home/user depending on context)
```

**Double quotes cause host expansion:**

```bash
# Incorrect - expands on host before being set
./app.flatimage fim-env add "MY_HOME=$HOME"

# This sets MY_HOME to YOUR host home path
./app.flatimage fim-env list
MY_HOME=/home/yourname
```

**Best practice:** Always use single quotes when defining environment variables unless you specifically want host-side expansion.

### Combining with Other Commands

Environment variables work seamlessly with other FlatImage features:

```bash
# Set environment
./app.flatimage fim-env add 'APP_MODE=production' 'LOG_LEVEL=info'

# Set boot command that uses environment
./app.flatimage fim-boot set myapp --mode '$APP_MODE'

# Bind host directory
./app.flatimage fim-bind add rw '$HOME/app-data' /data

# Set permissions
./app.flatimage fim-perms add home,network

# Commit everything
./app.flatimage fim-layer commit binary
```

## How it Works

Environment variables are stored in the FlatImage binary as embedded configuration data. When the container starts, FlatImage:

1. Reads the embedded environment variable configuration
2. Passes each variable to [bubblewrap](https://github.com/containers/bubblewrap) using the `--setenv` option
3. Bubblewrap sets these variables in the container environment before executing any commands

**Storage Format:**

The environment variables are stored as JSON in a dedicated section of the FlatImage binary:
```json
{
  "MY_NAME": "user",
  "MY_STATE": "happy",
  "APP_VERSION": "1.0.0"
}
```

This format allows efficient parsing and modification without affecting the filesystem layers.

**Scope:**

Environment variables set with `fim-env`:

- Are available to all processes in the container
- Persist across container restarts
- Are inherited by child processes
- Are isolated from the host environment
- Apply to both `fim-exec` and `fim-root` commands