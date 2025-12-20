# Environment Variables

## Overview

FlatImage provides environment variables for querying runtime information, controlling behavior, and accessing filesystem paths. Variables are organized into three categories:

1. **User-Modifiable Variables** - Can be set before running FlatImage to change behavior
2. **System-Set Variables** - Automatically set by FlatImage during initialization
3. **Internal Variables** - Used internally for configuration and debugging

## User-Modifiable Variables

These variables can be set before executing a FlatImage to control its behavior:

### Runtime Control

| Variable | Type | Description | Default |
|----------|------|-------------|---------|
| `FIM_DEBUG` | Integer (0/1) | Enable debug logging. | `0` (disabled) |
| `FIM_ROOT` | Integer (0/1) | Perform operations as root. | `0` (disabled) |
| `FIM_MAIN_OFFSET` | Flag | If set, prints the filesystem offset in the binary and exits. | Not set |
| `FIM_OVERLAY` | String | Override overlay filesystem type. Valid values: `bwrap`, `overlayfs`, `unionfs`. | From binary config |
| `FIM_CASEFOLD` | Integer (0/1) | Enable case-insensitive filesystem (CIOPFS layer). | From binary config |

Source: `environment.md` header, behavior verified in `filesystems/controller.hpp`

### Layer Management

| Variable | Type | Description | Example |
|----------|------|-------------|---------|
| `FIM_COMPRESSION_LEVEL` | Integer (0-9) | DwarFS compression level for `fim-layer commit` and `fim-layer create`. | `7` (default) |
| `FIM_LAYERS` | Colon-separated paths | Directories and/or layer files to mount. Directories are scanned for layer files; files are mounted directly. | `/path/to/layers:/path/to/layer.layer` |

**Layer Loading Priority:**

1. Embedded layers in binary (base layer)
2. Committed layers in binary
3. External layers from `FIM_LAYERS` environment variable
4. Managed layers from `FIM_DIR_LAYERS` (automatically mounted)

## System-Set Variables

These variables are automatically set by FlatImage during initialization and should not be modified by users:

### Metadata Variables

| Variable | Value | Description |
|----------|-------|-------------|
| `FIM_VERSION` | String | FlatImage version (e.g., `1.2.3`) |
| `FIM_COMMIT` | String | Git commit hash (first 7 chars) |
| `FIM_DIST` | String | Distribution name: `ARCH`, `ALPINE`, or `BLUEPRINT` |
| `FIM_TIMESTAMP` | String | Build timestamp in `YYYYMMDDHHMMSS` format |
| `FIM_PID` | Integer | Process ID of current FlatImage instance |

### Filesystem Path Variables

| Variable | Path | Description |
|----------|------|-------------|
| `FIM_DIR_GLOBAL` | `/tmp/fim` | Global temporary directory for all FlatImage instances |
| `FIM_DIR_APP` | `/tmp/fim/app/{COMMIT}_{TIMESTAMP}` | Application directory (unique per build) |
| `FIM_DIR_APP_BIN` | `$FIM_DIR_APP/bin` | Embedded binaries directory |
| `FIM_DIR_APP_SBIN` | `$FIM_DIR_APP/sbin` | Symbolic links directory |
| `FIM_DIR_INSTANCE` | `$FIM_DIR_APP/instance/{PID}` | Instance-specific directory (unique per process) |
| `FIM_DIR_RUNTIME` | `/tmp/fim/run` | Runtime directory |
| `FIM_DIR_RUNTIME_HOST` | `/tmp/fim/run/host` | Read-only host filesystem access (container-only) |
| `FIM_DIR_DATA` | `{BINARY_DIR}/.{BINARY_NAME}.data` | Host-side data directory (persistent storage) |
| `FIM_DIR_LAYERS` | `$FIM_DIR_DATA/layers` | Managed layers directory (automatically mounted) |

**Example Values:**
```bash
FIM_DIR_GLOBAL=/tmp/fim
FIM_DIR_APP=/tmp/fim/app/119216d_20241019145954
FIM_DIR_APP_BIN=/tmp/fim/app/119216d_20241019145954/bin
FIM_DIR_INSTANCE=/tmp/fim/app/119216d_20241019145954/instance/12345
FIM_DIR_DATA=/home/user/.firefox.flatimage.data
FIM_DIR_LAYERS=/home/user/.firefox.flatimage.data/layers
```

### Binary Path Variables

| Variable | Path | Description |
|----------|------|-------------|
| `FIM_BIN_SELF` | String | Full path to the FlatImage binary executable |
| `FIM_DIR_SELF` | String | Full path to the FlatImage binary directory |
| `FIM_OFFSET` | Integer | Byte offset of first DwarFS filesystem in binary |

## Internal Variables

These variables are used internally by FlatImage:

| Variable | Type | Description |
|----------|------|-------------|
| `FIM_DAEMON_CFG` | JSON string | Portal daemon configuration (serialized) | Portal system |
| `BASHRC_FILE` | Path | Instance-specific bashrc file path |
| `LD_LIBRARY_PATH` | Colon-separated paths | Library search paths for dynamic linking |
| `PATH` | Colon-separated paths | Executable search paths |

## Querying Environment Variables

From within a FlatImage container, you can query these variables:

```bash
# Show all FlatImage variables
env | grep FIM_

# Check current instance directory
echo $FIM_DIR_INSTANCE

# Verify distribution
echo $FIM_DIST

# Check version information
echo "FlatImage $FIM_VERSION (commit $FIM_COMMIT)"
```

From outside the container (before running):

```bash
# Enable debug mode
FIM_DEBUG=1 ./app.flatimage

# Force UnionFS overlay
FIM_OVERLAY=unionfs ./app.flatimage

# Enable case-insensitive filesystem
FIM_CASEFOLD=1 ./app.flatimage

# Load external layers
FIM_LAYERS=/path/to/extra.layer ./app.flatimage
```