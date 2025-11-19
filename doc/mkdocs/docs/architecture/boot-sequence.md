# Boot Sequence

## Overview

When you execute a FlatImage binary, it goes through a carefully orchestrated boot sequence that transforms a single executable file into a fully functional, sandboxed container environment. This document explains the conceptual flow of how FlatImage starts up.

## High-Level Boot Flow

```mermaid
flowchart LR
    Start([User Executes<br/>FlatImage Binary]) --> Initialization

    subgraph Initialization["Binary Relocation"]
        direction TB
        I1{Binary<br/>in /tmp?}
        I2[Copy to /tmp<br/>and re-execute]
        I3[Initialize System]

        I1 -->|No| I2
        I1 -->|Yes| I3
        I2 --> I3
    end

    Initialization --> SystemSetup

    subgraph SystemSetup["System Setup"]
        direction TB
        S1[Load Configuration<br/>from Binary]
        S2[Mount Filesystems<br/>DwarFS layers]
        S3[Start Portal<br/>Daemon]
        S4[Parse Command<br/>Arguments]

        S1 --> S2
        S2 --> S3
        S3 --> S4
    end

    SystemSetup --> Router{Command<br/>Type?}

    Router -->|Config Change| ConfigPath
    Router -->|Execute Command| ExecutionPath

    subgraph ConfigPath["Configuration Path"]
        direction TB
        CP1[Modify Reserved<br/>Space in Binary]
    end

    subgraph ExecutionPath["Container Execution Path"]
        direction TB
        E1[Setup Container<br/>Environment]
        E2[Apply Permissions]
        E3[Apply Bind Mounts]
        E4[Set Environment<br/>Variables]
        E5[Launch Bubblewrap<br/>Container]
        E6[Run Command<br/>Inside Container]
        E7[Cleanup & Unmount<br/>Filesystems]

        E1 --> E2
        E2 --> E3
        E3 --> E4
        E4 --> E5
        E5 --> E6
        E6 --> E7
    end

    ConfigPath --> Done([Exit])
    ExecutionPath --> Done

    style Initialization fill:#E3F2FD
    style SystemSetup fill:#FFF3E0
    style Router fill:#FFD700
    style ConfigPath fill:#87CEEB
    style ExecutionPath fill:#FFE4B5
    style Start fill:#90EE90
    style Done fill:#FFB6C6
```

## Phase 1: Binary Relocation

**Purpose**: Ensure the binary runs from `/tmp` where FUSE filesystems can be properly mounted.

```mermaid
flowchart LR
    A[Binary executed<br/>from any location] --> B{Already<br/>in /tmp?}

    B -->|Yes| C[Continue to<br/>next phase]
    B -->|No| D[Copy binary<br/>to /tmp]

    D --> E[Re-execute from<br/>new location]
    E --> F[Original process<br/>exits]

    style A fill:#E3F2FD
    style C fill:#90EE90
    style F fill:#FFB6C6
```

**Why this is necessary:**

- FUSE filesystems require specific mount point permissions
- Simplifies permission handling for mounts
- Ensures consistent behavior regardless of where binary is stored

**What happens:**

1. Binary checks its own path
2. If not in `/tmp`, copies itself there
3. Executes the copy with the same arguments
4. Original process terminates
5. New process continues from `/tmp`

## Phase 2: System Initialization

**Purpose**: Set up the runtime environment and verify system requirements.

```mermaid
flowchart TD
    Init[System Initialization] --> Logger[Setup Logging<br/>System]

    Logger --> Metadata[Set Metadata<br/>Environment Variables]
    Metadata --> MetaDetails["FIM_VERSION<br/>FIM_COMMIT<br/>FIM_DIST<br/>FIM_TIMESTAMP"]

    MetaDetails --> CheckFUSE{FUSE Module<br/>Available?}

    CheckFUSE -->|No| Error([Error: FUSE<br/>not available])
    CheckFUSE -->|Yes| CalcOffset[Calculate Filesystem<br/>Offsets in Binary]

    CalcOffset --> Ready[System Ready]

    style Init fill:#E3F2FD
    style Ready fill:#90EE90
    style Error fill:#FFB6C6
```

**Key concepts:**

- **Logging**: Debug output controlled by `FIM_DEBUG` environment variable
- **Metadata**: Build-time information embedded in binary (version, commit hash, distribution type, timestamp)
- **FUSE verification**: Ensures kernel module is loaded for filesystem operations
- **Offset calculation**: Determines where DwarFS filesystems begin in the binary

## Phase 3: Configuration Loading

**Purpose**: Extract all configuration from the binary's reserved space and set up runtime paths.

```mermaid
flowchart LR
    Start([Configuration Loading]) --> ReservedSpace

    subgraph ReservedSpace["Reserved Space Reading"]
        direction TB
        R1[Read Reserved Space<br/>from Binary]
        R2["Permissions Bitfield<br/>━━━━━━━━━━━━━━<br/>Runtime Flags<br/>casefold, overlay type<br/>━━━━━━━━━━━━━━<br/>Boot Command<br/>Default executable<br/>━━━━━━━━━━━━━━<br/>Environment Variables<br/>━━━━━━━━━━━━━━<br/>Bind Mount Definitions"]

        R1 --> R2
    end

    ReservedSpace --> PathSetup

    subgraph PathSetup["Directory Structure"]
        direction TB
        P1[Build Directory<br/>Structure Paths]
        P2["Create Directories:<br/>/tmp/fim/app/BUILD_ID/<br/>instance/PID/"]

        P1 --> P2
    end

    PathSetup --> BinaryExtract

    subgraph BinaryExtract["Binary Extraction"]
        direction TB
        B1[Extract Embedded<br/>Binaries]
        B2["bash<br/>━━━━━━━━━━━━━━<br/>fim_janitor<br/>━━━━━━━━━━━━━━<br/>fim_portal<br/>━━━━━━━━━━━━━━<br/>fim_portal_daemon"]

        B1 --> B2
    end

    BinaryExtract --> EnvSetup

    subgraph EnvSetup["Environment Setup"]
        direction TB
        E1[Set Environment<br/>Variables]
        E2["FIM_DIR_GLOBAL<br/>━━━━━━━━━━━━━━<br/>FIM_DIR_APP<br/>━━━━━━━━━━━━━━<br/>FIM_DIR_INSTANCE<br/>━━━━━━━━━━━━━━<br/>...and more"]

        E1 --> E2
    end

    EnvSetup --> Ready([Configuration Ready])

    style ReservedSpace fill:#E3F2FD
    style PathSetup fill:#FFD700
    style BinaryExtract fill:#FFE4B5
    style EnvSetup fill:#E8F5E9
    style Start fill:#E3F2FD
    style Ready fill:#90EE90
```

**Configuration sources:**

1. **Reserved space** (3-4 MB embedded in binary)
   - Permissions, boot command, environment, binds, desktop integration
2. **Environment variables** (can override reserved space)
   - `FIM_OVERLAY`, `FIM_CASEFOLD`, `FIM_FILES_LAYER`, etc.
3. **Compiled defaults** (fallback values)

**Directory structure created:**

- Global directory: `/tmp/fim`
- Build-specific: `/tmp/fim/app/{COMMIT}_{TIMESTAMP}`
- Instance-specific: `/tmp/fim/app/.../instance/{PID}`
- Binaries extracted to: `/tmp/fim/app/.../bin/`

## Phase 4: Filesystem Setup

**Purpose**: Mount the layered filesystem stack that becomes the container's root.

```mermaid
flowchart LR
    Start([Filesystem Setup]) --> LayerMount

    subgraph LayerMount["DwarFS Layer Mounting"]
        direction TB
        L1[Mount DwarFS<br/>Compressed Layers]
        L2["Layer 0<br/>Base Filesystem<br/>━━━━━━━━━━━━━━<br/>Layer 1..N<br/>Committed Layers<br/>━━━━━━━━━━━━━━<br/>External Layers<br/>via FIM_FILES_LAYER"]

        L1 --> L2
    end

    LayerMount --> OverlaySetup

    subgraph OverlaySetup["Overlay Configuration"]
        direction TB
        O1{Select<br/>Overlay Type}
        O2["UnionFS-FUSE<br/>Writable Layer<br/>━━━━━━━━━━━━━━<br/>FUSE-OverlayFS<br/>Writable Layer<br/>━━━━━━━━━━━━━━<br/>Bubblewrap<br/>Native Overlay"]

        O1 --> O2
    end

    OverlaySetup --> CasefoldCheck

    subgraph CasefoldCheck["Case-Insensitive Layer"]
        direction TB
        C1{Casefold<br/>Enabled?}
        C2[Mount CIOPFS<br/>Case-Insensitive Layer]
        C3[Skip CIOPFS]

        C1 -->|Yes| C2
        C1 -->|No| C3
    end

    CasefoldCheck --> JanitorSetup

    subgraph JanitorSetup["Cleanup Daemon"]
        direction TB
        J1[Spawn Janitor<br/>Cleanup Daemon]
    end

    JanitorSetup --> Ready([Merged Root<br/>Filesystem Ready])

    style LayerMount fill:#E3F2FD
    style OverlaySetup fill:#FFF3E0
    style CasefoldCheck fill:#E8F5E9
    style JanitorSetup fill:#FFE4B5
    style Start fill:#E3F2FD
    style Ready fill:#90EE90
```

**Filesystem layers (bottom to top):**

1. **Base layer**: Core filesystem (Alpine/Arch/Blueprint)
2. **Committed layers**: Previously saved changes
3. **External layers**: Mounted from host filesystem
4. **Writable overlay**: Temporary changes (UnionFS/OverlayFS/BWRAP native)
5. **Case-insensitive layer** (optional): CIOPFS for Wine/Proton compatibility

**Janitor daemon:**

- Monitors parent process health
- Automatically unmounts filesystems if parent crashes
- Prevents stale FUSE mounts

## Phase 5: Portal Daemon Startup

**Purpose**: Enable inter-process communication between host and container.

```mermaid
flowchart TD
    Start([Portal Daemon<br/>Startup]) --> FIFOSetup

    subgraph FIFOSetup["FIFO Creation"]
        direction TB
        F1[Create FIFO<br/>Communication Channels]
        F2[daemon.host.fifo]
        F3[daemon.guest.fifo]

        F1 --> F2
        F1 --> F3
    end

    FIFOSetup --> DaemonSpawn

    subgraph DaemonSpawn["Daemon Processes"]
        direction TB
        D1[Spawn Host Daemon<br/>Outside Container]
        D2[Spawn Guest Daemon<br/>Inside Container]
    end

    DaemonSpawn --> PollingLoop

    subgraph PollingLoop["Request Handling"]
        direction TB
        P1[Host: Poll for Requests<br/>from Container]
        P2[Guest: Poll for Requests<br/>from Host]
    end

    PollingLoop --> Ready([Portal System<br/>Ready])

    style FIFOSetup fill:#E3F2FD
    style DaemonSpawn fill:#FFF3E0
    style PollingLoop fill:#E8F5E9
    style Start fill:#E3F2FD
    style Ready fill:#90EE90
```

**Portal capabilities:**

- Container can execute commands on host
- Host can execute commands in container (via `fim-instance`)
- Full stdin/stdout/stderr redirection
- Signal forwarding (Ctrl+C, etc.)

## Phase 6: Command Parsing

**Purpose**: Determine what action to take based on command-line arguments.

```mermaid
flowchart TD
    Parse[Command Parsing] --> CheckArgs{Arguments<br/>Provided?}

    CheckArgs -->|No args| DefaultBoot[Default Boot<br/>Read boot command<br/>from reserved space]
    CheckArgs -->|fim-* command| ParseFim[Parse fim-*<br/>Command]

    DefaultBoot --> BootCmd[Execute Boot<br/>Command]

    ParseFim --> ConfigCmd{Configuration<br/>Command?}

    ConfigCmd -->|Yes| Examples1["fim-perms<br/>fim-env<br/>fim-boot<br/>fim-overlay<br/>fim-bind"]
    ConfigCmd -->|No| ExecCmd{Execution<br/>Command?}

    Examples1 --> ModifyConfig[Modify Reserved<br/>Space]

    ExecCmd -->|Yes| Examples2["fim-exec<br/>fim-root<br/>default boot"]
    ExecCmd -->|No| OtherCmd["fim-layer commit<br/>fim-version<br/>fim-help"]

    Examples2 --> SetupContainer[Setup Container]
    OtherCmd --> SpecialAction[Special Action]

    style Parse fill:#E3F2FD
    style ModifyConfig fill:#87CEEB
    style SetupContainer fill:#FFD700
```

**Command categories:**

1. **Configuration commands**: Modify binary without starting container
   - `fim-perms add network,gpu`
   - `fim-env set VAR=value`
   - `fim-boot set firefox`
2. **Execution commands**: Start container and run command
   - `fim-exec firefox` (run as user)
   - `fim-root apk add vim` (run as root)
   - Default boot (no arguments)
3. **Layer commands**: Manage filesystem layers
   - `fim-layer commit` (compress overlay to new layer)
4. **Utility commands**: Information and help
   - `fim-version`, `fim-help`

## Phase 7: Container Setup (for Execution Commands)

**Purpose**: Configure Bubblewrap to create the sandboxed environment.

```mermaid
flowchart LR
    Start([Container Setup]) --> PermConfig

    subgraph PermConfig["Permission Configuration"]
        direction TB
        P1[Read Permissions<br/>from Reserved Space]
        P2[Check Enabled<br/>Permissions]
        P3["Apply Binds:<br/>HOME → --bind $HOME<br/>GPU → --dev-bind /dev/dri<br/>NETWORK → network files<br/>XORG → X11 socket<br/>WAYLAND → wayland socket<br/>AUDIO → audio devices<br/>...and more"]

        P1 --> P2
        P2 --> P3
    end

    PermConfig --> CustomBinds

    subgraph CustomBinds["Custom Bind Mounts"]
        direction TB
        C1[Read Custom<br/>Binds from Database]
        C2["Apply by Type:<br/>RO → --ro-bind<br/>RW → --bind<br/>DEV → --dev-bind"]

        C1 --> C2
    end

    CustomBinds --> EnvConfig

    subgraph EnvConfig["Environment Configuration"]
        direction TB
        E1[Read Environment<br/>Variables]
        E2[Apply Each with<br/>--setenv]

        E1 --> E2
    end

    EnvConfig --> NSIdentity

    subgraph NSIdentity["Namespace & Identity"]
        direction TB
        N1["Configure Namespaces:<br/>Mount namespace<br/>PID namespace<br/>Network namespace<br/>IPC namespace"]
        N2[Set User Identity<br/>uid/gid]

        N1 --> N2
    end

    NSIdentity --> Ready([Configuration Ready])

    style PermConfig fill:#FFE4B5
    style CustomBinds fill:#E8F5E9
    style EnvConfig fill:#E3F2FD
    style NSIdentity fill:#FFF3E0
    style Start fill:#E3F2FD
    style Ready fill:#90EE90
```

**Isolation mechanisms:**

- **Mount namespace**: Container sees isolated filesystem
- **Network namespace**: No network unless `network` permission granted
- **PID namespace**: Process isolation (optional)
- **IPC namespace**: No inter-process communication with host
- **User namespace**: Unprivileged containers (no root required)

**Permission system:**

- **Default**: Full isolation (deny-all)
- **Opt-in**: Each permission explicitly grants access
- **Granular**: 15+ different permissions available
- **Composable**: Permissions can be combined

## Phase 8: Container Execution

**Purpose**: Launch Bubblewrap and execute the command inside the sandbox.

```mermaid
flowchart LR
    Start([Container Execution]) --> BwrapSetup

    subgraph BwrapSetup["Bubblewrap Setup"]
        direction TB
        B1[Launch Bubblewrap<br/>Process]
        B2[Mount Root<br/>Filesystem]
        B3[Apply All<br/>Bind Mounts]
        B4[Set All<br/>Environment Variables]

        B1 --> B2
        B2 --> B3
        B3 --> B4
    end

    BwrapSetup --> ContainerRuntime

    subgraph ContainerRuntime["Container Runtime"]
        direction TB
        R1[Spawn Guest Portal<br/>Daemon in Background]
        R2[Execute Command<br/>in Sandbox]

        R1 --> R2
    end

    ContainerRuntime --> Running[Command Running]

    Running --> Monitoring

    subgraph Monitoring["Runtime Monitoring"]
        direction TB
        M1[Portal Monitors<br/>for IPC Requests]
        M2{Command<br/>Exits?}

        M1 --> M2
        M2 -->|No| M1
    end

    M2 -->|Yes| Cleanup([Cleanup Phase])

    style BwrapSetup fill:#E3F2FD
    style ContainerRuntime fill:#FFF3E0
    style Running fill:#90EE90
    style Monitoring fill:#E8F5E9
    style M2 fill:#FFD700
```

**What happens inside:**

1. Guest portal daemon starts (optional, for host→container commands)
2. Bubblewrap creates isolated namespaces
3. Root filesystem mounted (merged layers + overlay)
4. All bind mounts applied
5. Environment variables set
6. Command executed (bash, firefox, etc.)
7. Portal system active for IPC
8. Command runs until completion or signal

**During execution:**

- Container has access only to granted permissions
- Portal enables host communication if needed
- Standard I/O works normally
- Signals forwarded (Ctrl+C terminates cleanly)

## Phase 9: Cleanup

**Purpose**: Properly shut down the container and unmount filesystems.

```mermaid
flowchart LR
    Start([Cleanup Phase]) --> ContainerCleanup

    subgraph ContainerCleanup["Container Cleanup"]
        direction TB
        C1[Signal handler<br/>catches SIGTERM/SIGINT]
        C2[Shutdown portal daemon]
        C3[Wait for container<br/>process to exit]
        C4[Bubblewrap exits<br/>with child exit code]

        C1 --> C2
        C2 --> C3
        C3 --> C4
    end

    ContainerCleanup --> FilesystemCleanup

    subgraph FilesystemCleanup["Filesystem Cleanup"]
        direction TB
        F1[Destroy Filesystem<br/>Controller]
        F2[Kill janitor daemon]
        F3[Un-mount overlay<br/>and DwarFS layers]

        F1 --> F2
        F2 --> F3
    end

    FilesystemCleanup --> Clean[Clean temporary<br/>directories]
    Clean --> Return([Return exit code<br/>to parent])

    style ContainerCleanup fill:#FFF3E0
    style FilesystemCleanup fill:#E8F5E9
    style Start fill:#90EE90
    style Return fill:#FFB6C6

```

**Cleanup order:**

1. Command finishes execution
2. Guest portal daemon stopped
3. Bubblewrap exits
4. Filesystem controller destroyed
5. Janitor daemon signaled to terminate
6. Filesystems unmounted (CIOPFS → overlay → DwarFS layers)
7. Temporary directories cleaned
8. Exit code returned to shell

**Graceful vs. crash handling:**

- **Graceful exit**: All cleanup happens in order
- **Crash/kill**: Janitor daemon detects parent death and unmounts filesystems
- **Result**: No stale FUSE mounts in either case

## Boot Sequence Timeline

```mermaid
gantt
    title FlatImage Boot Sequence Timeline (Typical)
    dateFormat X
    axisFormat %L ms

    section Phase 1
    Binary Relocation (if needed) :0, 50

    section Phase 2
    System Initialization :50, 100

    section Phase 3
    Configuration Loading :100, 200

    section Phase 4
    Filesystem Setup :200, 700

    section Phase 5
    Portal Startup :700, 750

    section Phase 6
    Command Parsing :750, 755

    section Phase 7
    Container Setup :755, 1055

    section Phase 8
    Container Execution :milestone, 1055, 0
```

**Performance factors:**

- Number of DwarFS layers (more = slower)
- Overlay type (BWRAP fastest, UnionFS slowest)
- Casefold enabled (adds CIOPFS overhead)
- External layers (network/disk latency)