# Multiple Instances with Separate Data

## Overview

This example demonstrates how to run multiple isolated instances of the same FlatImage application, each with its own configuration and data directory. This is useful for:

- Running multiple profiles of the same application (e.g., work and personal browsers)
- Testing different configurations without conflicts
- Running development and production instances side-by-side
- Creating portable application instances with different settings

## Why This Feature is Useful

Each FlatImage binary stores its persistent data in `{BINARY_DIR}/.{BINARY_NAME}.data/`, override it using the `FIM_DIR_DATA` environment variable. This is useful when you want to:

- Store data in a specific location (e.g., external drive)
- Use the same binary with different configurations
- Manage configurations centrally

## Example of Distinct Data Directories

```bash
# Download Firefox image
wget -O firefox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
chmod +x ./firefox.flatimage

# Configure Firefox
./firefox.flatimage fim-perms set wayland,xorg,audio,gpu,network
./firefox.flatimage fim-recipe install firefox
./firefox.flatimage fim-boot set firefox
./firefox.flatimage fim-layer commit binary

# Run work instance
FIM_DIR_DATA=./firefox-configs/work ./firefox.flatimage &

# Run personal instance
FIM_DIR_DATA=./firefox-configs/personal ./firefox.flatimage &

# Run testing instance
FIM_DIR_DATA=./firefox-configs/testing ./firefox.flatimage &
```

## Creating Instance Launcher Scripts

Create wrapper scripts for easy instance management:

### Work Instance Launcher

Create `firefox-work.sh`:

```bash
#!/bin/bash
FIM_DIR_DATA="$HOME/Firefox/firefox-work" ./firefox.flatimage "$@"
```

### Personal Instance Launcher

Create `firefox-personal.sh`:

```bash
#!/bin/bash
FIM_DIR_DATA="$HOME/Firefox/firefox-personal" ./firefox.flatimage "$@"
```

### Make Scripts Executable

```bash
chmod +x firefox-work.sh firefox-personal.sh
# Run instances
./firefox-work.sh &
./firefox-personal.sh &
```

## Summary

Multiple instance support in FlatImage enables:

- **Isolation**: Each instance has its own data directory
- **Easy Management**: Simple file-based organization
- **True Portability**: Move entire configurations by moving a directory
- **Resource Efficiency**: Share the same base image across instances