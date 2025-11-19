# Architecture Overview

## What is FlatImage?

FlatImage is a **single-file container system** for Linux. Everything—code, dependencies, configuration, and filesystems—is packaged into one executable. No installation required, no external files needed.

**Key Features:**

- 📦 **Single file** - One executable contains everything
- 🔒 **Sandboxed** - Secure isolation with granular permissions
- ⚙️ **Reconfigurable** - Change settings after build without recompiling
- 🗜️ **Compressed** - DwarFS compression for efficient storage
- 🔌 **Integrated** - Desktop integration, portal IPC, Wine/Proton support

## Architecture Modules

FlatImage is organized into distinct modules, each handling a specific responsibility:

```mermaid
graph TB
    User[User] --> Boot[Boot Module]
    Boot --> Config[Config Module]
    Config --> FS[Filesystems Module]
    Config --> Portal[Portal Module]
    Config --> DB[DB Module]

    FS --> Janitor[Janitor Module]
    Portal --> Subprocess[Subprocess Module]

    Config --> Bwrap[Bwrap Module]
    Bwrap --> Container[Sandboxed Container]

    DB -.reads/writes.-> Reserved[Reserved Space]

    style Boot fill:#90EE90
    style Config fill:#87CEEB
    style Container fill:#FFB6C6
```

## 1. Boot Module

The boot module is the **entry point** for FlatImage execution. It handles initial setup and binary relocation.

```mermaid
flowchart LR
    A[User executes<br/>FlatImage] --> B{In /tmp?}
    B -->|No| C[Copy to /tmp<br/>Re-execute]
    B -->|Yes| D[Set metadata<br/>env vars]
    C --> D
    D --> E[Verify FUSE<br/>module]
    E --> F[Initialize<br/>configuration]
    F --> G[Continue to<br/>main boot]

    style A fill:#90EE90
    style G fill:#87CEEB
```

**Responsibilities:**

- Initialize logging system
- Set version/commit/distribution metadata
- Relocate binary to `/tmp` if needed (FUSE requirement)
- Verify FUSE kernel module is loaded
- Calculate filesystem offsets in binary

## 2. Config Module

The config module is the **central configuration object** that contains all runtime settings. Every other module depends on this.

```mermaid
graph LR
    Config[Config Module] --> Path[Path]
    Config --> Logs[Logs]
    Config --> Flags[Flags]
    Config --> ModuleConfig[Module Configs]

    Path --> Dirs[Directories<br/>/tmp/fim/...]
    Path --> Files[Files<br/>bashrc, passwd]
    Path --> Bins[Binaries<br/>bash, portal, janitor]

    Logs --> BootLog[boot.log]
    Logs --> FuseLogs[fuse/*.log]
    Logs --> PortalLogs[daemon/*.log]

    Flags --> Casefold[Casefold<br/>on/off]
    Flags --> Overlay[Overlay Type<br/>unionfs/overlayfs/bwrap]

    ModuleConfig --> FSConfig[Filesystem<br/>Controller Config]
    ModuleConfig --> PortalConfig[Portal<br/>Daemon Config]

    style Config fill:#FFD700
```

**Responsibilities:**

- Define all filesystem paths (`FIM_DIR_*`)
- Configure logging for all subsystems
- Read flags from reserved space (casefold, overlay type)
- Set up module configurations
- Extract embedded binaries to `/tmp/fim/.../bin/`
- Set environment variables

## 3. Reserved Space Module

Reserved space is a **configuration storage area** embedded in the ELF binary. This enables post-build reconfiguration.

```mermaid
graph TD

    Binary[FlatImage Binary] --> ELF[ELF Header<br/>+ Code]
    ELF --> Reserved[Reserved Space<br/>~4 MB]
    Reserved --> DwarFS[DwarFS<br/>Layers]

    Reserved --> Perms[Permissions]
    Reserved --> Boot[Boot Command]
    Reserved --> Env[Environment]
    Reserved --> Bind[Bind Mounts]
    Reserved --> Desktop[Desktop Config]
    Reserved --> Icon[Icon]
    Reserved --> Other[Notify, Overlay,Casefold, Remote]

    style Reserved fill:#FFA500
```

**Responsibilities:**

- Store configuration data in the binary
- Provide read/write functions for configuration
- Validate space constraints at compile-time
- Enable `fim-*` commands to modify binary

**Configuration Components:**

- **Permissions** - Permission bitfield
- **Boot Command** - Default command
- **Environment** - Custom env vars
- **Bind Mounts** - Host-guest mounts
- **Icon** - PNG icon data
- **Desktop** - Desktop integration
- **Notify, Overlay, Casefold** - Flags

## 4. DB Module

The DB module provides safe wrappers around reserved space for complex configurations stored as JSON.

```mermaid
graph TB
    Commands[fim-* Commands] --> DB[DB Module]

    DB --> PermDB[db/permissions.hpp]
    DB --> BootDB[db/boot.hpp]
    DB --> EnvDB[db/env.hpp]
    DB --> BindDB[db/bind.hpp]
    DB --> DesktopDB[db/desktop.hpp]
    DB --> RecipeDB[db/recipe.hpp]
    DB --> PortalDB[db/portal/]

    PermDB -.reads/writes.-> Reserved[Reserved Space]
    BootDB -.reads/writes.-> Reserved
    EnvDB -.reads/writes.-> Reserved
    BindDB -.reads/writes.-> Reserved

    RecipeDB -.reads/writes.-> FileSystem[.flatimage.data/<br/>recipes/]

    style DB fill:#87CEEB
```

**Responsibilities:**

- Serialize/deserialize configuration to JSON
- Provide high-level APIs for configuration management
- Abstract away reserved space offsets
- Validate configuration data

## 5. Filesystems Module

The filesystems module **mounts and manages** the layered filesystem stack that provides the container's root.

```mermaid
graph TB
    Controller[Filesystem Controller] --> Mount1[Mount DwarFS Layers]
    Controller --> Mount2[Mount Overlay]
    Controller --> Mount3[Mount CIOPFS<br/>optional]
    Controller --> Spawn[Spawn Janitor]

    Mount1 --> Layer0[Layer 0<br/>Base]
    Mount1 --> Layer1[Layer 1<br/>Committed]
    Mount1 --> LayerN[Layer N<br/>External]

    Mount2 --> Union{Overlay Type?}
    Union -->|unionfs| UnionFS[UnionFS-FUSE]
    Union -->|overlayfs| OverlayFS[FUSE-OverlayFS]
    Union -->|bwrap| Native[Native Overlay]

    Mount3 --> CIOPFS[Case-Insensitive<br/>Layer]

    Layer0 --> Merged[Merged Root<br/>Filesystem]
    Layer1 --> Merged
    LayerN --> Merged
    UnionFS --> Merged
    OverlayFS --> Merged
    Native --> Merged
    CIOPFS --> Merged

    style Controller fill:#FFD700
    style Merged fill:#90EE90
```

**Responsibilities:**

- Mount DwarFS compressed layers
- Set up overlay filesystem (writable layer)
- Optionally mount CIOPFS for case-insensitivity
- Coordinate filesystem lifecycle
- Spawn janitor for cleanup

**Layer Stack:**
```
┌─────────────────────┐
│ CIOPFS (optional)   │ ← Case-insensitive
├─────────────────────┤
│ Overlay (writable)  │ ← Changes go here
├─────────────────────┤
│ Layer N             │ ← External/committed
│ Layer 1             │ ← Committed
│ Layer 0             │ ← Base
└─────────────────────┘
```

## 6. Janitor Module

The janitor is a **cleanup daemon** that ensures filesystems are properly unmounted if the parent process crashes.

```mermaid
sequenceDiagram
    participant Parent as FlatImage Process
    participant Janitor as Janitor Daemon
    participant FS as Mounted Filesystems

    Parent->>Janitor: spawn(parent_pid, mountpoints)
    Janitor->>Janitor: Store parent PID

    loop Monitor
        Janitor->>Parent: Check if alive (kill pid 0)
        Parent-->>Janitor: Still alive
    end

    Note over Parent: Process crashes

    Janitor->>Parent: Check if alive
    Parent-->>Janitor: No response

    Janitor->>FS: Unmount all filesystems
    FS-->>Janitor: Unmounted
    Janitor->>Janitor: Exit
```

**Responsibilities:**

- Monitor parent process health
- Unmount filesystems on parent death
- Prevent stale FUSE mounts
- Log cleanup operations

**Why It's Needed:**

- If FlatImage crashes, FUSE mounts remain
- Stale mounts can cause system issues
- Janitor ensures clean cleanup

## 7. Portal Module

The portal module provides **IPC between host and container** using FIFO-based message passing.

```mermaid
graph TB
    subgraph HostDaemon["Portal Daemon (Host Mode)"]
        HostLoop["Loop: Poll daemon.host.fifo<br/>for JSON requests"]
        HostFork["Fork child process"]
        HostExec["Execute on host<br/>with I/O via FIFOs"]

        HostLoop -->|Request received| HostFork
        HostFork --> HostExec
        HostExec -.Return to loop.-> HostLoop
    end

    subgraph GuestDaemon["Portal Daemon (Guest Mode)<br/>Optional"]
        GuestLoop["Loop: Poll daemon.guest.fifo<br/>for JSON requests"]
        GuestFork["Fork child process"]
        GuestExec["Execute in container<br/>with I/O via FIFOs"]

        GuestLoop -->|Request received| GuestFork
        GuestFork --> GuestExec
        GuestExec -.Return to loop.-> GuestLoop
    end

    FimPortal["fim_portal<br/>Inside Container"]
    FimInstance["fim-instance exec<br/>On Host"]

    FimPortal -->|Send JSON request| HostLoop
    HostExec -->|stdin/stdout/stderr<br/>via FIFOs| FimPortal

    FimInstance -->|Send JSON request| GuestLoop
    GuestExec -->|stdin/stdout/stderr<br/>via FIFOs| FimInstance

    style HostDaemon fill:#E8F5E9
    style GuestDaemon fill:#E3F2FD
    style FimPortal fill:#FFF3E0
    style FimInstance fill:#FFF3E0
```

**Responsibilities:**

- Enable host-container command execution
- Redirect stdin/stdout/stderr between processes
- Forward signals (Ctrl+C, etc.)
- Maintain FIFO communication channels

**How It Works:**

1. **Daemon** listens on FIFO for requests
2. **Dispatcher** sends JSON request to daemon
3. **Daemon** forks child process to execute command
4. **Child** redirects I/O through FIFOs
5. **Dispatcher** forwards I/O to/from caller
6. **Exit code** returned via FIFO

**Use Cases:**

- Execute host commands from container
- Execute container commands from host
- Multi-instance communication

## 8. Subprocess Module

The subprocess module provides **process management utilities** for spawning and controlling child processes.

```mermaid
graph LR
    Subprocess[Subprocess Builder] --> Args[with_args]
    Subprocess --> Env[with_env]
    Subprocess --> Log[with_log_file]
    Subprocess --> Cwd[with_cwd]

    Args --> Spawn{spawn}
    Env --> Spawn
    Log --> Spawn
    Cwd --> Spawn

    Spawn --> Child[Child Process<br/>Handle]

    Child --> PID[get_pid]
    Child --> Wait[wait]
    Child --> Kill[kill]
    Child --> Running[is_running]

    style Subprocess fill:#87CEEB
    style Child fill:#90EE90
```

**Responsibilities:**

- Build process execution configuration
- Fork and exec child processes
- Manage process lifetime
- Provide process control (kill, wait, status)

**Key Features:**

- Builder pattern for configuration
- RAII process management
- Signal handling
- Log file redirection

**Used By:**

- Portal daemon (spawning commands)
- Filesystem controller (spawning janitor)
- FUSE mounts (spawning dwarfs, overlayfs, etc.)

## 9. Bwrap Module

The bwrap module is a **wrapper around Bubblewrap** that creates the sandboxed container environment.

```mermaid
graph TB
    subgraph Input["Input Configuration"]
        Perms[Permissions Bitfield<br/>HOME, GPU, NETWORK, etc.]
        User[User Data<br/>uid/gid, name, home, shell]
        Binds[Bind Mounts<br/>RO/RW/DEV]
        Env[Environment Variables]
        Overlay[Overlay Config<br/>layers, upper, work]
    end

    subgraph BwrapClass["Bwrap Class"]
        direction TB
        BuildArgs["Build Arguments<br/>--bind, --dev-bind, --ro-bind<br/>--setenv, --uid, --gid<br/>--tmpfs, --proc, --dev"]

        ApplyPerms["Apply Permissions<br/>Loop through bitfield"]

        PermChecks["Permission Mappings:<br/><br/>HOME → --bind $HOME $HOME<br/>MEDIA → --bind /media /media<br/>AUDIO → --ro-bind /dev/snd<br/>WAYLAND → --ro-bind $XDG_RUNTIME_DIR<br/>XORG → --ro-bind /tmp/.X11-unix<br/>GPU → --dev-bind /dev/dri + NVIDIA symlinks<br/>NETWORK → --share-net<br/>DBUS_USER → --bind dbus user socket<br/>DBUS_SYSTEM → --bind dbus system socket<br/>UDEV → --ro-bind /run/udev<br/>INPUT → --dev-bind /dev/input<br/>USB → --dev-bind /dev/bus/usb<br/>SHM → --bind /dev/shm<br/>OPTICAL → --ro-bind /dev/sr*<br/>DEV → --dev /dev"]

        BuildArgs --> ApplyPerms
        ApplyPerms --> PermChecks
    end

    subgraph Execution["Execute Container"]
        SetupOverlay["Setup Filesystem:<br/>Native overlay OR<br/>--bind mount/ /"]

        SpawnPortal["Spawn Portal Daemon:<br/>nohup daemon & disown"]

        ExecCommand["Execute Command:<br/>bash -c 'program args'"]

        ErrorPipe["Error Pipe:<br/>syscall_nr, errno_nr"]

        SetupOverlay --> SpawnPortal
        SpawnPortal --> ExecCommand
        ExecCommand --> ErrorPipe
    end

    Perms --> ApplyPerms
    User --> BuildArgs
    Binds --> BuildArgs
    Env --> BuildArgs
    Overlay --> SetupOverlay

    PermChecks --> SetupOverlay

    style BwrapClass fill:#FFE4B5
    style Input fill:#E3F2FD
    style Execution fill:#E8F5E9
```

**Responsibilities:**

- Construct bubblewrap command line
- Apply permission-based filesystem binds
- Configure container namespaces
- Set user/group identity
- Apply environment variables
- Execute command in sandbox

**Isolation:**

- Default: Full isolation, no host access
- Opt-in: Each permission grants specific access
- Namespaces: Separate process, mount, IPC, network spaces

## Module Interaction Flow

Here's how the modules work together during a typical FlatImage execution:

```mermaid
sequenceDiagram
    participant User
    participant Boot
    participant Config
    participant Reserved
    participant DB
    participant FS as Filesystems
    participant Janitor
    participant Portal
    participant Bwrap
    participant Container

    User->>Boot: Execute FlatImage
    Boot->>Boot: Relocate to /tmp
    Boot->>Config: Create configuration

    Config->>Reserved: Read flags (casefold, overlay)
    Reserved-->>Config: Return flags

    Config->>Config: Extract binaries to /tmp/fim
    Config->>Config: Set environment variables

    Config->>FS: Initialize filesystem controller
    FS->>FS: Mount DwarFS layers
    FS->>FS: Mount overlay (unionfs/overlayfs/bwrap)
    FS->>FS: Mount CIOPFS (if casefold)
    FS->>Janitor: Spawn janitor daemon
    Janitor-->>FS: Running

    Config->>Portal: Spawn portal daemon
    Portal-->>Config: Running

    Config->>Boot: Configuration ready
    Boot->>DB: Parse command (fim-perms, fim-exec, etc.)

    alt Configuration Command
        DB->>Reserved: Write configuration
        Reserved-->>DB: Updated
        DB-->>User: Configuration saved
    else Container Command
        DB->>Bwrap: Set up container
        Bwrap->>Reserved: Read permissions
        Reserved-->>Bwrap: Permission list
        Bwrap->>DB: Read bind mounts
        DB-->>Bwrap: Bind list
        Bwrap->>DB: Read environment
        DB-->>Bwrap: Environment vars

        Bwrap->>Container: Execute in sandbox
        Container->>Portal: (Optional) Execute host commands
        Portal-->>Container: Command results
        Container-->>User: Exit code

        Bwrap->>FS: Destroy controller
        FS->>Janitor: Terminate
        FS->>FS: Unmount filesystems
    end
```

## Key Architectural Patterns

### 1. Central Configuration Object

The `FlatImage` struct in `config.hpp` acts as a **single source of truth** for all runtime configuration. Every module receives its configuration from this central object.

**Benefits:**

- Predictable initialization order
- No global state scattered across modules
- Easy to understand dependencies

### 2. Reserved Space for Persistence

Configuration is stored **inside the binary** rather than external files. This enables single-file portability while maintaining configurability.

**Benefits:**

- No external configuration files
- Configuration travels with binary
- Post-build reconfiguration

### 3. Layered Filesystem

Multiple **read-only compressed layers** with a **writable overlay** on top provides efficiency and flexibility.

**Benefits:**

- High compression ratios
- Incremental updates via layer commits
- Shared base layers
- Fast decompression

### 4. Process Isolation via Namespaces

Uses Linux **user namespaces** and **Bubblewrap** for unprivileged sandboxing with granular permissions.

**Benefits:**

- No root required
- Fine-grained access control
- Standard Linux isolation

### 5. FIFO-based IPC

The portal system uses **named pipes (FIFOs)** for simple, reliable host-container communication.

**Benefits:**

- Standard Unix IPC
- Full I/O redirection
- Signal forwarding
- No complex protocols