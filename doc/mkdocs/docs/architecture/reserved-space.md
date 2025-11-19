# Reserved Space

## Overview

FlatImage's reserved space is an embedded configuration storage area within the binary itself. This enables **post-build reconfiguration** without recompilation - you can modify permissions, environment variables, boot commands, and other settings after the binary has been built.

This architecture makes commands like `fim-perms add`, `fim-env set`, and `fim-boot set` possible by directly modifying the reserved section of the binary file.

## Conceptual Architecture

The reserved space acts as an embedded key-value store within the ELF binary, positioned between the executable code and the filesystem data:

```mermaid
flowchart LR
    Binary["FlatImage Binary"] --> Structure

    subgraph Structure["Binary Structure"]
        direction TB
        S1["ELF Header & Code<br/>Executable program"]
        S2["Reserved Space Marker<br/>Offset symbol"]
        S3["Configuration Storage<br/>Embedded settings"]
        S4["Filesystem Layers<br/>Compressed DwarFS data"]

        S1 --> S2
        S2 --> S3
        S3 --> S4
    end

    Structure --> Access

    subgraph Access["Configuration Access"]
        direction TB
        A1["Read operations<br/>Load settings at boot"]
        A2["Write operations<br/>Modify via fim-* commands"]

        A1 -.-> A2
    end

    style Structure fill:#E3F2FD
    style Access fill:#E8F5E9
    style Binary fill:#90EE90
```

**Key Concepts:**

- **Embedded Storage**: Configuration lives inside the binary file, not in external config files
- **Fixed Offsets**: Each configuration type has a dedicated region with known boundaries
- **Post-Build Modification**: Settings can be changed after compilation
- **Portable**: Configuration travels with the binary - no separate files needed

## Configuration Components

Reserved space stores multiple types of configuration data, each serving a specific purpose:

```mermaid
flowchart LR
    Start([Reserved Space]) --> Categories

    subgraph Categories["Configuration Types"]
        direction TB
        C1["🔒 Security Settings<br/>Permissions, isolation"]
        C2["🎨 Desktop Integration<br/>Icons, menus, MIME types"]
        C3["⚙️ Runtime Config<br/>Environment, boot commands"]
        C4["📁 Filesystem Config<br/>Bind mounts, overlay type"]

        C1 -.-> C2
        C2 -.-> C3
        C3 -.-> C4
    end

    Categories --> Management

    subgraph Management["Configuration Management"]
        direction TB
        M1["Read at boot<br/>Apply settings"]
        M2["Modify via fim-* tools<br/>Update on demand"]
        M3["Persist in binary<br/>No external files"]

        M1 --> M2
        M2 --> M3
    end

    style Categories fill:#E3F2FD
    style Management fill:#E8F5E9
    style Start fill:#90EE90
```

### Security Settings

**Permissions**

- Controls access to host resources (home directory, network, GPU, audio, etc.)
- Stored as a bitfield where each bit represents a permission flag
- Supports up to 64 different permission types
- **Commands:** `fim-perms add`, `fim-perms del`, `fim-perms list`

**Overlay Type**

- Selects which overlay filesystem to use (UNIONFS, OVERLAYFS, BWRAP)
- Affects performance and compatibility
- BWRAP provides native bubblewrap isolation
- **Commands:** `fim-overlay set`, `fim-overlay show`

**Casefold Flag**

- Enables case-insensitive filesystem behavior
- Critical for Windows application compatibility (Wine/Proton)
- Implemented via CIOPFS layer
- Cannot be combined with BWRAP overlay
- **Commands:** `fim-casefold on`, `fim-casefold off`

### Desktop Integration

**Desktop Configuration**

- Application name and metadata
- Integration types (desktop entries, MIME handlers, icons)
- Application categories for menu organization
- Stored as structured data
- **Commands:** `fim-desktop setup`, `fim-desktop enable`, `fim-desktop dump`

**Icon Data**

- Embedded application icon in PNG format
- Automatically resized to standard sizes for desktop integration
- Eliminates need for separate icon files
- **Commands:** `fim-desktop dump icon`

**Notify Flag**

- Controls desktop notifications on FlatImage startup
- Simple on/off toggle
- **Commands:** `fim-notify on`, `fim-notify off`

### Runtime Configuration

**Boot Command**

- Default command executed when FlatImage starts
- Defines the primary application to run
- Defaults to interactive shell if not set
- Includes command path and arguments
- **Commands:** `fim-boot set`, `fim-boot show`, `fim-boot clear`

**Environment Variables**

- Custom environment variables for the container
- Applied during container initialization
- Supports variable expansion
- Each entry is a key-value pair
- **Commands:** `fim-env add`, `fim-env set`, `fim-env del`, `fim-env list`

### Filesystem Configuration

**Bind Mounts**

- Host-to-guest filesystem mappings
- Defines what host directories are accessible inside the container
- Each bind has a source, destination, and access mode (read-only, read-write, device)
- **Commands:** `fim-bind add`, `fim-bind del`, `fim-bind list`

**Remote URL**

- URL for fetching remote resources (recipes, additional layers)
- Enables dynamic content loading
- Plain text storage
- **Commands:** `fim-remote set`, `fim-remote show`, `fim-remote clear`

## How Reserved Space Works

### Configuration Lifecycle

```mermaid
flowchart LR
    Start([User Action]) --> Modify

    subgraph Modify["Modification Phase"]
        direction TB
        M1["User runs fim-* command<br/>e.g., fim-perms add network"]
        M2["Parse command arguments<br/>Extract settings to change"]
        M3["Read current configuration<br/>Load from reserved space"]
        M4["Update configuration<br/>Merge with new values"]

        M1 --> M2
        M2 --> M3
        M3 --> M4
    end

    Modify --> Write

    subgraph Write["Write Phase"]
        direction TB
        W1["Validate new data<br/>Check size limits"]
        W2["Open binary file<br/>Acquire write access"]
        W3["Clear target region<br/>Zero-initialize"]
        W4["Write new data<br/>Update configuration"]

        W1 --> W2
        W2 --> W3
        W3 --> W4
    end

    Write --> Next

    subgraph Next["Next Execution"]
        direction TB
        N1["FlatImage starts"]
        N2["Read reserved space<br/>Load configuration"]
        N3["Apply settings<br/>Configure container"]

        N1 --> N2
        N2 --> N3
    end

    style Modify fill:#E3F2FD
    style Write fill:#FFF3E0
    style Next fill:#E8F5E9
    style Start fill:#90EE90
```

### Read and Write Operations

**Write Process:**

1. Validate that new data fits within allocated space
2. Open the binary file with write permissions
3. Navigate to the configuration's region
4. Clear existing data (zero-initialization)
5. Write new configuration data
6. Close the file

**Read Process:**

1. Open the binary file in read-only mode
2. Navigate to the configuration's region
3. Read the configuration data
4. Parse and validate the data
5. Return the configuration

**Safety Guarantees:**

- **Bounds Checking**: All operations validate that data fits within allocated space
- **Atomic Regions**: Each configuration type has an isolated region
- **Zero-Initialization**: Regions are cleared before writing to prevent data leakage
- **Error Handling**: Operations return success/failure status for robust error handling

### Practical Example

When you modify permissions, the following conceptual flow occurs:

```mermaid
flowchart LR
    Start([fim-perms add network]) --> Parse

    subgraph Parse["Command Processing"]
        direction TB
        P1["Parse 'network' argument"]
        P2["Map to permission bit"]
        P3["Identify target region"]

        P1 --> P2
        P2 --> P3
    end

    Parse --> Read

    subgraph Read["Read Current State"]
        direction TB
        R1["Open binary file"]
        R2["Read permissions region"]
        R3["Parse current bitfield"]

        R1 --> R2
        R2 --> R3
    end

    Read --> Update

    subgraph Update["Update Configuration"]
        direction TB
        U1["Set network bit"]
        U2["Preserve other bits"]
        U3["Create new bitfield"]

        U1 --> U2
        U2 --> U3
    end

    Update --> WriteBack

    subgraph WriteBack["Persist Changes"]
        direction TB
        W1["Clear permissions region"]
        W2["Write updated bitfield"]
        W3["Close binary file"]

        W1 --> W2
        W2 --> W3
    end

    WriteBack --> Effect

    subgraph Effect["Next Execution"]
        direction TB
        E1["FlatImage boots"]
        E2["Reads permissions"]
        E3["Network access enabled"]

        E1 --> E2
        E2 --> E3
    end

    style Parse fill:#E3F2FD
    style Read fill:#FFF3E0
    style Update fill:#E8F5E9
    style WriteBack fill:#FFE4B5
    style Effect fill:#F3E5F5
    style Start fill:#90EE90
```

## Advantages of Reserved Space

**Post-Build Reconfiguration**

- Modify settings without rebuilding the binary
- Changes take effect on next execution
- No recompilation needed

**Portability**

- Configuration embedded in the binary
- No external config files required
- Single file contains complete application

**Atomic Updates**

- Each configuration type isolated in its own region
- Changes to one setting don't affect others
- Safe concurrent reads

## Limitations

**Fixed Size Constraints**

- Each configuration type has a maximum size
- Operations fail if data exceeds limits
- Trade-off between flexibility and binary size

**Binary Modification Required**

- Every change modifies the binary file
- Requires write permissions
- Not ideal for read-only filesystems (requires copy)