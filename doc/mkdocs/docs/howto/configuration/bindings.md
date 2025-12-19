# Bind Directories

Mount host directories inside your FlatImage container for seamless file access.

## What are Bind Mounts?

Bind mounts make host directories accessible inside the container at specific paths. This allows containerized applications to:

- Access files on your host system
- Save data to specific locations
- Share directories between host and container
- Use custom home directories

## Quick Examples

### Basic Read-Only Bind

Mount a host directory as read-only:

```bash
# Bind host directory to container path
./app.flatimage fim-bind add ro '$HOME/Documents' /docs

# Access files inside container
./app.flatimage fim-exec ls /docs
```

### Read-Write Bind

Allow the container to modify files:

```bash
# Bind with write access
./app.flatimage fim-bind add rw '$PWD' /workspace

# Create files from inside container
./app.flatimage fim-exec sh -c 'echo "test" > /workspace/test.txt'

# File appears on the current directory
cat test.txt
```

### Multiple Binds

Configure multiple mount points:

```bash
# Bind several directories
./app.flatimage fim-bind add ro '$HOME/Documents' /documents
./app.flatimage fim-bind add rw '$HOME/Downloads' /downloads

# List all configured binds
./app.flatimage fim-bind list
```

### Share Data Between Applications

Use a common directory across multiple FlatImages:

```bash
# Create shared directory
mkdir -p "$PWD/shared"

# Bind in first application
./app1.flatimage fim-bind add rw "$PWD/shared" /shared

# Bind in second application
./app2.flatimage fim-bind add rw "$PWD/shared" /shared

# Both applications can access the same files
./app1.flatimage fim-exec sh -c 'echo "test" > /shared/test.txt'
./app2.flatimage fim-exec sh -c 'cat /shared/test.txt'
```

## Managing Binds

### List Configured Binds

```bash
./app.flatimage fim-bind list
```

Output shows all configured bind mounts with permissions and paths.

### Remove Binds

```bash
# Remove specific bind by guest path
./app.flatimage fim-bind del 0
```

## Special Variables

Use these variables in bind paths:

- `$HOME` - User's home directory
- `$PWD` - Current working directory
- `$FIM_DIR_SELF` - Directory containing the FlatImage binary

**Example:**

```bash
# Bind directory next to the binary
./app.flatimage fim-bind add rw '"$FIM_DIR_SELF"/app-data' /data
```

## Learn More

- [fim-bind](../../cmd/bind.md) - Complete bind mount command documentation
- [fim-perms](../../cmd/perms.md) - Configure container permissions
- [Architecture: Directories](../../architecture/directories.md) - Directory structure details
