# Filesystem Layers

## Overview

FlatImage uses a **layered filesystem architecture** to provide:

- **Compressed read-only base layers** (DwarFS)
- **Writable overlay layer** for temporary changes
- **Optional case-insensitive layer** for Windows compatibility
- **Incremental layer commits** for persistent changes

This architecture enables efficient storage, fast startup, and flexible layer management.

## Filesystem Stack

The complete filesystem stack (from top to bottom):

```
┌─────────────────────────────────────┐
│  Container Root (/)                 │ ← What the container sees
├─────────────────────────────────────┤
│  CIOPFS (optional)                  │ ← Case-insensitive layer
│  Case-folding overlay               │ ← Only if casefold enabled
├─────────────────────────────────────┤
│  Overlay Layer                      │ ← Writable layer
│  (UnionFS / OverlayFS / BWRAP)      │ ← Selected by fim-overlay
├─────────────────────────────────────┤
│  DwarFS Layer N (external)          │ ← FIM_FILES_LAYER
├─────────────────────────────────────┤
│  DwarFS Layer 2 (committed)         │ ← fim-layer commit
├─────────────────────────────────────┤
│  DwarFS Layer 1 (committed)         │ ← fim-layer commit
├─────────────────────────────────────┤
│  DwarFS Layer 0 (base)              │ ← Built-in base filesystem
└─────────────────────────────────────┘
```

**Data flow:** When the container reads a file, it searches from top to bottom. The first layer containing the file wins. This enables **copy-on-write** behavior.

## Layer Types

### 1. DwarFS Compressed Layers

**DwarFS** is a high-compression read-only filesystem that provides:

- **Extreme compression ratios** (better than squashfs)
- **Fast on-the-fly decompression** via FUSE
- **Metadata caching** for improved performance

#### Layer Mounting

DwarFS layers are mounted with offset parameters:

```bash
dwarfs -o offset={OFFSET} -o imagesize={SIZE} {BINARY} {MOUNTPOINT}
```

- **offset:** Byte position in binary where filesystem begins
- **imagesize:** Size of the filesystem in bytes
- **BINARY:** The FlatImage executable itself
- **MOUNTPOINT:** Where to mount (e.g., `/tmp/fim/app/.../layers/0/`)

#### Magic Number Detection

DwarFS filesystems are identified by the magic bytes `DWARFS` (6 bytes):

```cpp
bool is_dwarfs = (bytes[0..5] == "DWARFS");
```

The mounting code scans the binary for these magic bytes to locate each filesystem.

#### Layer Locations

1. **Embedded layers:** Stored in binary after reserved space
    - Offset: `FIM_RESERVED_OFFSET + FIM_RESERVED_SIZE`
    - Created during build or with `fim-layer commit`

2. **External directory layers:** `FIM_DIRS_LAYER` environment variable
    - Scans directories for `.dwarfs` files
    - Mounted in alphabetical order

3. **External file layers:** `FIM_FILES_LAYER` environment variable
    - Mounts specific `.dwarfs` files
    - Precise control over layer order

### 2. Overlay Filesystems

The overlay layer provides **writable storage** on top of read-only DwarFS layers. FlatImage supports three overlay backends:

#### UnionFS-FUSE (Type: UNIONFS)

**Advantages:**

- Most compatible across kernel versions
- Works with case-insensitive layer (CIOPFS)
- Widely tested and stable

**Disadvantages:**

- Slower than OverlayFS
- Requires FUSE (userspace overhead)

**Usage:**
```bash
./app.flatimage fim-overlay set unionfs
```

#### FUSE-OverlayFS (Type: OVERLAYFS)

**Advantages:**

- Faster than UnionFS
- Better performance for large filesystems
- Works with case-insensitive layer

**Disadvantages:**

- Broken for several applications due to [this](https://github.com/containers/fuse-overlayfs/issues/432) change.
- Requires fuse-overlayfs binary
- Slightly less compatible than UnionFS

**Usage:**
```bash
./app.flatimage fim-overlay set overlayfs
```

#### Bubblewrap Native (Type: BWRAP)

**Advantages:**

- Fastest option (native overlay)
- No FUSE overhead
- Minimal dependencies

**Disadvantages:**

- **Cannot be used with casefold** (incompatible)
- Requires kernel overlay support
- Less flexible

**Usage:**
```bash
./app.flatimage fim-overlay set bwrap
```

### 3. Case-Insensitive Layer (CIOPFS)

**CIOPFS** (Case-Insensitive On Purpose FS) provides Windows-compatible filesystem behavior.

#### Why It's Needed

**Linux filesystems** are **case-sensitive:**
```bash
$ touch readme.txt README.txt
$ ls
readme.txt  README.txt  # Both files exist
```

**Windows filesystems** are **case-insensitive:**
```
C:\> echo test > readme.txt
C:\> type README.TXT
test  # Same file
```

**Problem:** Wine/Proton applications expect case-insensitive behavior. Without it, games may fail to find assets like `Textures/Sky.DDS` if the code references `textures/sky.dds`.

#### How It Works

CIOPFS is mounted **on top of** the overlay layer:

```
┌─────────────────────────────────┐
│ CIOPFS Mount                    │ ← /tmp/fim/app/.../instance/{PID}/mount/
│ (case-insensitive view)         │
├─────────────────────────────────┤
│ Overlay Layer                   │ ← .{BINARY}.data/casefold/
│ (case-sensitive storage)        │
└─────────────────────────────────┘
```

**Access:**

- Writing: `file.TXT` is stored as `file.txt` (lowercase)
- Reading: `FILE.txt`, `file.TXT`, `File.Txt` all access the same file

#### Limitations

- **Not case-preserving:** Files are stored in lowercase
- **Cannot be used with BWRAP overlay**
- Adds slight FUSE overhead

**Example:**
```bash
# Enable casefold
$ ./app.flatimage fim-casefold on

# Create file
$ ./app.flatimage fim-root touch /MyFile.TXT

# Access with different casing
$ ./app.flatimage stat /myfile.txt    # Works
$ ./app.flatimage stat /MYFILE.TXT    # Works
$ ./app.flatimage stat /MyFile.TXT    # Works

# Disable casefold
$ ./app.flatimage fim-casefold off

# Now only lowercase works
$ ./app.flatimage stat /myfile.txt    # Works
$ ./app.flatimage stat /MyFile.TXT    # Not found
```

## Layer Management Operations

### Creating External Layers

Create a standalone DwarFS layer:

```bash
./app.flatimage fim-layer create /path/to/directory layer.dwarfs
```

**Compression level** (0-10) can be controlled via `FIM_COMPRESSION_LEVEL`:

```bash
FIM_COMPRESSION_LEVEL=9 ./app.flatimage fim-layer create /path layer.dwarfs
```

- **Lower (0-3):** Faster compression, larger files
- **Medium (4-7):** Balanced (default: 7)
- **Higher (8-10):** Slower compression, smaller files

### Committing Changes

Persist overlay changes to a new embedded layer:

```bash
./app.flatimage fim-layer commit
```

**What happens:**

1. Overlay directory (`.{BINARY}.data/data/`) is compressed to DwarFS
2. DwarFS layer is appended to the binary
3. Overlay directory is cleared
4. Next run mounts the new committed layer

**Effect:** Changes become permanent and compressed.

### Loading External Layers

#### Load from directory:
```bash
FIM_DIRS_LAYER=/path/to/layers ./app.flatimage
```

All `.dwarfs` files in `/path/to/layers/` are mounted.

#### Load specific files:
```bash
FIM_FILES_LAYER=/path/to/layer1.dwarfs:/path/to/layer2.dwarfs ./app.flatimage
```

Layers are mounted in the order specified.

**Use cases:**

- Sharing layers across multiple FlatImages
- Development: test layers without embedding
- Layer distribution: download and mount external layers