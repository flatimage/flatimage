# Environment Variables

Control application behavior by setting environment variables in your FlatImage.

## Methods to Set Environment Variables

FlatImage supports three ways to pass environment variables to containerized applications, each with different precedence and use cases.

### Method 1: Runtime Environment Variables

Pass environment variables when executing the FlatImage:

```bash
# Single variable
FOO=bar ./app.flatimage fim-exec sh -c 'echo $FOO'
# Output: bar

# Multiple variables
FOO=bar BAZ=qux ./app.flatimage fim-exec sh -c 'echo $FOO $BAZ'
# Output: bar qux

# With exported variables
export MY_VAR=hello
./app.flatimage fim-exec sh -c 'echo $MY_VAR'
# Output: hello
```

**Characteristics:**
- Not persistent across runs
- Available to host shell before execution

### Method 2: Baked Environment Variables (fim-env)

Store environment variables permanently in the FlatImage binary:

```bash
# Add persistent variables
./app.flatimage fim-env add 'APP_MODE=production' 'LOG_LEVEL=info'

# Verify configuration
./app.flatimage fim-env list
# Output:
# APP_MODE=production
# LOG_LEVEL=info

# Variables are automatically available
./app.flatimage fim-exec sh -c 'echo $APP_MODE'
# Output: production
```

**Characteristics:**

- Overrides host's environment variables (Method 1)
- Stored in the binary's reserved space
- Persistent across container restarts

### Method 3: Container Default Variables

FlatImage automatically sets several variables for the container environment:

```bash
./app.flatimage fim-exec env | grep -E 'USER|HOME|SHELL|TERM|XDG'
# Output:
# USER=yourname
# HOME=/home/yourname
# SHELL=/bin/bash
# TERM=xterm
# XDG_RUNTIME_DIR=/run/user/1000
```

**Auto-configured variables:**
- `USER` - Username from container user configuration
- `HOME` - Home directory path
- `SHELL` - Shell path (typically `/bin/bash`)
- `TERM` - Terminal type (set to `xterm`)
- `XDG_RUNTIME_DIR` - Runtime directory for the user

**Characteristics:**
- Lowest precedence (overridden by all other methods)
- Automatically configured by bubblewrap
- Based on user identity (UID/GID)

## Variable Precedence

When the same variable is set by multiple methods, this order determines which value wins:

```
1. fim-env baked variables
   ↓ overrides
2. Runtime environment
   ↓ overrides
3. Container defaults
```

**Example:**

```bash
# Container default variable
./app.flatimage fim-exec sh -c 'echo $USER'
# Output: user

# Environment
USER=foo ./app.flatimage fim-exec sh -c 'echo $USER'
# Output: foo

# Check the default
./app.flatimage fim-env set 'USER=bar'
USER=foo ./app.flatimage fim-exec sh -c 'echo $USER'
# Output: bar
```

## Variable Expansion and Expressions

FlatImage expands variables and expressions when reading from `fim-env`:

### Basic Variable Expansion

```bash
# Store a variable reference
./app.flatimage fim-env add 'MY_HOME=$HOME'

# Inside container, $HOME expands to container's HOME
./app.flatimage fim-exec sh -c 'echo $MY_HOME'
# Output: /home/yourname (container's home)
```

### Command Substitution

Execute commands during variable expansion:

```bash
# Store command expression
./app.flatimage fim-env add 'USER_INFO=$(id -u)'

# Command executes during expansion
./app.flatimage fim-exec sh -c 'echo "USER: $USER_INFO"'
# Output: USER: 1000

# More complex expressions
./app.flatimage fim-env add 'SYS_INFO=$(uname -r)'
./app.flatimage fim-exec sh -c 'echo "Kernel: $SYS_INFO"'
# Output: Kernel: 6.1.0-40-amd64
```

### Quoting Rules

**Use single quotes to prevent host-side expansion:**

```bash
# Correct - variable expands in container
./app.flatimage fim-env add 'MY_PATH=$HOME/data'

# Inside container
./app.flatimage fim-exec sh -c 'echo $MY_PATH'
# Output: /home/yourname/data (container's HOME)
```

**Double quotes cause host-side expansion:**

```bash
# Incorrect - expands on host before storing
./app.flatimage fim-env add "MY_PATH=$HOME/data"

# This stores your host's HOME path
./app.flatimage fim-env list
# Output: MY_PATH=/home/yourname/data (host's path, now hardcoded)
```

### Expression Examples

**Get current user ID:**

```bash
./app.flatimage fim-env add 'UID_INFO="$(id -u)"'
./app.flatimage fim-exec sh -c 'echo $UID_INFO'
```

**Get timestamp:**

```bash
./app.flatimage fim-env add 'BOOT_DATE="$(date +%Y-%m-%d)"'
./app.flatimage fim-exec sh -c 'echo "Boot date: $BOOT_DATE"'
```

**Compute derived values:**

```bash
./app.flatimage fim-env add 'CORES="$(nproc)"'
./app.flatimage fim-exec sh -c 'echo "CPU cores: $CORES"'
```

**Multi-line output:**

```bash
./app.flatimage fim-env add 'DISTRO="$(cat /etc/os-release | grep PRETTY_NAME | cut -d= -f2)"'
./app.flatimage fim-exec sh -c 'echo $DISTRO'
```

## Managing Variables

### View All Variables

```bash
# List all configured variables
./app.flatimage fim-env list

# View specific variable
./app.flatimage fim-exec sh -c 'echo $MY_VAR'
```

### Add Variables

```bash
# Add single variable
./app.flatimage fim-env add 'NEW_VAR=value'

# Add multiple variables
./app.flatimage fim-env add 'VAR1=val1' 'VAR2=val2' 'VAR3=val3'
```

### Update Variables

```bash
# Use 'add' to update existing variables
./app.flatimage fim-env add 'EXISTING_VAR=new_value'
```

### Replace All Variables

```bash
# Clear and set new configuration
./app.flatimage fim-env set 'ENV=production' 'DEBUG=false'
```

### Delete Variables

```bash
# Delete specific variables
./app.flatimage fim-env del MY_VAR ANOTHER_VAR

# Delete all variables
./app.flatimage fim-env clear
```

## Learn More

- [fim-env](../cmd/env.md) - Complete fim-env command documentation
- [fim-exec](../cmd/exec.md) - Execute commands with environment variables
- [Configuration](../configuration/user-identity.md) - User identity and environment setup
