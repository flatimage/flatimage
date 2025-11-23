# Directories

## Overview

FlatImage organizes its runtime files into a well-defined directory structure. This document describes the complete directory hierarchy, including temporary directories in `/tmp/fim` and host-side data directories alongside the FlatImage binary.

## Complete Directory Structure

The following shows the complete directory structure with corresponding environment variables in brackets:

```
/tmp/fim/                                    [FIM_DIR_GLOBAL]
├── app/
│   └── {COMMIT}_{TIMESTAMP}/                [FIM_DIR_APP]
│       ├── bin/                             [FIM_DIR_APP_BIN]
│       │   ├── bash                         (embedded bash shell)
│       │   ├── fim_janitor                  (cleanup daemon)
│       │   ├── fim_portal                   (portal dispatcher)
│       │   └── fim_portal_daemon            (portal daemon)
│       ├── sbin/                            [FIM_DIR_APP_SBIN]
│       └── instance/
│           └── {PID}/                       [FIM_DIR_INSTANCE]
│               ├── bashrc                   (instance-specific bashrc)
│               ├── passwd                   (instance-specific passwd)
│               ├── portal/
│               │   ├── daemon/
│               │   │   ├── host.fifo        (host daemon FIFO)
│               │   │   └── guest.fifo       (guest daemon FIFO)
│               │   └── dispatcher/
│               │       ├── host/
│               │       └── guest/
│               ├── logs/
│               │   ├── boot.log             (boot initialization)
│               │   ├── bwrap/               (bubblewrap logs)
│               │   ├── daemon/
│               │   │   ├── host/
│               │   │   │   ├── daemon.log
│               │   │   │   └── {PID}/
│               │   │   │       ├── child.log
│               │   │   │       └── grand.log
│               │   │   └── guest/
│               │   │       ├── daemon.log
│               │   │       └── {PID}/
│               │   │           ├── child.log
│               │   │           └── grand.log
│               │   ├── dispatcher/
│               │   │   ├── host/
│               │   │   │   └── {PID}.log
│               │   │   └── guest/
│               │   │       └── {PID}.log
│               │   └── fuse/
│               │       ├── dwarfs.log       (DwarFS filesystem)
│               │       ├── ciopfs.log       (case-insensitive FS)
│               │       ├── overlayfs.log    (OverlayFS)
│               │       ├── unionfs.log      (UnionFS)
│               │       └── janitor.log      (cleanup daemon)
│               ├── mount/                   (merged root filesystem)
│               └── layers/                  (layer mount points)
│                   ├── 0/                   (base layer)
│                   ├── 1/                   (layer 1)
│                   └── N/                   (layer N)
│
└── run/                                     [FIM_DIR_RUNTIME]
    └── host/                                [FIM_DIR_RUNTIME_HOST]

{BINARY_DIR}/                                (directory containing the binary)
└── .{BINARY_NAME}.data/                     [FIM_DIR_DATA]
    ├── tmp/                                 (temporary files)
    ├── work/
    │   └── {PID}/                           (overlay work directory)
    ├── data/                                (overlay upper/data layer)
    ├── casefold/                            (case-insensitive mount point)
    └── recipes/                             (package recipe definitions)
```

## Directory Descriptions

### Global Directory (`/tmp/fim`)

The `/tmp/fim` directory is the root for all FlatImage temporary files. It contains:

- **`app/`**: Application-specific directories organized by build version
- **`run/`**: Runtime access to host filesystem (read-only)

### Application Directory (`{COMMIT}_{TIMESTAMP}`)

Each FlatImage build has a unique identifier composed of:

- **Git commit hash** (first 7 characters)
- **Build timestamp** in `YYYYMMDDHHMMSS` format

Example: `119216d_20241019145954` represents:

- Commit: `119216d`
- Built: 2024-10-19 at 14:59:54

This directory contains:

#### `bin/` - Embedded Binaries
Static binaries extracted from the FlatImage on first run:

- `bash` - Embedded bash shell for container
- `fim_janitor` - Cleanup daemon that unmounts filesystems on parent death
- `fim_portal` - Portal dispatcher for sending commands
- `fim_portal_daemon` - Portal daemon for receiving and executing commands

Referenced by: `FIM_DIR_APP_BIN`

#### `sbin/` - Symbolic Binaries
Symlinks to busybox tools.

Referenced by: `FIM_DIR_APP_SBIN`

#### `instance/{PID}/` - Process Instances
Each running FlatImage instance creates a PID-specific directory. Multiple instances of the same FlatImage can run simultaneously, each with its own isolated instance directory.

Referenced by: `FIM_DIR_INSTANCE`

**Instance-specific files:**
- `bashrc` - Bash configuration for this instance
- `passwd` - User/group configuration for this instance

### Portal Directory

The portal system enables transparent IPC between host and container. See [Portal Architecture](portal.md) for details.

**Structure:**
```
portal/
├── daemon/
│   ├── host.fifo      - Named pipe for host daemon
│   └── guest.fifo     - Named pipe for guest daemon
└── dispatcher/
    ├── host/          - Host dispatcher temporary files
    └── guest/         - Guest dispatcher temporary files
```

### Logs Directory

Comprehensive logging structure for all FlatImage subsystems. Each component has dedicated log files for debugging and monitoring.

**Log categories:**

- **`boot.log`**: Initialization and configuration loading
- **`bwrap/`**: Bubblewrap sandbox operations
- **`daemon/`**: Portal daemon logs (separate for host/guest modes)
- **`dispatcher/`**: Portal dispatcher logs
- **`fuse/`**: Filesystem operations (DwarFS, overlays, casefold)

### Mount Directory

The `mount/` directory serves as the **merged root filesystem** for the container. It combines:

1. Base DwarFS layer(s)
2. Committed layers
3. Writable overlay
4. Optional case-insensitive layer

This is what the container sees as its root `/`.

### Layers Directory

Contains mount points for individual DwarFS compressed layers:

- **Layer 0**: Base filesystem (always present)
- **Layer 1-N**: Committed layers created with `fim-layer commit`

Layers are stacked in order, with higher numbers taking precedence.

### Runtime Host Directory

The `run/host/` directory provides **read-only access** to the host filesystem from within the container. This is used internally by FlatImage for:

- GPU permission: Symlinks to NVIDIA drivers (`/usr/lib`, `/usr/bin`)
- Library access: Accessing host libraries when needed
- Socket access: Connecting to host services

Referenced by: `FIM_DIR_RUNTIME_HOST`

This directory is **NOT** visible from the host - only from inside the container.

## Host Data Directory

FlatImage creates a `.{BINARY_NAME}.data/` directory alongside the binary for persistent storage.

**Example:** If your binary is named `firefox.flatimage`, the data directory is `.firefox.flatimage.data/`

Referenced by: `FIM_DIR_DATA`

### Data Directory Structure

```
.{BINARY_NAME}.data/
├── tmp/           - Temporary files
├── work/{PID}/    - Overlay work directory (per-instance)
├── data/          - Overlay upper layer (persistent changes)
├── casefold/      - Case-insensitive mount point
└── recipes/       - Package recipe JSON files
```

**Purposes:**

- **`tmp/`**: Temporary files that can be safely deleted
- **`work/`**: Required by bwrap/overlayfs for metadata (deleted on exit)
- **`data/`**: Writable layer for persistent changes before `fim-layer commit`
- **`casefold/`**: Mount point when case-insensitivity is enabled
- **`recipes/`**: Downloaded package recipe definitions

## Application ID Format

The application directory name follows the format: `{COMMIT}_{TIMESTAMP}`

- **COMMIT**: First 7 characters of git commit hash (from `FIM_COMMIT`)
- **TIMESTAMP**: Build time in `YYYYMMDDHHMMSS` format (from `FIM_TIMESTAMP`)

This ensures:

1. Each build is uniquely identifiable
2. Multiple versions can coexist in `/tmp/fim/app/`
3. Binaries are shared across instances of the same build
4. Different builds don't interfere with each other

## Environment Variable Summary

| Environment Variable | Path | Description |
|---------------------|------|-------------|
| `FIM_DIR_GLOBAL` | `/tmp/fim` | Global temporary directory |
| `FIM_DIR_APP` | `/tmp/fim/app/{COMMIT}_{TIMESTAMP}` | Application directory |
| `FIM_DIR_APP_BIN` | `{FIM_DIR_APP}/bin` | Application binaries |
| `FIM_DIR_APP_SBIN` | `{FIM_DIR_APP}/sbin` | System binaries |
| `FIM_DIR_INSTANCE` | `{FIM_DIR_APP}/instance/{PID}` | Instance directory |
| `FIM_DIR_RUNTIME` | `/tmp/fim/run` | Runtime directory |
| `FIM_DIR_RUNTIME_HOST` | `/tmp/fim/run/host` | Host filesystem access |
| `FIM_DIR_DATA` | `{BINARY_DIR}/.{BINARY_NAME}.data` | Host-side data directory |