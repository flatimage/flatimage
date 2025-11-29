# External Layer Directories

Load multiple layer files automatically by pointing to directories instead of individual files.

## What are Layer Directories?

Layer directories provide a convenient way to load multiple external layers by specifying directories rather than individual files. FlatImage automatically scans these directories and mounts all valid layer files found within them.

**Benefits:**

- **Batch loading**: Mount multiple layers with a single directory path
- **Simplified management**: Organize layers in directories by category or purpose
- **Automatic discovery**: New layers added to directories are automatically loaded

## FIM_DIRS_LAYER: Automatic Layer Discovery

The `FIM_DIRS_LAYER` environment variable specifies directories containing layer files. FlatImage scans each directory and mounts all regular files it finds.

### Basic Usage

```bash
# Single directory
FIM_DIRS_LAYER="./layers" ./app.flatimage

# Multiple directories (colon-separated)
FIM_DIRS_LAYER="./base-layers:./app-layers" ./app.flatimage

# Absolute paths
FIM_DIRS_LAYER="/opt/shared-layers:/home/user/custom-layers" ./app.flatimage
```

### How Directory Scanning Works

**Important characteristics:**

1. **Non-recursive**: Only scans direct children of each directory (subdirectories are ignored)
2. **Regular files only**: Excludes directories, symlinks, device files, and special files
3. **Alphabetical order**: Files within each directory are loaded in alphabetical order
4. **No extension filtering**: All regular files are validated as layer files (DWARFS type)
5. **Permissive errors**: Invalid files are skipped with a warning; processing continues

**Example directory structure:**

```
~/layers/
├── 01-audio.layer           ← Mounted (alphabetical: first)
├── 02-networking.layer      ← Mounted (alphabetical: second)
├── 03-gpu-nvidia.layer      ← Mounted (alphabetical: third)
├── readme.txt               ← Scanned, skipped (not a valid layer)
├── backup.layer.bak         ← Scanned, skipped (not a valid layer)
├── subdir/                  ← Ignored (not a regular file)
│   └── nested.layer         ← NOT mounted (not in top level)
└── symlink.layer -> ...     ← Ignored (not a regular file)
```

**Result**: Only `01-audio.layer`, `02-networking.layer`, and `03-gpu-nvidia.layer` are mounted, in that order.

## Example: Organized Layer Directory Structure

### Step 1: Download and Setup

```bash
# Download Alpine Linux FlatImage
wget -O app.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
chmod +x app.flatimage

# Configure permissions
./app.flatimage fim-perms add network,xorg,wayland,audio,gpu

# Set a recipe remote
./app.flatimage fim-remote set https://raw.githubusercontent.com/flatimage/recipes/master

# Create organized directory structure
mkdir -p layers/base
mkdir -p layers/desktop
```

### Step 2: Create Base System Layers

Build foundational layers with numbered prefixes for ordering:

```bash
# Create audio layer
./app.flatimage fim-recipe install wayland,xorg
./app.flatimage fim-layer commit file ./layers/base/00-display.layer

# Create audio layer
./app.flatimage fim-recipe install audio
./app.flatimage fim-layer commit file ./layers/base/01-audio.layer

# Create GPU layer
./app.flatimage fim-recipe install gpu
./app.flatimage fim-layer commit file ./layers/base/02-gpu.layer

# Create fonts layer
./app.flatimage fim-recipe install fonts
./app.flatimage fim-layer commit file ./layers/base/03-fonts.layer
```

**Directory structure now:**
```
layers/base/
├── 01-audio.layer
├── 02-gpu.layer
└── 03-fonts.layer
```

### Step 3: Create Desktop Application Layers

Build application layers on top of base:

```bash
# Load all base layers automatically
export FIM_DIRS_LAYER="$PWD/layers/base"

# Create Firefox layer
./app.flatimage fim-root apk add --no-cache firefox
./app.flatimage fim-layer commit file ./layers/desktop/firefox.layer

# Create GIMP layer
./app.flatimage fim-root apk add --no-cache gimp
./app.flatimage fim-layer commit file ./layers/desktop/gimp.layer

# Create LibreOffice layer
./app.flatimage fim-root apk add --no-cache libreoffice
./app.flatimage fim-layer commit file ./layers/desktop/libreoffice.layer
```

**Directory structure now:**
```
layers/
├── base/
│   ├── 00-display.layer
│   ├── 01-audio.layer
│   ├── 02-gpu.layer
│   └── 03-fonts.layer
└── desktop/
    ├── firefox.layer
    ├── gimp.layer
    └── libreoffice.layer
```

### Step 4: Run with Different Layer Combinations

Load different combinations by specifying directories:

```bash
# Run Firefox with base + desktop layers
mkdir -p data/firefox
FIM_DIR_DATA=$PWD/data/firefox \
FIM_DIRS_LAYER="$PWD/layers/base:$PWD/layers/desktop" \
  ./app.flatimage fim-exec firefox

# Run GIMP with base + desktop layers
mkdir -p data/gimp
FIM_DIR_DATA=$PWD/data/gimp \
FIM_DIRS_LAYER="$PWD/layers/base:$PWD/layers/desktop" \
  ./app.flatimage fim-exec gimp

# Run LibreOffice with base + desktop layers
mkdir -p data/libreoffice
FIM_DIR_DATA=$PWD/data/libreoffice \
FIM_DIRS_LAYER="$PWD/layers/base:$PWD/layers/desktop" \
  ./app.flatimage fim-exec libreoffice
```

## Controlling Load Order with Naming

Since files within a directory are loaded alphabetically, use prefixes to control ordering:

### Numeric Prefixes

```
layers/
├── 00-base-system.layer      ← First
├── 01-audio-support.layer    ← Second
├── 02-gpu-drivers.layer      ← Third
├── 10-applications.layer     ← Fourth
└── 99-overrides.layer        ← Last
```

### Descriptive Prefixes

```
layers/
├── base-alpine.layer         ← Alphabetically first
├── drivers-audio.layer       ← Second
├── drivers-gpu.layer         ← Third
└── utils-system.layer        ← Last
```

### Date-Based Versioning

```
layers/base/
├── 2024-01-15-audio.layer
├── 2024-02-10-gpu.layer
└── 2024-03-05-fonts.layer
```

## Combining FIM_DIRS_LAYER with FIM_FILES_LAYER

You can use both environment variables together. FlatImage processes directories first, then appends files:

```bash
# Load base layers from directory, then specific override file
FIM_DIRS_LAYER="$PWD/layers/base" \
FIM_FILES_LAYER="$PWD/overrides/custom-config.layer" \
  ./app.flatimage fim-exec bash
```

**Layer loading order:**

1. Base layer (embedded in binary)
2. Committed layers (embedded in binary)
3. **FIM_DIRS_LAYER files** (alphabetical within each directory)
4. **FIM_FILES_LAYER files** (exact order specified)
5. Overlay layer (writable)

This allows precise control: use directories for common layers, then files for specific overrides.

## Creating Launcher Scripts

Make layer combinations easy to launch with shell scripts:

### Firefox Launcher

Create `firefox.sh`:

```bash
#!/bin/sh
DIR_SCRIPT="$(cd "$(dirname "$0")" && pwd)"
export FIM_DIR_DATA="$DIR_SCRIPT/data/firefox"
export FIM_DIRS_LAYER="$DIR_SCRIPT/layers/base:$DIR_SCRIPT/layers/desktop"
"$DIR_SCRIPT/app.flatimage" fim-exec firefox "$@"
```

### GIMP Launcher

Create `gimp.sh`:

```bash
#!/bin/sh
DIR_SCRIPT="$(cd "$(dirname "$0")" && pwd)"
export FIM_DIR_DATA="$DIR_SCRIPT/data/gimp"
export FIM_DIRS_LAYER="$DIR_SCRIPT/layers/base:$DIR_SCRIPT/layers/desktop"
"$DIR_SCRIPT/app.flatimage" fim-exec gimp "$@"
```

### LibreOffice Launcher

Create `libreoffice.sh`:

```bash
#!/bin/sh
DIR_SCRIPT="$(cd "$(dirname "$0")" && pwd)"
export FIM_DIR_DATA="$DIR_SCRIPT/data/libreoffice"
export FIM_DIRS_LAYER="$DIR_SCRIPT/layers/base:$DIR_SCRIPT/layers/desktop"
"$DIR_SCRIPT/app.flatimage" fim-exec libreoffice "$@"
```

### Make Executable and Run

```bash
# Make scripts executable
chmod +x firefox.sh gimp.sh libreoffice.sh

# Launch applications (can be run from any directory)
./firefox.sh &
./gimp.sh &
./libreoffice.sh &
```

## Learn More

- [External Layer Files](layer-files.md) - Using FIM_FILES_LAYER for precise control
- [fim-layer](../../cmd/layer.md) - Complete layer management documentation
- [Commit Changes](../getting-started/commit-changes.md) - Different commit modes explained
- [Architecture: Layers](../../architecture/layers.md) - Technical details of layered filesystem
- [Architecture: Filesystem](../../architecture/filesystem.md) - Filesystem architecture overview
- [Architecture: Environment](../../architecture/environment.md) - Environment variables reference
