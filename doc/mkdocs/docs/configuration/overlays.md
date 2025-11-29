# Overlays

## Overview

FlatImage uses overlay filesystems to provide writable access on top of compressed read-only layers. The overlay system is crucial for allowing applications to modify files while preserving the immutable base layers.

## Available Overlay Types

FlatImage supports three overlay implementations, each with different characteristics:

### BWRAP (Default)

**Bubblewrap Native Overlay** uses the kernel's built-in overlayfs support through bubblewrap's `--overlay` option.

**Advantages:**
- Fastest performance (no FUSE overhead)
- Native kernel implementation
- Direct syscall access

**Limitations:**
- Cannot be used with case-insensitive filesystem (casefold)
- Requires kernel overlayfs support
- May fail with `SYS_mount` syscall restrictions on some systems

**Use when:**
- Maximum performance is needed
- Not using Wine/Proton (which requires casefold)
- Running on modern kernels with overlayfs support

### OVERLAYFS

**FUSE-OverlayFS** is a userspace implementation of overlay filesystems using the `fuse-overlayfs` binary.

**Advantages:**
- Good performance
- Works with casefold enabled
- No special kernel requirements
- Compatible with restricted environments

**Limitations:**
- FUSE overhead (slower than BWRAP)
- Requires `fuse-overlayfs` binary in PATH

**Use when:**
- BWRAP is incompatible (casefold enabled)
- Running in environments without overlayfs kernel support
- Need balance between performance and compatibility

### UNIONFS

**UnionFS-FUSE** is a userspace union filesystem using the `unionfs-fuse` binary.

**Advantages:**
- Maximum compatibility
- Works with casefold enabled
- Automatic fallback option
- Reliable across different systems

**Limitations:**
- Slowest performance
- FUSE overhead
- Requires `unionfs` binary in PATH

**Use when:**
- Other overlays fail
- Running in highly restricted environments
- Compatibility is more important than performance

## Configuration Methods

### Permanent Configuration

Use the [`fim-overlay`](../cmd/overlay.md) command to set the overlay type permanently:

```bash
# Set to BWRAP (default, fastest)
./app.flatimage fim-overlay set bwrap

# Set to OVERLAYFS (good performance, compatible with casefold)
./app.flatimage fim-overlay set overlayfs

# Set to UNIONFS (maximum compatibility)
./app.flatimage fim-overlay set unionfs
```

The setting is stored in the FlatImage binary's reserved space (1 byte at offset 9) and persists across all executions.

### Temporary Override

Use the `FIM_OVERLAY` environment variable to override the overlay type for a single execution:

```bash
# Test with different overlays
FIM_OVERLAY=bwrap ./app.flatimage fim-exec command
FIM_OVERLAY=overlayfs ./app.flatimage fim-exec command
FIM_OVERLAY=unionfs ./app.flatimage fim-exec command
```

This is useful for:
- Performance testing before committing to a permanent setting
- Troubleshooting overlay-related issues
- One-off executions with specific requirements

### Configuration Priority

FlatImage determines which overlay to use in this order (highest to lowest priority):

1. **Environment Variable:** `FIM_OVERLAY` (temporary override)
2. **Reserved Space:** Value set with `fim-overlay set` (permanent)
3. **Default:** BWRAP if nothing is configured

## Automatic Fallback

FlatImage includes intelligent fallback mechanisms:

### SYS_mount Fallback

If BWRAP overlay fails with a `SYS_mount` syscall error, FlatImage automatically retries with UNIONFS:

```
ERROR: Bwrap failed SYS_mount, retrying with unionfs-fuse...
```

This handles cases where kernel overlayfs is restricted by security policies.

### Casefold Incompatibility

When case-insensitive filesystem (casefold) is enabled, BWRAP overlay cannot be used. FlatImage automatically falls back to UNIONFS in this scenario.

## How Overlay Layers Work

FlatImage stacks multiple filesystem layers to create a unified view:

```
┌──────────────────────────────────────┐
│  Upper Directory (writable)          │  ← {BINARY_DIR}/.{BINARY_NAME}.data/data/
│  - New files and modifications       │  ← Persists across runs
│  - Per-instance changes              │
├──────────────────────────────────────┤
│  DwarFS Layer N (read-only)          │  ← Last commit
├──────────────────────────────────────┤
│  DwarFS Layer 1 (read-only)          │  ← Second commit
├──────────────────────────────────────┤
│  DwarFS Layer 0 (read-only)          │  ← Base system
└──────────────────────────────────────┘
         ↓ merged into ↓
┌──────────────────────────────────────┐
│  Mount Point (unified view)          │  ← /tmp/fim/app/.../instance/{PID}/mount/
└──────────────────────────────────────┘
```

### Layer Locations

- **Upper Directory (writable):** `{BINARY_DIR}/.{BINARY_NAME}.data/data/`
  - Stores all modifications
  - Persists across multiple runs
  - Can be committed to create new read-only layers

- **Work Directory (temporary):** `{BINARY_DIR}/.{BINARY_NAME}.data/work/{PID}/`
  - Overlay filesystem metadata
  - Per-instance (unique for each PID)
  - Cleaned up after exit

- **DwarFS Layers (read-only):** `/tmp/fim/app/{COMMIT}_{TIMESTAMP}/instance/{PID}/layers/{N}/`
  - Compressed with high ratio
  - Immutable after creation
  - Can be stacked infinitely

## Performance Comparison

Test the performance of different overlay types using this benchmark:

```bash
# Test BWRAP performance
time env FIM_OVERLAY=bwrap ./app.flatimage fim-exec bash -c \
  'for i in {1..1000}; do touch /tmp/test-$i; done'

# Test OVERLAYFS performance
time env FIM_OVERLAY=overlayfs ./app.flatimage fim-exec bash -c \
  'for i in {1..1000}; do touch /tmp/test-$i; done'

# Test UNIONFS performance
time env FIM_OVERLAY=unionfs ./app.flatimage fim-exec bash -c \
  'for i in {1..1000}; do touch /tmp/test-$i; done'
```

**Expected Results (approximate):**
- **BWRAP:** 1.0x (baseline, fastest)
- **OVERLAYFS:** 1.2-1.5x slower than BWRAP
- **UNIONFS:** 1.5-2.5x slower than BWRAP

Actual performance varies based on:
- Workload type (many small files vs. few large files)
- Kernel version
- Filesystem configuration
- Hardware capabilities

## Best Practices

### Choosing an Overlay

1. **Start with BWRAP** (default)
   - Best performance for most use cases
   - Works for standard applications

2. **Switch to OVERLAYFS if:**
   - Using Wine/Proton (requires casefold)
   - BWRAP fails with SYS_mount errors
   - Need compatibility with more systems

3. **Use UNIONFS if:**
   - Both BWRAP and OVERLAYFS fail
   - Running in highly restricted environments
   - Reliability is paramount

### Testing Before Committing

Always test with temporary override before setting permanently:

```bash
# Test the overlay first
export FIM_OVERLAY=overlayfs
./app.flatimage fim-exec your-application

# If it works well, make it permanent
./app.flatimage fim-overlay set overlayfs
```

### Verifying Configuration

Check your current overlay setting:

```bash
./app.flatimage fim-overlay show
```

## Troubleshooting

### BWRAP Fails with SYS_mount

**Symptom:** Error message "Bwrap failed SYS_mount"

**Cause:** Kernel overlayfs syscalls are restricted by security policies

**Solution:** Switch to OVERLAYFS or UNIONFS:
```bash
./app.flatimage fim-overlay set overlayfs
```

### Overlay Not Found

**Symptom:** Error about missing `fuse-overlayfs` or `unionfs` binary

**Cause:** Required overlay binary not in PATH

**Solution:**
- Install the missing package on your host system
- Switch to a different overlay type
- Use BWRAP (embedded in FlatImage)

### Poor Performance

**Symptom:** Slow file operations in container

**Cause:** Inefficient overlay implementation for your workload

**Solution:** Test different overlays and choose the fastest:
```bash
# Run benchmark with each overlay type
for overlay in bwrap overlayfs unionfs; do
  echo "Testing $overlay..."
  time env FIM_OVERLAY=$overlay ./app.flatimage fim-exec your-workload
done
```

## Related Commands

- [`fim-overlay`](../cmd/overlay.md) - Configure overlay type
- [`fim-casefold`](../cmd/casefold.md) - Enable case-insensitive filesystem
- [`fim-layer`](../cmd/layer.md) - Commit overlay changes to new layer
- [`fim-bind`](../cmd/bind.md) - Configure bind mounts

## Technical Details

### Storage in Binary

The overlay configuration is stored in the FlatImage binary's reserved space:
- **Offset:** 9 bytes from reserved space start
- **Size:** 1 byte
- **Format:** Bitmask (1 << 1 = BWRAP, 1 << 2 = OVERLAYFS, 1 << 3 = UNIONFS)

### BWRAP Native Implementation

When using BWRAP overlay, the overlay is configured via bubblewrap arguments:
```bash
bwrap --overlay-src {lower} --overlay {upper} {work} {mount} ...
```

The filesystem controller skips FUSE mount operations and passes overlay configuration directly to bubblewrap.

### FUSE Implementation

When using OVERLAYFS or UNIONFS, the filesystem controller mounts the overlay before starting bubblewrap:

**OVERLAYFS:**
```bash
fuse-overlayfs -f \
  -o squash_to_uid={uid} \
  -o squash_to_gid={gid} \
  -o lowerdir={layer0}:{layer1}:... \
  -o upperdir={upper} \
  -o workdir={work} \
  {mountpoint}
```

**UNIONFS:**
```bash
unionfs -f -o cow \
  {upper}=RW:{layerN}=RO:{layer1}=RO:{layer0}=RO \
  {mountpoint}
```

## See Also

- [Filesystem Architecture](../architecture/filesystem.md)
- [Layers Architecture](../architecture/layers.md)
- [How to Commit Changes](../howto/getting-started/commit-changes.md)
