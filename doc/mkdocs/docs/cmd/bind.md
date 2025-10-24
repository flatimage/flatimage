# Bind Host Paths

## What is it?

The `fim-bind` command allows you to make specific files, directories, or devices from your host system available inside the FlatImage container. This is useful when you need access to specific resources that aren't covered by the standard [permission system](./perms.md).

**Use Cases:**

- Mount external storage or specific directories
- Access hardware devices not covered by standard permissions
- Share configuration files between host and container
- Provide read-only access to data

## How to Use

You can use `./app.flatimage fim-help bind` to get the following usage details:

```txt
fim-bind : Bind paths from the host to inside the container
Usage: fim-bind <add> <type> <src> <dst>
  <add> : Create a novel binding of type <type> from <src> to <dst>
  <type> : ro, rw, dev
  <src> : A file, directory, or device
  <dst> : A file, directory, or device
Usage: fim-bind <del> <index>
  <del> : Deletes a binding with the specified index
  <index> : Index of the binding to erase
Usage: fim-bind <list>
  <list> : Lists current bindings
```

**Binding Types:**

- `ro` - Read-only: Container can read but not modify
- `rw` - Read-write: Container can read and modify
- `dev` - Device: Bind a hardware device

### Bind a Device

Make hardware devices available inside the container:

```bash
# Bind a specific device
./app.flatimage fim-bind add dev /dev/video0 /dev/video0
```

This makes `/dev/video0` (typically a webcam) accessible inside the container at the same path.

**Example: Access a custom USB device**

```bash
# Bind a USB device
./app.flatimage fim-bind add dev /dev/ttyUSB0 /dev/ttyUSB0
```

### Bind a Read-Only Directory

Mount directories as read-only when you want to access files without risking modification:

```bash
# Bind your Music directory as read-only
./app.flatimage fim-bind add ro '$HOME/Music' /Music
```

**Important:** The single quotes around `'$HOME/Music'` prevent immediate shell expansion. The `$HOME` variable will be expanded when the FlatImage runs, meaning:
- Each user gets their own Music directory mounted
- The binding adapts to different user environments automatically

**Example: Share a read-only data directory**

```bash
# Mount a shared data directory
./app.flatimage fim-bind add ro /mnt/shared-data /data

# Access it from the container
./app.flatimage fim-exec ls /data
```

### Bind a Read-Write Directory

Use read-write bindings when the container needs to modify host files:

```bash
# Bind your Documents folder with write access
./app.flatimage fim-bind add rw '$HOME/Documents' /Documents
```

The container can now read from and write to `~/Documents` through the `/Documents` path.

**Example: Working with project files**

```bash
# Mount your project directory
./app.flatimage fim-bind add rw '$HOME/projects/myapp' /workspace

# Edit files from the container
./app.flatimage fim-exec vim /workspace/main.cpp
```

**Warning:** Be cautious with read-write bindings. The container has full access to modify or delete files in the bound directory.

### List Current Bindings

View all configured bindings:

```bash
./app.flatimage fim-bind list
```

**Example output:**

```json
{
  "0": {
    "dst": "/dev/video0",
    "src": "/dev/video0",
    "type": "dev"
  },
  "1": {
    "dst": "/Music",
    "src": "$HOME/Music",
    "type": "ro"
  },
  "2": {
    "dst": "/Documents",
    "src": "$HOME/Documents",
    "type": "rw"
  }
}
```

The output shows:

- **Index** - The number used to reference this binding (for deletion)
- **src** - The host path being bound
- **dst** - Where it appears in the container
- **type** - The binding type (ro, rw, or dev)

### Delete a Binding

Remove a binding using its index:

```bash
# Delete binding at index 1
./app.flatimage fim-bind del 1

# Verify it's removed
./app.flatimage fim-bind list
```

**Example:**

```bash
# Before deletion
./app.flatimage fim-bind list
{
  "0": {
    "dst": "/dev/video0",
    "src": "/dev/video0",
    "type": "dev"
  },
  "1": {
    "dst": "/Music",
    "src": "$HOME/Music",
    "type": "ro"
  }
}

# Delete the Music binding
./app.flatimage fim-bind del 1

# After deletion
./app.flatimage fim-bind list
{
  "0": {
    "dst": "/dev/video0",
    "src": "/dev/video0",
    "type": "dev"
  }
}
```

### Bindings vs Permissions

**When to use bindings:**

- Need access to specific paths not covered by permissions
- Want fine-grained control over what's accessible
- Need to mount paths at custom locations in the container
- Working with specific hardware devices

**When to use permissions:**

- Need common functionality (display, audio, network, etc.)
- Want simple, predefined access patterns
- Prefer standard behavior across different systems

**Example:** Instead of binding `/run/user/1000/pulse/native` manually, use the `audio` permission which handles this automatically.

### Tips and Best Practices

**Use Variable Expansion for Portability**

```bash
# Good - adapts to different users
./app.flatimage fim-bind add ro '$HOME/data' /data

# Less portable - hardcoded to specific user
./app.flatimage fim-bind add ro /home/john/data /data
```

**Prefer Read-Only When Possible**

```bash
# If you only need to read, use ro
./app.flatimage fim-bind add ro '$HOME/photos' /photos
```

This prevents accidental modification and improves security.

**Bind Only What You Need**

```bash
# Good - specific access
./app.flatimage fim-bind add ro '$HOME/Documents/work' /work-docs

# Overly broad - exposes everything
./app.flatimage fim-bind add rw '$HOME' /home
```

## How it Works

FlatImage uses [bubblewrap's](https://github.com/containers/bubblewrap) bind mount mechanisms to make host resources available in the container. When you create a binding:

1. The binding configuration is saved to the FlatImage binary
2. On startup, FlatImage passes the binding to bubblewrap
3. Bubblewrap creates the bind mount using Linux kernel facilities
4. The resource appears at the specified path inside the container

**Technical Details:**
- Read-only bindings use `--ro-bind`
- Read-write bindings use `--bind`
- Device bindings use `--dev-bind` which also sets up device nodes properly
- All bindings respect existing container security boundaries