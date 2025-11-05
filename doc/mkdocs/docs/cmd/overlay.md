# Overlay Filesystem

## What is it?

The `fim-overlay` command allows you to select which overlay filesystem implementation FlatImage uses to provide read-write access over the compressed read-only filesystem layers. An overlay filesystem creates a unified view by stacking multiple filesystem layers, with newer layers taking precedence over older ones.

**Why Overlay Filesystems Matter:**

FlatImage stores applications and data in compressed, read-only layers for efficiency. However, you need to modify files when:

- Installing new software
- Creating configuration files
- Writing application data
- Running applications that modify their directories

The overlay filesystem solves this by:

- Presenting a writable filesystem to applications
- Storing changes in a temporary layer (until committed)
- Preserving the read-only base layers unchanged
- Combining all layers into a seamless view

## How to Use

You can use `./app.flatimage fim-help overlay` to get the following usage details:

```txt
fim-overlay : Show or select the default overlay filesystem
Usage: fim-overlay <set> <overlayfs|unionfs|bwrap>
  <set> : Sets the default overlay filesystem to use
  <overlayfs> : Uses 'fuse-overlayfs' as the overlay filesystem
  <unionfs> : Uses 'unionfs-fuse' as the overlay filesystem
  <bwrap> : Uses 'bubblewrap' native overlay options as the overlay filesystem
Usage: fim-overlay <show>
  <show> : Shows the current overlay filesystem
```

### Show Current Overlay

Display which overlay implementation is currently configured:

```bash
./app.flatimage fim-overlay show
```

**Example output:**
```
bwrap
```

This shows the currently selected overlay filesystem (bwrap, unionfs, or overlayfs).

### Set Overlay Permanently

Change the default overlay filesystem by writing to the FlatImage configuration:

```bash
# Use bubblewrap native overlay (default)
./app.flatimage fim-overlay set bwrap

# Use unionfs-fuse
./app.flatimage fim-overlay set unionfs

# Use fuse-overlayfs
./app.flatimage fim-overlay set overlayfs
```

The setting is stored in the FlatImage binary and persists across executions.

**Example workflow:**

```bash
# Check current setting
./app.flatimage fim-overlay show
bwrap

# Switch to overlayfs for better performance
./app.flatimage fim-overlay set overlayfs

# Verify the change
./app.flatimage fim-overlay show
overlayfs

# Test with the new overlay
./app.flatimage fim-exec echo "Using overlayfs now"
```

### Set Overlay Temporarily

Use an environment variable to override the overlay setting for a single execution:

```bash
# Use bwrap for this execution only
export FIM_OVERLAY=bwrap
./app.flatimage fim-exec command

# Use unionfs temporarily
export FIM_OVERLAY=unionfs
./app.flatimage fim-exec command

# Use overlayfs temporarily
export FIM_OVERLAY=overlayfs
./app.flatimage fim-exec command

# Unset to use the configured default
unset FIM_OVERLAY
./app.flatimage fim-exec command
```

Temporary configuration doesn't modify the FlatImage binary, making it useful for:

- Testing different overlays
- One-off executions with specific requirements
- Debugging overlay-related issues
- Scripts that need specific overlay behavior

**Example: Testing overlay performance**

```bash
# Time execution with bwrap
time env FIM_OVERLAY=bwrap ./app.flatimage fim-exec bash -c 'for i in {1..1000}; do touch /tmp/file-$i; done'

# Time execution with overlayfs
time env FIM_OVERLAY=overlayfs ./app.flatimage fim-exec bash -c 'for i in {1..1000}; do touch /tmp/file-$i; done'

# Time execution with unionfs
time env FIM_OVERLAY=unionfs ./app.flatimage fim-exec bash -c 'for i in {1..1000}; do touch /tmp/file-$i; done'
```

## How it Works

An overlay filesystem provides a unified view of multiple filesystem layers by combining them into a single directory tree. FlatImage uses this to present read-only compressed layers with a writable top layer.

**Technical Details:**

**Overlay Operation:**

When you read or write files in the container:

1. **Read operation:**

   - Overlay checks upper directory (writable layer)
   - If not found, checks layer 2
   - If not found, checks layer 1
   - If not found, checks layer 0 (base)
   - Returns file from first layer where found

2. **Write operation:**

   - If file exists in lower layers, copy to upper directory (copy-up)
   - Modify file in upper directory
   - Original in lower layers remains unchanged

3. **Delete operation:**

   - Create "whiteout" marker in upper directory
   - Overlay hides the file from lower layers
   - Original file still exists in lower layers

**Layer Stacking:**

Layers are stacked from oldest (bottom) to newest (top):

```
┌─────────────────────────────────┐
│   Upper Directory (writable)    │  ← New files, modifications
├─────────────────────────────────┤
│   Layer 2: Applications         │  ← Last commit
├─────────────────────────────────┤
│   Layer 1: Updates              │  ← Second commit
├─────────────────────────────────┤
│   Layer 0: Base System          │  ← Initial layer
└─────────────────────────────────┘
```

**File Precedence:**

If `/etc/config.conf` exists in multiple layers:

1. Check upper directory first (uncommitted changes)
2. If not found, check layer 2
3. If not found, check layer 1
4. If not found, check layer 0
5. Use first found