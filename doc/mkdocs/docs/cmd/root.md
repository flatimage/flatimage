# Execute Commands as Root

## What is it?

The `fim-root` command executes programs inside the FlatImage container as the root user (UID 0, GID 0). This provides administrative privileges within the container for system-level operations such as installing software and modifying system files.

**Use Cases:**

- Installing packages with package managers (apk, pacman, apt)
- Modifying system configuration files in `/etc`
- Creating or modifying system directories
- Changing file ownership and permissions
- Installing system-wide libraries and dependencies
- Setting up the container environment before committing

**Difference from `fim-exec`:**

- `fim-exec` - Runs as regular user (your UID/GID), suitable for applications
- `fim-root` - Runs as root (UID/GID 0), suitable for system administration

## How to Use

You can use `./app.flatimage fim-help root` to get the following usage details:

```txt
fim-root : Executes a command as the root user
Usage: fim-root <program> [args...]
  <program> : Name of the program to execute, it can be the name of a binary or the full path
  <args...> : Arguments for the executed program
Example: fim-root id -u
```

Execute commands with root privileges inside the container:

```bash
# Check user ID (should be 0 for root)
./app.flatimage fim-root id -u
# Output: 0

# Check username
./app.flatimage fim-root whoami
# Output: root

# List root-owned files
./app.flatimage fim-root ls -la /root
```

## How it Works

`fim-root` uses [bubblewrap](https://github.com/containers/bubblewrap) to execute commands inside the container with root user privileges (UID 0, GID 0).

**Technical Details:**

**User Identity:**

`fim-root` sets both the user UID and GID to `0` (root). This ensures:

- Files created have root ownership
- Permission checks match your root
- Home directory becomes `/root`

**Command Resolution:**

When you execute `fim-root program`:
1. FlatImage checks if `program` is an absolute path
2. If not, searches directories in the container's `$PATH`
3. Executes the first match found
4. Returns error (exit code 127) if not found