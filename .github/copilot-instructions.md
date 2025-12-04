# FlatImage AI Agent Instructions

**Quick Start**: FlatImage packages Linux apps into single sandboxed executables using C++23, static linking, compressed DwarFS layers, and 4MB ELF-embedded configuration.

**Repository**: https://github.com/ruanformigoni/flatimage  
**Docs**: https://flatimage.github.io/docs | https://flatimage.github.io/docs-developer  
**Last Updated**: November 13, 2025

---

## Critical Patterns for AI Agents

### Error Handling (MANDATORY)

**Never use exceptions** - Use `Value<T>` (aliased `std::expected<T, std::string>`):

```cpp
// Function declaration
Value<int> divide(int a, int b) {
  return_if(b == 0, Error("E::Division by zero"));
  return a / b;
}

// Pop macro: unwrap or return error (use in Value<T> functions)
Value<int> compute() {
  int result = Pop(divide(10, 2));  // Auto-propagates on error
  return result * 2;
}

// Try macro: unwrap or return std::nullopt (use in std::optional functions)
std::optional<int> try_compute() {
  int result = Try(divide(10, 2));  // Returns nullopt on error
  return result * 2;
}
```

**Key macros** (`src/macro.hpp`):
- `Pop(expr)` - Unwrap `Value<T>` or return error with logging
- `Try(expr)` - Unwrap `Value<T>` or return `std::nullopt`  
- `return_if(cond, value, "E::message")` - Conditional return with optional logging
- `Error("E::msg", args...)` - Create `Unexpected` error value
- `.discard("W::msg")` - Ignore error but log if present
- `.forward("E::msg")` - Re-wrap error with additional context

### Logging Pattern

**Always** use `logger()` macro with level prefix in message:

```cpp
logger("I::Starting application");           // Info
logger("E::Failed to open {}", path);       // Error  
logger("W::Deprecated function used");      // Warning
logger("D::Variable value: {}", var);       // Debug (only if FIM_DEBUG=1)
logger("C::Critical failure");              // Critical
```

Levels: `D::` (debug), `I::` (info), `W::` (warning), `E::` (error), `C::` (critical)

### Namespace Convention

**All namespaces** use `ns_` prefix:
- `ns_log`, `ns_parser`, `ns_bwrap`, `ns_filesystems`, `ns_reserved`, etc.
- Nested: `ns_parser::ns_cmd`, `ns_filesystems::ns_controller`

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Build System](#build-system)
3. [Adding Commands](#adding-commands)
4. [Reserved Space System](#reserved-space-system)
5. [Filesystem Stack](#filesystem-stack)
6. [Testing](#testing)
7. [Key Files Reference](#key-files-reference)

---

## Architecture Overview

### Execution Flow

```
boot.cpp main()
  ↓
Relocate to /tmp/fim-<hash> if not there
  ↓
Parse command line (parser/parser.hpp)
  ↓
If fim-* command → Execute & exit
  ↓
Else: Setup filesystem stack
  ↓
Mount DwarFS layers (read-only base)
  ↓
Mount overlay (BWRAP/OverlayFS/UnionFS for RW)
  ↓
If casefold enabled → Mount CIOPFS on top
  ↓
Launch Portal daemon (host-side IPC)
  ↓
Execute bwrap sandbox with permissions
  ↓
Run application inside container
```

### Binary Structure (Single ELF File)

```
┌─────────────────────────────────┐
│ Boot Executable                 │ ← Main C++ program
├─────────────────────────────────┤
│ Embedded Tools (size+data)      │ ← fim_portal, bwrap, dwarfs, etc.
├─────────────────────────────────┤
│ Reserved Space (4MB)            │ ← Permissions, env, boot cmd, icon
├─────────────────────────────────┤
│ DwarFS Layer 0 (8 bytes + data) │ ← Base OS (Alpine/Arch)
│ DwarFS Layer 1 (8 bytes + data) │ ← User changes (if committed)
│ DwarFS Layer N...               │ ← Additional layers
└─────────────────────────────────┘
```

**Key**: `FIM_RESERVED_OFFSET` symbol points to reserved space start.

---

## Build System

### Docker Build (Recommended)

```bash
cd deploy
./flatimage.sh alpine      # MUSL-based Alpine subsystem
./flatimage.sh arch        # GNU-based Arch subsystem  
./flatimage.sh blueprint   # Empty container
```

**Output**: `dist/{alpine,arch,blueprint}.flatimage`

### Required CMake Variables

Set these before configuring:
```bash
export FIM_DIST=ALPINE                        # ALPINE, ARCH, or BLUEPRINT
export FIM_RESERVED_SIZE=4194304              # 4MB (do not change)
export FIM_FILE_TOOLS=/flatimage/build/tools.json
export FIM_FILE_META=/flatimage/build/meta.json
```

### CMake Targets

```bash
cmake -DFIM_TARGET=boot -S . -B build        # Build main executable
cmake -DFIM_TARGET=doxygen -S . -B build     # Generate API docs
cmake -DFIM_TARGET=mkdocs -S . -B build      # Generate user docs
```

**Critical**: After building, run `./build/magic ./build/boot` to patch magic bytes.

---

## Adding Commands

**Complete workflow** for new `fim-mycommand`:

### 1. Create Implementation (`src/parser/cmd/mycommand.hpp`)

```cpp
namespace ns_parser::ns_cmd {

struct CmdMyCommand {
  Value<void> operator()(ns_config::FlatimageConfig& config, 
                         ns_parser::VecArgs& args) {
    // Pop arguments
    std::string action = Pop(args.pop_front<"E::Missing action">());
    
    // Implement logic
    if (action == "set") {
      std::string value = Pop(args.pop_front<"E::Missing value">());
      logger("I::Setting value: {}", value);
      // Write to reserved space or database
      return {};
    }
    
    return Error("E::Unknown action: {}", action);
  }
};

} // namespace
```

### 2. Register in Parser (`src/parser/parser.hpp`)

```cpp
enum class FimCommand {
  // ...
  MYCOMMAND,  // ← Add here
};

Value<FimCommand> fim_command_from_string(std::string_view str) {
  // ...
  if (str == "fim-mycommand") return FimCommand::MYCOMMAND;  // ← Add here
  // ...
}
```

### 3. Add Executor Case (`src/parser/executor.hpp`)

```cpp
case FimCommand::MYCOMMAND: {
  ns_cmd::CmdMyCommand{}(config, vec_args).discard("E::Command failed");
  break;
}
```

### 4. Add Help Entry (`src/parser/cmd/help.hpp`)

```cpp
HelpEntry{"fim-mycommand"}
  .with_synopsis("Brief description")
  .with_usage("fim-mycommand <set|get> [args...]")
  .with_description("Detailed explanation of what this command does")
  .with_example(R"(fim-mycommand set value)")
  .with_note("Important usage notes")
```

### 5. Add User Documentation (`doc/mkdocs/docs/cmd/mycommand.md`)

Create markdown file with examples and detailed usage.

---

## Reserved Space System

**4MB ELF-embedded configuration** allowing post-build reconfiguration without recompilation.

### Layout (src/reserved/*.hpp)

| Offset | Size | File | Purpose |
|--------|------|------|---------|
| 0 | 8 B | `permissions.hpp` | Permission bitmask (15 types) |
| 8 | 1 B | `notify.hpp` | Desktop notification flag |
| 9 | 1 B | `overlay.hpp` | Filesystem type (0=BWRAP, 1=OVERLAYFS, 2=UNIONFS) |
| 10 | 1 B | `casefold.hpp` | Case-insensitive FS flag |
| 11 | 4 KiB | `desktop.hpp` | Desktop integration JSON |
| 4107 | 8 KiB | `boot.hpp` | Boot command JSON |
| 12299 | 1 MiB | `icon.hpp` | Application icon |
| 1060875 | 1 MiB | `env.hpp` | Environment variables JSON |
| 2109451 | 1 MiB | `bind.hpp` | Bind mounts JSON |
| 3158027 | 4 KiB | `remote.hpp` | Recipe repository URL |

### Read/Write Pattern

**All handlers** use `ns_reserved::read()` and `ns_reserved::write()`:

```cpp
// Read from reserved space
Value<uint8_t> ns_reserved::ns_casefold::read(fs::path const& path_file_binary) {
  uint8_t buffer[1];
  Pop(ns_reserved::read(path_file_binary, OFFSET_BEGIN, OFFSET_END, buffer, sizeof(buffer)));
  return buffer[0];
}

// Write to reserved space
Value<void> ns_reserved::ns_casefold::write(fs::path const& path_file_binary, uint8_t is_casefold) {
  return ns_reserved::write(path_file_binary, OFFSET_BEGIN, OFFSET_END, 
                           reinterpret_cast<char const*>(&is_casefold), sizeof(uint8_t));
}
```

**Offsets** are defined as constants in each handler file.

---

## Filesystem Stack

### Layer Orchestration (`src/filesystems/controller.hpp`)

```cpp
// Mount order (bottom to top):
1. DwarFS layer 0 → /mount/layers/0  (base OS, read-only)
2. DwarFS layer 1+ → /mount/layers/N (user changes, read-only)
3. Overlay → /mount/fuse             (copy-on-write: BWRAP/OverlayFS/UnionFS)
4. CIOPFS → /config/casefold         (case-insensitive, optional)

// Final mount point used by application:
is_casefold ? /config/casefold : /mount/fuse
```

### Compatibility Matrix

| Overlay Type | Speed | Casefold Support | Use Case |
|--------------|-------|------------------|----------|
| BWRAP | Fastest | ❌ No | Default (no case folding) |
| OverlayFS | Fast | ❌ No | Performance (no Wine/Proton) |
| UnionFS | Medium | ✅ Yes | Compatibility + Wine/Proton |

**Auto-fallback**: If casefold enabled and BWRAP selected, auto-switches to UnionFS.

### Filesystem Implementations

**DwarFS** (`src/filesystems/dwarfs.hpp`):
- Compressed read-only layers embedded in binary
- Magic: "DWARFS" (6 bytes)
- Uses offset/size from binary structure

**OverlayFS** (`src/filesystems/overlayfs.hpp`):
- fuse-overlayfs implementation
- Requires work directory for state tracking
- Best performance (lowest CPU overhead)

**UnionFS** (`src/filesystems/unionfs.hpp`):
- unionfs-fuse implementation  
- Compatible with CIOPFS stacking
- Medium performance

**CIOPFS** (`src/filesystems/ciopfs.hpp`):
- Case-insensitive overlay (Wine/Proton support)
- Stacks over other FUSE filesystems
- Not case-preserving (filenames converted to lowercase)

---

## Testing

### CLI Integration Tests

```bash
cd test/cli

# Setup
export FILE_IMAGE_SRC=/path/to/alpine.flatimage
export FILE_IMAGE=/tmp/test.flatimage
export DIR_IMAGE=/tmp/test

# Run all tests
python3 test.py <source-image>

# Run specific test module
python3 -m unittest bindings.TestFimBind
python3 -m unittest permissions.TestFimPerms
```

**Test modules** (`test/cli/*.py`):
- `bindings.py` - fim-bind mount configuration
- `boot.py` - fim-boot default command
- `casefold.py` - fim-casefold case-insensitive FS
- `desktop.py` - fim-desktop XDG integration
- `environment.py` - fim-env variable management
- `exec.py` - fim-exec user execution
- `instance.py` - fim-instance multi-instance
- `layer.py` - fim-layer compression
- `overlay.py` - fim-overlay FS selection
- `permissions.py` - fim-perms sandbox control
- `portal.py` - Portal IPC system
- `recipe.py` - fim-recipe package installation
- `remote.py` - fim-remote repository URL
- `root.py` - fim-root root execution
- `version.py` - fim-version info display

### Test Pattern

Each test:
1. Copies source image to temporary location
2. Modifies configuration via fim-* commands
3. Verifies changes via get/list commands
4. Cleans up `.app.flatimage.config` directory

---

## Key Files Reference

### Entry Points

- **`src/boot/boot.cpp`** - Main entry point, relocation, command routing
- **`src/boot/relocate.hpp`** - Binary self-relocation to /tmp/fim-<hash>

### Core Systems

- **`src/config.hpp`** - `FlatimageConfig` structure (paths, flags, user info)
- **`src/macro.hpp`** - Error handling macros (Pop, Try, return_if, Error)
- **`src/std/expected.hpp`** - `Value<T>` wrapper around std::expected

### Command System

- **`src/parser/parser.hpp`** - Command enum and string parsing
- **`src/parser/executor.hpp`** - Command dispatcher with filesystem setup
- **`src/parser/interface.hpp`** - Abstract command interface
- **`src/parser/cmd/*.hpp`** - Individual command implementations

### Subsystems

- **`src/bwrap/bwrap.hpp`** - Bubblewrap sandbox configuration
- **`src/filesystems/controller.hpp`** - Filesystem stack orchestration
- **`src/portal/portal.hpp`** - Portal daemon spawning
- **`src/portal/portal_daemon.cpp`** - Host-side IPC daemon
- **`src/portal/portal_dispatcher.cpp`** - Container-side IPC client
- **`src/janitor/janitor.cpp`** - Cleanup process for failed mounts
- **`src/magic/magic.cpp`** - Magic byte patcher (post-build)

### Libraries

- **`src/lib/log.hpp`** - Logging with level-based filtering
- **`src/lib/subprocess.hpp`** - Process spawning with I/O redirection
- **`src/lib/env.hpp`** - Environment variable utilities
- **`src/lib/elf.hpp`** - ELF binary parsing
- **`src/lib/image.hpp`** - Icon resizing/conversion (ImageMagick wrapper)

### Databases

- **`src/db/env.hpp`** - Environment variable JSON storage
- **`src/db/boot.hpp`** - Boot command JSON storage  
- **`src/db/bindings.hpp`** - Bind mount JSON storage
- **`src/db/desktop.hpp`** - Desktop integration JSON storage
- **`src/db/recipe.hpp`** - Recipe system implementation

---

## Project Overview

### What is FlatImage?

FlatImage is a **containerization and sandboxing utility** that packages complete applications—code, dependencies, and configuration—into a **single executable file** that runs sandboxed on any Linux distribution. It combines static linking, compressed filesystem layers, bubblewrap sandboxing, and persistent ELF-embedded configuration to achieve true portability across Linux distributions.

### Key Features

- **🔒 Sandboxed by Default**: Granular permissions (network, GPU, home, audio, etc.). Default: zero access, fully isolated.
- **📦 Self-Contained**: All configuration embedded in the ELF binary's 4MB reserved space.
- **⚡ Fast & Compact**: DwarFS compression delivers high compression ratios with on-the-fly decompression.
- **✨ Truly Portable**: Static linking + embedded tools. One file runs on any Linux distro without dependencies.
- **🔧 Reconfigurable After Build**: Change permissions, environment, boot commands, and bindings post-creation without rebuilding.
- **🗂️ Multiple Filesystem Backends**: Switch between OverlayFS (fast), UnionFS (compatible), BWRAP (native), or CIOPFS (case-insensitive) at runtime.
- **🎮 Case-Insensitive Filesystem**: Windows-style case folding for Wine/Proton compatibility. Toggle with `fim-casefold on`.
- **🔌 Portal IPC System**: Transparent host-guest communication. Execute host commands from containers with full I/O redirection.
- **🧱 Layered Architecture**: Stack compressed layers with `fim-layer commit`. Copy-on-write, incremental builds, immutable base layers.
- **📦 Package Recipes**: Install curated package sets with dependency resolution: `fim-recipe install gpu,audio,xorg`.
- **🔗 Runtime Bind Mounts**: Map host paths dynamically without rebuilding: `fim-bind add rw '$HOME/Documents' /Documents`.
- **🚀 Multi-Instance Support**: Run multiple isolated instances simultaneously.
- **🖥️ Desktop Integration**: Auto-generated menu entries, MIME types, and icons. Paths auto-update on file moves.

### Supported Distributions

FlatImage can create containers based on:

- **Alpine Linux** (MUSL-based) - Complete subsystem with `apk` package manager
- **Arch Linux** (GNU-based) - Complete subsystem with `pacman` package manager
- **Blueprint** - Empty FlatImage for custom Linux subsystems

### Documentation

- **User Documentation**: https://flatimage.github.io/docs
- **Developer Documentation**: https://flatimage.github.io/docs-developer

---

## Architecture

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        boot/boot.cpp                        │
│                  (Main Entry Point)                         │
└──────────────────────┬──────────────────────────────────────┘
                       │
          ┌────────────┼────────────┬─────────────┐
          ▼            ▼            ▼             ▼
     ┌─────────┐ ┌──────────┐ ┌─────────┐ ┌───────────┐
     │ Config  │ │  Parser  │ │Reserved │ │ Executor  │
     │ System  │ │ Commands │ │  Space  │ │ Filesys   │
     └─────────┘ └──────────┘ └─────────┘ └───────────┘
          │            │            │             │
          └────────────┴────────────┴─────────────┘
                       │
          ┌────────────┼────────────┬─────────────┐
          ▼            ▼            ▼             ▼
     ┌─────────┐ ┌──────────┐ ┌─────────┐ ┌───────────┐
     │  BWRAP  │ │   FUSE   │ │ Portal  │ │  Desktop  │
     │Sandbox  │ │ Filesys  │ │   IPC   │ │   Integ   │
     └─────────┘ └──────────┘ └─────────┘ └───────────┘
```

### Execution Flow

1. **Boot** → Parse reserved space config → Parse command line
2. **If command**: Execute command (fim-boot, fim-exec, etc.)
3. **Else**: Setup filesystems → Launch sandboxed application

### Project Structure

```
flatimage/
├── src/                      # C++23 source code
│   ├── boot/                 # Main entry point and relocation
│   ├── bwrap/                # Bubblewrap sandbox integration
│   ├── filesystems/          # Filesystem implementations (DwarFS, OverlayFS, UnionFS, CIOPFS)
│   ├── parser/               # Command-line parser and command implementations
│   ├── portal/               # Portal IPC system (daemon, dispatcher, child)
│   ├── db/                   # Runtime database (env, bindings, boot, desktop, recipe)
│   ├── reserved/             # Reserved ELF space handlers (permissions, overlay, casefold, etc.)
│   ├── xdg/                  # XDG desktop integration scripts
│   ├── lib/                  # Utility libraries (subprocess, logging, ELF, image processing)
│   ├── std/                  # C++ standard library extensions (expected, concepts, enums)
│   ├── janitor/              # Cleanup process for failed mounts
│   ├── magic/                # ELF magic byte patcher
│   └── config.hpp            # Main configuration structures
├── deploy/                   # Deployment and build scripts
├── scripts/                  # Helper scripts
├── cmake/                    # CMake configuration
├── docker/                   # Dockerfiles for builds
├── doc/                      # Documentation (Doxygen, MkDocs)
├── examples/                 # Example build scripts (Firefox, Steam)
├── test/                     # Test suites
├── sources/                  # Source lists for package managers
├── mime/                     # MIME type definitions
└── build/                    # Build output directory
```

---

## Core Components

### 1. Reserved Space System (4MB)

The **reserved space** is a 4MB block within the ELF binary that stores persistent configuration. This allows FlatImage to be self-contained with all settings embedded in the executable itself.

**Total Size**: 4,194,304 bytes (4MB)

#### Reserved Space Layout

| Offset | Size | Handler | Purpose |
|--------|------|---------|---------|
| 0 | 8 B | `reserved/permissions.hpp` | Sandboxing permissions (15 types as bitmask) |
| 8 | 1 B | `reserved/notify.hpp` | Desktop notification flag |
| 9 | 1 B | `reserved/overlay.hpp` | Filesystem type (BWRAP/OVERLAYFS/UNIONFS) |
| 10 | 1 B | `reserved/casefold.hpp` | Case-insensitive filesystem flag |
| 11 | 4 KiB | `reserved/desktop.hpp` | Desktop integration config (JSON) |
| 4,107 | 8 KiB | `reserved/boot.hpp` | Boot command + arguments (JSON) |
| 12,299 | 1 MiB | `reserved/icon.hpp` | Application icon (PNG/JPG/SVG) |
| 1,060,875 | 1 MiB | `reserved/env.hpp` | Environment variables (JSON) |
| 2,109,451 | 1 MiB | `reserved/bind.hpp` | Mount bindings (JSON) |
| 3,158,027 | 4 KiB | `reserved/remote.hpp` | Remote recipe repository URL |

**Remaining**: 1,032,181 bytes (24.6%) reserved for future expansion

#### Key Files

- **`reserved/reserved.hpp`**: Low-level read/write functions for reserved space access
- **`reserved/permissions.hpp`**: 15 granular permission types as 64-bit bitmask
- **`reserved/overlay.hpp`**: Filesystem backend selection
- **`reserved/casefold.hpp`**: Case-insensitive filesystem toggle
- **`reserved/notify.hpp`**: Desktop notification configuration
- **`reserved/boot.hpp`**: Default boot command storage
- **`reserved/desktop.hpp`**: Desktop integration metadata
- **`reserved/icon.hpp`**: Application icon data
- **`reserved/env.hpp`**: Environment variable storage
- **`reserved/bind.hpp`**: Bind mount configuration
- **`reserved/remote.hpp`**: Recipe repository URL

### 2. Filesystem Controller

**Location**: `src/filesystems/controller.hpp`

Orchestrates the complete filesystem stack:

1. **DwarFS layers** (compressed base OS)
2. **Overlay** (BWRAP/OverlayFS/UnionFS for copy-on-write)
3. **Case folding** (CIOPFS for Windows compatibility)

#### Filesystem Selection Logic

```
User selects overlay type (BWRAP/OVERLAYFS/UNIONFS)
  ↓
If case folding enabled:
  ↓
  BWRAP → auto-switch to UNIONFS (incompatible)
  ↓
Mount DwarFS layers (base OS)
  ↓
Mount selected overlay (copy-on-write)
  ↓
If case folding: mount CIOPFS over overlay
  ↓
Final mount point ready for application
```

### 3. Bubblewrap Sandbox

**Location**: `src/bwrap/bwrap.hpp`

Provides process isolation using [Bubblewrap](https://github.com/containers/bubblewrap):

- **Namespace setup**: PID, mount, network, IPC, UTS, user namespaces
- **Mount configuration**: Bind mounts for resources based on permissions
- **UID/GID mapping**: User namespace configuration
- **AppArmor integration**: Profile application for additional security

#### Permission System (15 Types)

Stored as a 64-bit bitmask in reserved space:

| Permission | Description |
|------------|-------------|
| `home` | Access to $HOME directory |
| `media` | External storage/USB drives |
| `audio` | PulseAudio/PipeWire sockets |
| `wayland` | Wayland display server |
| `xorg` | X11 display server |
| `dbus_user` | Session D-Bus |
| `dbus_system` | System D-Bus |
| `udev` | Device monitoring |
| `usb` | USB devices |
| `input` | Input devices (keyboard, mouse) |
| `gpu` | GPU acceleration (DRI + NVIDIA) |
| `network` | Network access |
| `shm` | Shared memory |
| `optical` | CD/DVD drives |
| `dev` | All /dev devices (superuser) |

**Default**: Zero permissions (fully isolated sandbox)

### 4. Portal IPC System

**Location**: `src/portal/`

Provides FIFO-based inter-process communication between FlatImage instances and between containers and the host system.

#### Portal Architecture

```
Host System:
  Portal Daemon (runs in background)
    ├── FIFO: /tmp/fim/run/portal/0/stdout
    ├── FIFO: /tmp/fim/run/portal/0/stderr
    └── FIFO: /tmp/fim/run/portal/0/exit

Container (Instance 0):
  Application → fim_portal thunar ~/Desktop
    ↓
  Writes command to portal FIFO
    ↓
  Portal daemon spawns thunar on host
    ↓
  Output redirected back through FIFOs
    ↓
  Container receives host command output
```

#### Key Files

- **`portal/portal.hpp`**: Portal daemon (host-side)
- **`portal/child.hpp`**: Portal child process handler
- **`portal/config.hpp`**: Portal configuration structures
- **`portal/fifo.hpp`**: FIFO (named pipe) utilities
- **`portal/portal_daemon.cpp`**: Portal daemon implementation
- **`portal/portal_dispatcher.cpp`**: Command dispatcher

### 5. Command Parser

**Location**: `src/parser/`

Parses command-line arguments and routes to command handlers.

#### Key Files

- **`parser/parser.hpp`**: Command-line argument parser (17+ commands)
- **`parser/interface.hpp`**: Abstract command interface
- **`parser/executor.hpp`**: Command executor with filesystem setup
- **`parser/cmd/`**: Individual command implementations

### 6. Configuration System

**Location**: `src/config.hpp`

Central configuration structure (`FlatimageConfig`) containing:

- **Build information**: Version, commit, timestamp, distribution
- **Paths**: Binary, runtime directories, mount points, logs
- **User configuration**: UID, GID, home directory
- **Filesystem configuration**: Overlay type, case folding
- **Database paths**: Environment, bindings, boot, desktop, recipes
- **Daemon configuration**: Portal daemon paths and settings

### 7. Janitor Process

**Location**: `src/janitor/janitor.cpp`

Cleanup daemon that monitors the main process and unmounts filesystems if the parent fails to cleanup properly. Uses signals (SIGTERM, SIGPIPE) to coordinate with parent process.

### 8. Magic Byte Patcher

**Location**: `src/magic/magic.cpp`

Post-build utility that:
- Writes magic signature bytes ('F', 'I', 0x01) at offset 8 in ELF header
- Identifies the binary as a FlatImage
- Enables detection by file managers and MIME handlers

---

## Build System

### Build Requirements

- **C++ Compiler**: GCC with C++23 support (`-std=gnu++23`)
- **CMake**: 3.20 or higher
- **Build Type**: Static linking (`-static` flag)
- **Dependencies**: nlohmann_json, Boost, libturbojpeg, libpng, zlib

### Build Process

#### 1. Docker-Based Build (Recommended)

```bash
cd deploy
./flatimage.sh alpine      # Alpine Linux (MUSL)
./flatimage.sh arch        # Arch Linux (GNU)
./flatimage.sh blueprint   # Blueprint (empty)
```

#### 2. Manual Build

```bash
# Set environment variables
export FIM_DIST=ARCH                          # or ALPINE, BLUEPRINT
export FIM_RESERVED_SIZE=4194304              # 4MB
export FIM_FILE_TOOLS=/flatimage/build/tools.json
export FIM_FILE_META=/flatimage/build/meta.json

# Configure CMake
cmake -DFIM_TARGET=boot -S . -B build

# Build
cmake --build build

# Post-process
./build/magic ./build/boot
```

### CMake Structure

```
CMakeLists.txt              # Root CMake file
cmake/
  ├── Variables.cmake       # Variable validation functions
  ├── Doxygen.cmake        # Doxygen documentation generation
  └── MkDocs.cmake         # MkDocs documentation generation
```

#### CMake Targets

- **`boot`**: Build the main FlatImage boot executable
- **`doxygen`**: Generate Doxygen API documentation
- **`mkdocs`**: Generate MkDocs user documentation

### Build Artifacts

```
build/
├── boot                  # Main FlatImage executable
├── magic                 # Magic byte patcher utility
├── bin/
│   ├── fim_portal        # Portal client
│   ├── fim_portal_daemon # Portal daemon
│   ├── fim_bwrap_apparmor # Bubblewrap with AppArmor
│   ├── fim_janitor       # Cleanup daemon
│   ├── bash              # Bash shell
│   ├── busybox           # BusyBox utilities
│   ├── bwrap             # Bubblewrap
│   ├── ciopfs            # Case-insensitive filesystem
│   ├── dwarfs_aio        # DwarFS filesystem
│   ├── overlayfs         # OverlayFS implementation
│   ├── unionfs           # UnionFS implementation
│   └── magick            # ImageMagick for icon processing
├── meta.json             # Tool metadata
└── tools.json            # Tool list
```

### ELF Binary Structure

The final FlatImage binary has this structure:

```
┌──────────────────────────────────────┐
│ Boot Executable (ELF)                │  Variable size
├──────────────────────────────────────┤
│ Embedded Binaries (size + data)      │  Variable size
│   ├── fim_portal (8 bytes + binary)  │
│   ├── fim_portal_daemon              │
│   ├── fim_bwrap_apparmor             │
│   ├── fim_janitor                    │
│   ├── bash                           │
│   ├── busybox                        │
│   ├── bwrap                          │
│   ├── ciopfs                         │
│   ├── dwarfs_aio                     │
│   ├── overlayfs                      │
│   ├── unionfs                        │
│   └── magick                         │
├──────────────────────────────────────┤
│ Reserved Space (Configuration)       │  4 MB (fixed)
├──────────────────────────────────────┤
│ DwarFS Layer Size (8 bytes)          │  Fixed
├──────────────────────────────────────┤
│ DwarFS Compressed Filesystem(s)      │  Variable size
└──────────────────────────────────────┘
```

The `FIM_RESERVED_OFFSET` external symbol points to the start of the reserved space.

---

## Command Reference

FlatImage provides 17 commands (all prefixed with `fim-`):

### Configuration Commands

#### `fim-boot`
Set or get the default startup command.
```bash
./app.flatimage fim-boot set firefox
./app.flatimage fim-boot get
```

#### `fim-env`
Manage environment variables.
```bash
./app.flatimage fim-env add "VAR=value"
./app.flatimage fim-env del "VAR"
./app.flatimage fim-env list
./app.flatimage fim-env clear
```

#### `fim-bind`
Configure bind mounts.
```bash
./app.flatimage fim-bind add rw '$HOME/Documents' /Documents
./app.flatimage fim-bind add ro '/usr/share/fonts' /fonts
./app.flatimage fim-bind list
./app.flatimage fim-bind del /Documents
```

#### `fim-overlay`
Select overlay filesystem type.
```bash
./app.flatimage fim-overlay set overlayfs   # Fast (fuse-overlayfs)
./app.flatimage fim-overlay set unionfs     # Compatible (unionfs-fuse)
./app.flatimage fim-overlay set bwrap       # Native (bubblewrap)
./app.flatimage fim-overlay get
```

#### `fim-layer`
Manage filesystem layers.
```bash
./app.flatimage fim-layer commit            # Compress changes to new layer
./app.flatimage fim-layer list              # List all layers
```

#### `fim-casefold`
Enable/disable case-insensitive filesystem.
```bash
./app.flatimage fim-casefold on
./app.flatimage fim-casefold off
./app.flatimage fim-casefold get
```

#### `fim-perms`
Configure sandboxing permissions.
```bash
./app.flatimage fim-perms add network,audio,xorg
./app.flatimage fim-perms set gpu,wayland
./app.flatimage fim-perms del network
./app.flatimage fim-perms list
./app.flatimage fim-perms clear
```

#### `fim-notify`
Enable/disable startup notifications.
```bash
./app.flatimage fim-notify on
./app.flatimage fim-notify off
```

#### `fim-desktop`
Configure desktop integration.
```bash
./app.flatimage fim-desktop setup ./config.json
./app.flatimage fim-desktop enable icon,mimetype,entry
./app.flatimage fim-desktop disable icon
```

#### `fim-remote`
Set recipe repository URL.
```bash
./app.flatimage fim-remote set https://github.com/user/recipes
./app.flatimage fim-remote get
```

### Execution Commands

#### `fim-exec`
Execute command as regular user.
```bash
./app.flatimage fim-exec firefox
./app.flatimage fim-exec -- ls -la /
```

#### `fim-root`
Execute command as root.
```bash
./app.flatimage fim-root apk add firefox
./app.flatimage fim-root pacman -Syu
```

#### `fim-instance`
Manage multiple instances.
```bash
./app.flatimage fim-instance list
./app.flatimage fim-instance exec 1 firefox
```

### Information Commands

#### `fim-version`
Display version information.
```bash
./app.flatimage fim-version
```

#### `fim-help`
Display command help.
```bash
./app.flatimage fim-help
./app.flatimage fim-help fim-perms
```

### Recipe Commands

#### `fim-recipe`
Fetch and install package recipes.
```bash
./app.flatimage fim-recipe fetch
./app.flatimage fim-recipe install gpu,audio,xorg
./app.flatimage fim-recipe list
```

---

## Filesystem Implementations

### 1. DwarFS (Compressed Read-Only)

**Location**: `src/filesystems/dwarfs.hpp`

**Purpose**: Compressed read-only filesystem for base layers

**Features**:
- Excellent compression (10-20x ratio)
- On-the-fly decompression
- Supports offset/size specification for embedded layers
- Magic header: "DWARFS" (6 bytes)

**Use Case**: Embedding OS layers within the FlatImage binary

### 2. OverlayFS (fuse-overlayfs)

**Location**: `src/filesystems/overlayfs.hpp`

**Purpose**: Kernel-like overlay via FUSE

**Features**:
- Best performance (low CPU overhead)
- Copy-on-write semantics
- Supports UID/GID squashing
- Work directory for state tracking

**Use Case**: Performance-critical applications

**Compatibility**: Cannot be combined with CIOPFS

### 3. UnionFS (unionfs-fuse)

**Location**: `src/filesystems/unionfs.hpp`

**Purpose**: Pure FUSE union filesystem

**Features**:
- Medium performance (higher CPU than overlayfs)
- Broad compatibility
- Supports RW/RO layer specification
- Compatible with case folding (CIOPFS)

**Use Case**: Compatibility fallback, case folding support

### 4. BWRAP Native

**Location**: Implemented in `src/filesystems/controller.hpp` and `src/bwrap/bwrap.hpp`

**Purpose**: Bubblewrap's built-in overlay capability

**Features**:
- No external tool required
- Fast and simple
- Cannot be combined with CIOPFS

**Use Case**: Default mode when no case folding needed

### 5. CIOPFS (Case-Insensitive Overlay)

**Location**: `src/filesystems/ciopfs.hpp`

**Purpose**: Case-insensitive overlay filesystem

**Features**:
- Provides Windows-style case folding
- Transparent case conversion layer
- Stacks over other FUSE filesystems
- Limitation: Cannot be combined with BWRAP native overlays

**Use Case**: Wine applications, Windows compatibility

---

## Development Guidelines

### Code Style

- **Language**: C++23 with GNU extensions (`-std=gnu++23`)
- **Namespace Prefix**: All namespaces use `ns_` prefix (e.g., `ns_log`, `ns_parser`)
- **Error Handling**: Use `Expected<T>` (similar to Rust's `Result`) for fallible functions
- **Logging**: Use `logger()` macro with level prefixes:
  - `I::` - Info
  - `E::` - Error
  - `W::` - Warning
  - `D::` - Debug
  - `C::` - Critical
- **Documentation**: Doxygen comments for all public APIs
- **Concepts**: Use C++20 concepts for template constraints
- **Static Linking**: All binaries are statically linked

### Error Handling Pattern

FlatImage uses **`std::expected<T, E>`** throughout for explicit error handling:

```cpp
// Function that can fail
Expected<int> divide(int a, int b) {
  if (b == 0) {
    return Unexpected("Division by zero");
  }
  return a / b;
}

// Usage with Pop macro (unwraps or returns error)
Expected<int> compute() {
  int result = Pop(divide(10, 2));  // Unwraps to 5
  return result * 2;
}

// Usage with Try macro (returns std::optional)
std::optional<int> try_compute() {
  int result = Try(divide(10, 2));  // Returns std::nullopt on error
  return result * 2;
}

// Usage with error handling
auto result = divide(10, 0);
if (!result) {
  logger("E::Error: {}", result.error());
}
```

**Benefits**:
- No exceptions (better for static linking and embedded contexts)
- Explicit error propagation (visible in function signatures)
- Composable error handling (monadic operations)
- Better control flow (no hidden jumps)

### Useful Macros

**Location**: `src/macro.hpp`

```cpp
// Unwrap Expected<T> or return error
#define Pop(expr) /* ... */

// Unwrap Expected<T> or return std::nullopt
#define Try(expr) /* ... */

// Return error if condition is true
#define return_if(cond, error) /* ... */

// Discard result (log on error)
#define discard(msg) /* ... */

// Forward error with message
#define forward(msg) /* ... */

// Logging with automatic file/line
#define logger(fmt, ...) /* ... */
```

### Adding a New Command

1. **Create implementation** in `src/parser/cmd/mycommand.hpp`:
   ```cpp
   namespace ns_parser::ns_cmd {
     struct CmdMyCommand {
       // Implementation
     };
   }
   ```

2. **Add help text** in `src/parser/cmd/help.hpp`:
   ```cpp
   { "fim-mycommand", /* help entry */ }
   ```

3. **Register in parser** in `src/parser/parser.hpp`:
   ```cpp
   enum class FimCommand {
     // ...
     MYCOMMAND,
   };
   
   if (str == "fim-mycommand") return FimCommand::MYCOMMAND;
   ```

4. **Add executor case** in `src/parser/executor.hpp`:
   ```cpp
   case FimCommand::MYCOMMAND:
     // Execute command
     break;
   ```

5. **Add documentation** in `doc/mkdocs/docs/cmd/mycommand.md`

### Adding a New Filesystem Type

1. **Create class** inheriting from `Filesystem` in `src/filesystems/myfs.hpp`:
   ```cpp
   namespace ns_filesystems::ns_myfs {
     class MyFs final : public ns_filesystem::Filesystem {
       public:
         MyFs(/* params */);
         Value<void> mount() override;
         ~MyFs() override; // Auto-unmounts
     };
   }
   ```

2. **Implement mount/unmount** logic

3. **Update controller** in `src/filesystems/controller.hpp`:
   - Add selection logic
   - Update compatibility matrix

4. **Update reserved space handler** if persistent storage needed

### Library Utilities

#### Logging System (`src/lib/log.hpp`)

```cpp
// Set log level
ns_log::set_level(ns_log::Level::DEBUG);

// Log messages
logger("I::Starting application");
logger("E::Error: {}", error_message);
logger("D::Debug info: {} = {}", var_name, value);
```

**Levels**: DEBUG, INFO, WARN, ERROR, CRITICAL

#### Subprocess Execution (`src/lib/subprocess.hpp`)

```cpp
// Synchronous execution
auto result = ns_subprocess::Subprocess("/usr/bin/ls")
  .with_arg("-la")
  .with_arg("/home")
  .with_var("PATH", "/usr/bin")
  .spawn();

// Background process
auto child = ns_subprocess::Subprocess("/usr/bin/daemon")
  .with_daemon()
  .spawn();
```

#### ELF Binary Parsing (`src/lib/elf.hpp`)

```cpp
// Read ELF headers
auto elf_info = ns_elf::parse(path_to_binary);

// Locate sections
auto section = ns_elf::find_section(elf_info, ".fim_reserved_offset");
```

#### Image Processing (`src/lib/image.hpp`)

```cpp
// Resize icon
ns_image::resize(input_path, output_path, width, height);

// Convert format
ns_image::convert(png_path, jpg_path);
```

---

## Testing

### Test Structure

```
test/
├── cli/                    # CLI integration tests
│   ├── test.py            # Main test suite
│   ├── bindings.py        # fim-bind tests
│   ├── boot.py            # fim-boot tests
│   ├── casefold.py        # fim-casefold tests
│   ├── desktop.py         # fim-desktop tests
│   ├── environment.py     # fim-env tests
│   ├── exec.py            # fim-exec tests
│   ├── instance.py        # fim-instance tests
│   ├── layer.py           # fim-layer tests
│   ├── overlay.py         # fim-overlay tests
│   ├── permissions.py     # fim-perms tests
│   ├── portal.py          # Portal tests
│   ├── recipe.py          # fim-recipe tests
│   ├── remote.py          # fim-remote tests
│   ├── root.py            # fim-root tests
│   └── version.py         # fim-version tests
├── meta/                   # Metadata tests
└── subprocess/             # Subprocess tests
```

### Running Tests

#### CLI Tests

```bash
cd test/cli

# Set environment variables
export FILE_IMAGE_SRC=/path/to/alpine.flatimage
export FILE_IMAGE=/tmp/test.flatimage
export DIR_IMAGE=/tmp/test

# Run all tests
python3 test.py

# Run specific test
python3 -m unittest bindings.TestFimBind
```

#### Metadata Tests

```bash
cd test/meta
python3 test.py
python3 magic.py
python3 build_info.py
```

### Test Configuration

Tests use `.app.flatimage.config` directory for runtime configuration to avoid conflicts with production settings.

---

## Key Concepts

### 1. Self-Relocation

**Location**: `src/boot/relocate.hpp`

FlatImage relocates itself to `/tmp/fim-<hash>` on first run to:
- Ensure writable location for extraction
- Avoid permission issues
- Enable proper FUSE mounting

The relocation process:
1. Calculate SHA-256 hash of binary
2. Create `/tmp/fim-<hash>` directory
3. Copy binary to new location
4. Extract embedded tools
5. Re-execute from new location

### 2. Magic Bytes

**Location**: `src/magic/magic.cpp`

FlatImage binaries are identified by magic bytes at offset 8:
- `'F'` (0x46)
- `'I'` (0x49)
- `0x01`

This enables:
- File manager detection
- MIME type association
- Double-click execution

### 3. Layer System

Layers are compressed DwarFS filesystems stacked in order:
- Layer 0: Base OS (Alpine/Arch)
- Layer 1+: User modifications

Each layer is immutable. Changes are saved to a new layer with `fim-layer commit`.

**Advantages**:
- Incremental updates
- Version control
- Rollback capability
- Space efficiency (compression + deduplication)

### 4. Portal IPC

The portal enables containers to execute commands on the host:

```bash
# Inside container
fim_portal thunar ~/Desktop

# Thunar opens on host, accessing host's ~/Desktop
```

**Use Cases**:
- Desktop integration (file managers, terminals)
- Desktop notifications
- Host service access
- Multi-instance communication

### 5. Desktop Integration

FlatImage generates XDG-compliant desktop files:
- **Desktop entries**: `.desktop` files in `~/.local/share/applications`
- **MIME types**: MIME type handlers
- **Icons**: Multi-size icons in `~/.local/share/icons/hicolor`

Desktop files include absolute paths that auto-update when the binary is moved.

### 6. Recipe System

Recipes are JSON files defining package collections:

```json
{
  "name": "gpu",
  "description": "GPU acceleration support",
  "dependencies": ["xorg"],
  "alpine": ["mesa", "mesa-dri-gallium"],
  "arch": ["mesa", "lib32-mesa"]
}
```

Recipes support:
- Dependency resolution
- Distribution-specific packages
- Local caching
- Cycle detection

### 7. Multi-Instance Support

Multiple FlatImage instances can run simultaneously:
- Each instance has unique ID (0, 1, 2, ...)
- Separate portal FIFOs per instance
- Isolated filesystem mounts
- Instance-specific configuration

```bash
# List instances
./app.flatimage fim-instance list

# Execute in specific instance
./app.flatimage fim-instance exec 1 firefox
```

### 8. Binary Structure

The FlatImage binary embeds:
1. **Boot executable** (C++ entry point)
2. **Tool binaries** (portal, bwrap, filesystems)
3. **Reserved space** (4MB configuration)
4. **DwarFS layers** (compressed OS + changes)

All components are concatenated into a single executable file.

### 9. Static Linking

All FlatImage binaries are statically linked:
- No external dependencies
- Runs on any Linux kernel >= 3.10
- Distribution-agnostic
- Predictable behavior

Tools are downloaded from: https://github.com/flatimage/tools

### 10. Permissions vs Capabilities

- **Permissions** (FlatImage): User-facing security controls (network, audio, GPU, etc.)
- **Capabilities** (Linux): Kernel-level privilege controls (CAP_NET_ADMIN, CAP_SYS_ADMIN, etc.)

FlatImage permissions map to combinations of:
- Bind mounts (access to specific paths)
- Network namespaces
- Device access
- Environment variables

---

## Related Projects

- **[RunImage](https://github.com/VHSgunzo/runimage)**: Portable single-file Linux container in unprivileged user namespaces
- **[AppBundle](https://github.com/xplshn/pelf)**: The .AppImage alternative designed for Linux, BSDs and more
- **[NixAppImage](https://github.com/pkgforge/nix-appimage)**: Nix-based AppImages
- **[Flatpak](https://flatpak.org/)**: Linux application sandboxing and distribution framework
- **[Conty](https://github.com/Kron4ek/Conty)**: Container-based portable apps
- **[binctr](https://github.com/genuinetools/binctr)**: Fully static, unprivileged, self-contained containers as executable binaries
- **[nix-bundle](https://github.com/matthewbauer/nix-bundle)**: Bundle Nix derivations to run anywhere
- **[AppImage](https://appimage.org/)**: Linux apps that run anywhere
- **[exodus](https://github.com/Intoli/exodus)**: Painless relocation of Linux binaries and dependencies without containers
- **[statifier](https://statifier.sourceforge.net/)**: Tool for creating portable self-containing Linux executables
- **[bubblewrap](https://github.com/containers/bubblewrap)**: Low-level unprivileged sandboxing tool used by Flatpak
- **[Proot](https://github.com/proot-me/proot)**: chroot, mount --bind, and binfmt_misc without privilege/setup

---

## Contributing

### Workflow

1. **Fork** the repository
2. **Create** a feature branch
3. **Implement** changes following code style
4. **Add tests** for new functionality
5. **Update documentation** (Doxygen + MkDocs)
6. **Submit** pull request

### Code Review Checklist

- [ ] Code follows C++23 style guidelines
- [ ] All functions have Doxygen documentation
- [ ] Error handling uses `Expected<T>` pattern
- [ ] Logging uses `logger()` with appropriate levels
- [ ] Tests pass for affected functionality
- [ ] Documentation updated in `doc/mkdocs/`
- [ ] No compiler warnings with `-Wall -Wextra -Werror`
- [ ] Static analysis passes (if applicable)

### Bug Reports

Include:
- FlatImage version (`fim-version`)
- Distribution (Alpine/Arch)
- Host OS and kernel version
- Full command output with `FIM_DEBUG=1`
- Steps to reproduce

### Feature Requests

Include:
- Use case description
- Expected behavior
- Alternative solutions considered
- Compatibility concerns

---

## Additional Resources

- **Main Repository**: https://github.com/flatimage/flatimage (likely https://github.com/ruanformigoni/flatimage)
- **User Documentation**: https://flatimage.github.io/docs
- **Developer Documentation**: https://flatimage.github.io/docs-developer
- **Tools Repository**: https://github.com/flatimage/tools
- **Issue Tracker**: GitHub Issues
- **Examples**: `examples/firefox.sh`, `examples/steam.sh`

---

## Glossary

- **BWRAP**: Bubblewrap - Linux sandboxing tool
- **CIOPFS**: Case-Insensitive On Purpose Filesystem
- **DwarFS**: Compressed read-only filesystem with high compression ratios
- **ELF**: Executable and Linkable Format (Linux binary format)
- **FIFO**: First In First Out (named pipe for IPC)
- **FUSE**: Filesystem in Userspace
- **OverlayFS**: Filesystem that overlays multiple directories
- **Portal**: IPC mechanism for host-container communication
- **Reserved Space**: 4MB configuration block in ELF binary
- **UnionFS**: Filesystem that unions multiple directories
- **XDG**: X Desktop Group (standards for desktop integration)

---

**Last Updated**: November 13, 2025
**Maintainer**: Ruan E. Formigoni
**License**: Apache License 2.0
