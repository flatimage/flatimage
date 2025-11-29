# External Layer Files

Create reusable, shareable layer files that can be loaded on demand without embedding them in the binary.

## What are External Layers?

External layers are compressed filesystem layers stored as separate files. They allow you to:

- Build modular application stacks
- Share layers between multiple FlatImages
- Load different combinations of software on demand
- Keep binaries small while maintaining flexibility
- Version control individual layers

## FIM_FILES_LAYER: Base + Applications

This example creates a base layer with common packages, then separate layers for Firefox and GIMP.

### Step 1: Download and Setup

```bash
# Download Alpine Linux FlatImage
wget -O app.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
chmod +x app.flatimage

# Configure permissions
./app.flatimage fim-perms add network,xorg,wayland,audio,gpu

# Set a recipe remote (Optional, if you want to use fim-root apk add... then skip)
./app.flatimage fim-remote set https://raw.githubusercontent.com/flatimage/recipes/master

# Create layers directory
mkdir -p layers
```

### Step 2: Create Base Layer

Install common dependencies that both applications need:

```bash
# Install base packages
./app.flatimage fim-recipe install wayland,xorg,audio,fonts

# Commit to external file
./app.flatimage fim-layer commit file ./layers/base.layer
```

The base layer is now saved as `./layers/base.layer`.

### Step 3: Create Firefox Layer

Load the base layer first, then install Firefox:

```bash
# Set the base to always be mounted as a layer
# This avoids re-installing all packages
export FIM_FILES_LAYER="$PWD/layers/base.layer"

# Install Firefox (base layer provides common dependencies)
./app.flatimage fim-root apk add --no-cache firefox

# Commit to external file
./app.flatimage fim-layer commit file ./layers/firefox.layer
```

This way, Firefox doesn't need to reinstall the common dependencies already in the base layer.

### Step 4: Create GIMP Layer

Load the base layer first, then install GIMP:

```bash
export FIM_FILES_LAYER="$PWD/layers/base.layer"

# Install GIMP (base layer provides common dependencies)
./app.flatimage fim-root apk add --no-cache gimp

# Commit to external file
./app.flatimage fim-layer commit file ./layers/gimp.layer
```

GIMP also benefits from the common dependencies in the base layer.

### Step 5: Run with Base + Firefox

Load base and Firefox layers together:

```bash
# Create the data directory
mkdir -p data/firefox
# Run with base + firefox layers
FIM_DIR_DATA=$PWD/data/firefox FIM_FILES_LAYER="$PWD/layers/base.layer:$PWD/layers/firefox.layer" \
  ./app.flatimage fim-exec firefox
# Or alternatively
export FIM_FILES_LAYER="$PWD/layers/base.layer"
FIM_DIR_DATA=$PWD/data/firefox FIM_FILES_LAYER="$FIM_FILES_LAYER:$PWD/layers/firefox.layer" \
  ./app.flatimage fim-exec firefox
```

Firefox launches with all base dependencies available!

### Step 6: Run with Base + GIMP

Switch to GIMP by loading different layers:

```bash
# Create the data directory
mkdir -p data/gimp
# Run with base + gimp layers
FIM_DIR_DATA=$PWD/data/gimp FIM_FILES_LAYER="$PWD/layers/base.layer:$PWD/layers/gimp.layer" \
  ./app.flatimage fim-exec gimp
# Or alternatively
export FIM_FILES_LAYER="$PWD/layers/base.layer"
FIM_DIR_DATA=$PWD/data/gimp FIM_FILES_LAYER="$FIM_FILES_LAYER:$PWD/layers/gimp.layer" \
  ./app.flatimage fim-exec gimp
```

GIMP launches with the same base dependencies!

## Understanding FIM_FILES_LAYER

The `FIM_FILES_LAYER` environment variable specifies external layers to load:

```bash
# Single layer
FIM_FILES_LAYER="./layer.layer" ./app.flatimage

# Multiple layers (colon-separated)
FIM_FILES_LAYER="./base.layer:./app.layer" ./app.flatimage

# Absolute paths work too
FIM_FILES_LAYER="/opt/layers/base.layer:/opt/layers/firefox.layer" ./app.flatimage
```

**Layer Order**: Layers are applied left-to-right, with later layers taking precedence over earlier ones.

## Creating Launcher Scripts

Make layer combinations easy to launch:

### Firefox Launcher

Create `firefox.sh`:

```bash
#!/bin/sh
DIR_SCRIPT="$(cd "$(dirname "$0")" && pwd)"
export FIM_DIR_DATA="$DIR_SCRIPT/data/firefox"
export FIM_FILES_LAYER="$DIR_SCRIPT/layers/base.layer:$DIR_SCRIPT/layers/firefox.layer"
"$DIR_SCRIPT/app.flatimage" fim-exec firefox "$@"
```

### GIMP Launcher

Create `gimp.sh`:

```bash
#!/bin/sh
DIR_SCRIPT="$(cd "$(dirname "$0")" && pwd)"
export FIM_DIR_DATA="$DIR_SCRIPT/data/gimp"
export FIM_FILES_LAYER="$DIR_SCRIPT/layers/base.layer:$DIR_SCRIPT/layers/gimp.layer"
"$DIR_SCRIPT/app.flatimage" fim-exec gimp "$@"
```

### Make Executable and Run

```bash
# Create data directories
mkdir -p data/firefox data/gimp

# Make scripts executable
chmod +x firefox.sh gimp.sh

# Launch Firefox (can be run from any directory)
./firefox.sh &

# Launch GIMP (can be run from any directory)
./gimp.sh &
```

## Learn More

- [fim-layer](../cmd/layer.md) - Complete layer management documentation
- [Commit Changes](getting-started/commit-changes.md) - Different commit modes explained
- [Architecture: Layers](../architecture/layers.md) - Technical details of layered filesystem
- [Architecture: Filesystem](../architecture/filesystem.md) - Filesystem architecture overview
- [Environment Variables](environment-variables.md) - Using FIM_FILES_LAYER and other variables
