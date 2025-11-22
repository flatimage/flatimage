@page modules Modules

This section provides an organized overview of all headers grouped by functionality.
Click on the header links to view detailed API documentation.

---

[TOC]

# Core System {#mod_core}

## Configuration & Boot

- `src/config.hpp` - Global configuration structures and distribution enums.
  Defines `Flatimage` with all runtime paths, metadata, and build information.
- `src/common.hpp` - Common includes, type aliases, and shared utilities used
  across the codebase. Single point of inclusion for standard library headers.
- `src/macro.hpp` - Preprocessor macros for compile-time configuration and
  code generation. Includes helper macros for error handling and debugging.
- `src/boot/relocate.hpp` - Executable self-relocation and extraction. Handles copying the
  FlatImage binary to runtime directories and extracting embedded tools.

---

# Reserved Space System {#mod_reserved}

The reserved space is a **block within the ELF binary** that stores persistent configuration. This allows FlatImage to be self-contained with all settings embedded in the executable itself, surviving file moves and copies. The file `reserved/reserved.hpp` provides low-level read/write functions for reserved space access. Provides `read()` and `write()` primitives that all other reserved handlers use.

---

# Filesystem Implementations {#mod_filesystems}

FlatImage supports **4 different filesystem strategies** that can be selected at runtime
via `fim-overlay` command or `FIM_OVERLAY` environment variable.

## Core Filesystem Infrastructure

- `filesystems/filesystem.hpp` - Abstract base class defining the filesystem interface.
  All filesystem implementations inherit from this and implement `mount()` and destructor
  for automatic unmounting. Provides RAII semantics for mount lifecycle.

- `filesystems/controller.hpp` - **Orchestrates the complete filesystem stack**.
  - Handles mounting sequence: DwarFS layers → Overlay (BWRAP/OverlayFS/UnionFS) -> Case folding.
  - Includes janitor process for cleanup and automatic fallback logic (e.g., BWRAP+casefold -> UNIONFS).
  - This is the main entry point for filesystem operations.

## Overlay Implementations

- `filesystems/overlayfs.hpp` - **OverlayFS (fuse-overlayfs)**: Kernel-like overlay via FUSE.
  - Good performance, copy-on-write semantics.
  - Compatible with case folding (CIOPFS).
  - Supports UID/GID squashing and work directory for state tracking.
  - [Buggy](https://github.com/containers/fuse-overlayfs/issues/432) on multiple layers due to [this commit](https://github.com/containers/fuse-overlayfs/commit/6a0de4a5b0f7845ab2ec3e242c1a8e6d68168dd6).

- `filesystems/unionfs.hpp` - **UnionFS (unionfs-fuse)**: Pure FUSE union filesystem.
  - Medium performance (higher CPU usage than overlayfs), but more compatible.
  - Compatible with case folding (CIOPFS).
  - Use case: Priority compatibility fallback of FlatImage.

- `BWRAP Native` (implemented in controller): Bubblewrap's built-in overlay capability.
  - No external tools required, but cannot be combined with CIOPFS. Fast and simple.
  - Use case: Default mode when no case folding needed.

## Specialized Filesystems

- `filesystems/dwarfs.hpp` - **DwarFS**: Compressed read-only filesystem for base layers.
  - Excellent compression.
  - Supports offset/size specification for embedded layers. Magic header: "DWARFS" (6 bytes).
  - Use case: Embedding OS layers within the FlatImage binary.

- `filesystems/ciopfs.hpp` - **CIOPFS**: Case-insensitive overlay filesystem.
  - Provides Windows-style case folding for Wine/Proton compatibility.
  - Transparent case conversion layer that stacks over other FUSE filesystems.
  - Limitation: Cannot be combined with BWRAP native overlays.
  - Use case: Wine applications, Windows compatibility.

- `filesystems/utils.hpp` - **Filesystem utilities**. Provides utility functions for managing instances and layers.
  - Includes path monitoring and instance state tracking.
  - Use case: Filesystem state queries and utility operations.

## Filesystem Selection Logic

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

---

# Sandboxing & Isolation {#mod_sandbox}

## Bubblewrap Integration

- `bwrap/bwrap.hpp` - **Bubblewrap wrapper with AppArmor support**. Provides process isolation primitives including namespace setup, mount configuration, UID/GID mapping, and AppArmor profile application. Builds bubblewrap command-line arguments from permission bitmask. Implements 15 permission types (home, network, audio, GPU, etc.) as bind mount collections.

---

# Command Parser & Execution {#mod_parser}

## Parser Infrastructure

- `parser/parser.hpp` - **Command-line argument parser**. Tokenizes arguments and routes to command handlers. Uses a registry pattern for command registration. Supports both direct execution (`fim-exec`, `fim-root`) and configuration commands (`fim-boot`, `fim-env`).

- `parser/interface.hpp` - Abstract command interface that all command implementations must follow. Defines the contract for command execution and help text generation.

- `parser/executor.hpp` - **Command executor**. Manages command lifecycle and filesystem setup before execution. Handles the transition from parsing to actual command execution, including environment preparation and cleanup.

## Command Implementations

All commands are implemented in `parser/cmd/` and follow a consistent pattern:

- `parser/cmd/help.hpp` - **Help system**. Generates usage documentation for all commands. Includes examples, arguments, and notes. Uses HelpEntry builder pattern for formatting.

- `parser/cmd/bind.hpp` - **Bind mount management** (`fim-bind`). Add/delete/list host path bindings (ro, rw, dev types). Stores configuration in reserved space (1 MiB JSON).

- `parser/cmd/desktop.hpp` - **Desktop integration** (`fim-desktop`). Setup menu entries, MIME types, and icons. Generates XDG-compliant `.desktop` files and icon sets. Supports entry, mimetype, and icon integrations. Paths auto-update on binary relocation.

- `parser/cmd/icon.hpp` - **Icon processing**. Handles PNG/JPG/SVG icon scaling to multiple sizes (16x16, 32x32, 48x48, 64x64, 96x96, 128x128, 256x256). Integrates with desktop integration system for hicolor icon theme installation.

- `parser/cmd/layers.hpp` - **Filesystem layer management** (`fim-layer`). Create compressed DwarFS layers, commit changes, and append layers to the FlatImage binary. Uses `mkdwarfs` with configurable compression levels (0-9). Layers are stacked with newest taking precedence.

- `parser/cmd/recipe.hpp` - **Package recipe system** (`fim-recipe`, `fim-remote`). Fetch and install curated package sets with recursive dependency resolution. Supports distribution-specific package managers (apk for Alpine, pacman for Arch). Includes cycle detection and local caching.

## Available Commands

```
Configuration:
  fim-boot      - Set default startup command
  fim-env       - Manage environment variables
  fim-bind      - Configure bind mounts
  fim-overlay   - Select overlay filesystem type
  fim-layer     - Manage filesystem layers
  fim-casefold  - Enable case-insensitive filesystem
  fim-perms     - Configure sandboxing permissions
  fim-notify    - Enable startup notifications
  fim-desktop   - Desktop integration setup
  fim-remote    - Set recipe repository URL

Execution:
  fim-exec      - Execute command as regular user
  fim-root      - Execute command as root
  fim-instance  - Manage multiple instances

Information:
  fim-version   - Display version information
  fim-help      - Display command help

Recipes:
  fim-recipe    - Fetch/install package recipes
```

---

# Portal IPC System {#mod_portal}

The portal provides **FIFO-based inter-process communication** between FlatImage instances and between containers and the host system.

## Portal Components

- `portal/portal.hpp` - **Portal daemon (host-side)**. Manages FIFO creation, process spawning, and output redirection. Runs on the host and accepts commands from containers via named pipes. Handles instance registration and command routing.

- `portal/child.hpp` - **Portal child process handler**. Manages individual command execution within the portal system. Creates child processes with redirected stdout/stderr to FIFOs. Handles process lifecycle and cleanup.

- `portal/config.hpp` - Portal configuration file.

- `portal/fifo.hpp` - **FIFO (named pipe) utilities**. Low-level FIFO creation, read, write, and cleanup operations. Handles non-blocking I/O and error conditions.

## Portal Database Components

- `db/portal/daemon.hpp` - Portal daemon database. Manages daemon-side state, logging, and process tracking. Handles background portal service operations and lifecycle.

- `db/portal/dispatcher.hpp` - Command dispatcher. Routes portal commands between containers and host system. Handles message queuing and command ordering.

- `db/portal/message.hpp` - Portal message format and serialization. Defines the message protocol for IPC communication, including encoding/decoding of portal commands.

---

# Runtime Database {#mod_database}

The runtime database stores FlatImage component configurations. These configuration are stored in the binary and immutable unless explicitly changed through FlatImage's configuration commands.

## Database Handlers

- `db/db.hpp` - **Main database interface**. Provides database CRUD operations.
- `db/env.hpp` - Runtime environment variable storage.
- `db/bind.hpp` - Runtime bind mount tracking.
- `db/boot.hpp` - Stores the current default boot command.
- `db/desktop.hpp` - Runtime desktop integration state.
- `db/recipe.hpp` - Parse recipes from external recipe files.
- `db/remote.hpp` - Store the remote URL.

## Database vs Reserved Space

| Feature | Reserved Space | Runtime Database |
|---------|---------------|------------------|
| Persistence | Permanent (in ELF) | Session only |
| Location | FlatImage binary | Runtime directories |
| Size | 4 MB total | No fixed limit |
| Purpose | Configuration | Transient state |
| Survives moves | ✅ Yes | ❌ No |

---

# Library Utilities {#mod_library}

Reusable utility modules used throughout the codebase.

## System Integration

- `lib/log.hpp` - **Thread-safe logging with fork safety**. Uses `thread_local` storage and `pthread_atfork()` to handle multi-threading and process forking correctly.  Supports 5 log levels (DEBUG, INFO, WARN, ERROR, CRITICAL) with compile-time dispatch.  Automatic file/line capture, sink file support, and format string utilities.

- `lib/subprocess.hpp` - **Process execution utilities**. Spawns child processes with proper error handling, output capture, and timeout support. Provides both synchronous and asynchronous execution modes. Includes pipe management for stdin/stdout/stderr redirection.

- `lib/subprocess/pipe.hpp` - Pipe creation and management for subprocess communication. Handles data forwarding between the parent and the child.

- `lib/linux.hpp` - **Linux syscall wrappers** and OS-specific utilities. Includes mount/umount wrappers, namespace operations, and low-level system calls not exposed by C++ standard library.

- `lib/env.hpp` - **Environment variable manipulation**. Utilities for getting, setting, and unsetting environment variables with proper error handling. Supports variable expansion and substitution.

## Binary & Filesystem

- `lib/elf.hpp` - **ELF binary parsing and manipulation**. Reads ELF headers, sections, and symbols. Used for locating the reserved space offset and patching binaries. Implements ELF64 format support for reading FlatImage metadata.

- `lib/fuse.hpp` - **FUSE filesystem utilities**. Detection of FUSE filesystems, mounting, unmounting, and status checking. Includes fusermount wrapper for proper FUSE cleanup.

- `lib/image.hpp` - **Image processing** (PNG/JPG). Resizes icons to multiple standard sizes for desktop integration. Uses libturbojpeg (JPEG) and libpng (PNG) for efficient encoding/decoding.

---

# Standard Library Extensions {#mod_std}

C++ standard library supplements and utilities.

## Type System Extensions

- `std/expected.hpp` - **`std::expected<T, E>` implementation** (Rust-like Result type).  Provides explicit error handling without exceptions. Used throughout FlatImage for functions that can fail. Includes monadic operations (`and_then`, `or_else`, `transform`).
- `std/concept.hpp` - **Concept definitions** for template constraints. Includes concepts for string representability, iterability, and other type requirements.
- `std/enum.hpp` - Enum class wrapper including string conversion, iteration, and reflection.

## String & Container Utilities

- `std/string.hpp` - **String manipulation utilities**. Includes `static_string` for compile-time strings and additional string utility functions.

- `std/vector.hpp` - Vector utilities including range operations, transformations, and vector-specific algorithms. Supplements std::vector with convenience methods.

- `std/filesystem.hpp` - Filesystem path utilities and extensions. Wraps std::filesystem with additional helpers for common path operations.