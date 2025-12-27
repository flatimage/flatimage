# Download and Run

Get started with FlatImage in 30 seconds.

## Quick Start

```bash
# Download a FlatImage distribution
wget -O alpine.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine-x86_64.flatimage

# Make it executable
chmod +x alpine.flatimage

# Run it
./alpine.flatimage
```

You're now inside an isolated Alpine Linux environment!

## Available Distributions

**Alpine Linux** - MUSL-based with apk package manager:
```bash
wget -O alpine.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine-x86_64.flatimage
```

**Arch Linux** - GNU-based with pacman package manager:
```bash
wget -O arch.flatimage https://github.com/flatimage/flatimage/releases/latest/download/arch-x86_64.flatimage
```

**Blueprint** - Empty template for custom builds:
```bash
wget -O blueprint.flatimage https://github.com/flatimage/flatimage/releases/latest/download/blueprint-x86_64.flatimage
```

## Exit the Container

Press `CTRL+D` or type `exit` to leave the container.