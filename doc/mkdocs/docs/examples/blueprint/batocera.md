# Blueprint - Batocera

This advanced example demonstrates how to use the Blueprint FlatImage (an empty container) to create a custom Batocera Linux portable gaming system. Batocera is a retro-gaming distribution, and this example shows how to fetch an external system image, extract it, customize it (removing Nvidia packages), and integrate it into a FlatImage.

This guide uses an **Alpine FlatImage as a toolbox** to provide all necessary build tools without installing anything on your host system.

---

## Overview

This example demonstrates:

- Using Alpine FlatImage as a portable toolbox for build dependencies
- Using the Blueprint (empty) FlatImage as a base
- Fetching and extracting an external Linux distribution
- Modifying the extracted system (removing specific packages)
- Dumping the root filesystem into a FlatImage layer
- Creating a completely custom portable gaming environment

---

## Prerequisites

Download the the Alpine FlatImage to create a toolbox:

```bash
wget -O toolbox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine-x86_64.flatimage
chmod +x ./toolbox.flatimage
```

Configure the toolbox with necessary permissions and tools:

```bash
# Bind mount the working directory so toolbox can access it
./toolbox.flatimage fim-bind add rw $PWD $PWD
# Grant network access
./toolbox.flatimage fim-perms set network
# Install required tools inside the Alpine container
./toolbox.flatimage fim-root apk add wget curl xz squashfs-tools-ng util-linux coreutils findutils
# Set default boot command so we can run commands directly
./toolbox.flatimage fim-boot set bash -c '"$@"' --
```

Now the toolbox can be used like a regular command without `fim-exec`!

---

## Step 1 - Download Blueprint FlatImage

The Blueprint image is an empty container with no pre-installed distribution:

```bash
wget -O batocera.flatimage https://github.com/flatimage/flatimage/releases/latest/download/blueprint-x86_64.flatimage
# Make it executable
chmod +x ./batocera.flatimage
# Set permissions
./batocera.flatimage fim-perms set media,audio,wayland,xorg,dev,udev,dbus_user,usb,input,gpu,network
```

---

## Step 2 - Download Batocera Image

Use the Alpine toolbox to fetch the latest Batocera Linux image:

```bash
# Download Batocera (x86_64 version) using the toolbox
./toolbox.flatimage wget -O boot.tar.xz https://mirrors.o2switch.fr/batocera/x86_64/stable/last/boot.tar.xz
```

---

## Step 3 - Extract Batocera Root Filesystem

Use the Alpine toolbox to extract the SquashFS filesystem without needing root:

```bash
# Extract the tarball
tar xf boot.tar.xz boot/batocera.update
# Extract the sqfs image
./toolbox.flatimage rdsquashfs -C -u / -p ./root ./boot/batocera.update
# Set executable permission bits for binaries
chmod -R 755 root/bin root/usr/bin
```

---

## Step 4 - Remove Unused Packages

Use the Alpine toolbox to remove Nvidia proprietary drivers and other packages. Nvidia drivers are bound from the host, so there is no need to keep the ones in the guest.

```bash
# Remote builtin nvidia drivers
./toolbox.flatimage find root -iname "*nvidia*" -exec rm -rf "{}" \; || true
# Remote kernel modules
rm -rf root/lib/modules
# Remote firmware files
rm -rf root/lib/firmware
```

---

## Step 5 - Create required files/directories

```bash
mkdir -p root/userdata/system/logs
mkdir -p root/var/run
```

--

## Step 6 - Create FlatImage Layer from Batocera Root

Create a layer from the root directory and append it to the image.

```bash
# Create layer
./batocera.flatimage fim-layer create ./root root.layer
# Append layer
./batocera.flatimage fim-layer add root.layer
```

---

## Step 7 - Configure Environment

Set up the Batocera environment:

```bash
# Set environment variables
./batocera.flatimage fim-env add \
  'HOME=/userdata' \
  'USER=batocera' \
  'XDG_CONFIG_HOME=/userdata/system/configs' \
  'XDG_CACHE_HOME=/userdata/system/cache' \
  'XDG_DATA_HOME=/userdata/system/data'
```

---

## Step 8 - Set Boot Command

Configure Batocera's startup command as `emulationstation`:

```bash
./batocera.flatimage fim-boot set emulationstation
```

---

## Step 9 - Desktop Integration (Optional)

Create desktop integration for launching Batocera:

```bash
# Create desktop integration configuration
tee batocera.json <<-'EOF'
{
  "name": "batocera",
  "icon": "https://upload.wikimedia.org/wikipedia/commons/e/ef/Batocera-logo-art.png",
  "categories": ["Game"],
}
EOF

# Setup desktop integration
./batocera.flatimage fim-desktop setup ./batocera.json

# Enable integrations
./batocera.flatimage fim-desktop enable icon,entry,mimetype

# Enable startup notification
./batocera.flatimage fim-notify on
```

---

## Running Batocera

Launch Batocera with:

```bash
./batocera.flatimage
```

All Batocera data (ROMs, saves, configurations) will be stored in the `.batocera.flatimage.data` directory, making it fully portable.

---

## Complete Script

Here's a complete automated script combining all steps using the Alpine toolbox:

```bash
#!/bin/bash
set -euo pipefail

# Step 0 - Download and configure Alpine toolbox
echo "Setting up Alpine toolbox..."
wget -O toolbox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine-x86_64.flatimage
chmod +x ./toolbox.flatimage

# Configure the toolbox
./toolbox.flatimage fim-bind add rw $PWD $PWD
./toolbox.flatimage fim-perms set network
./toolbox.flatimage fim-root apk add wget curl xz squashfs-tools-ng util-linux coreutils findutils || true
./toolbox.flatimage fim-boot set bash -c '"$@"' --

# Step 1 - Download Blueprint FlatImage
echo "Downloading Blueprint FlatImage..."
wget -O batocera.flatimage https://github.com/flatimage/flatimage/releases/latest/download/blueprint-x86_64.flatimage
chmod +x ./batocera.flatimage
./batocera.flatimage fim-perms set media,audio,wayland,xorg,dev,udev,dbus_user,usb,input,gpu,network

# Step 2 - Download Batocera Image
echo "Downloading Batocera image..."
./toolbox.flatimage wget -O boot.tar.xz https://mirrors.o2switch.fr/batocera/x86_64/stable/last/boot.tar.xz

# Step 3 - Extract Batocera Root Filesystem
echo "Extracting Batocera root filesystem..."
tar xf boot.tar.xz boot/batocera.update
./toolbox.flatimage rdsquashfs -C -u / -p ./root ./boot/batocera.update
chmod -R 755 root/bin root/usr/bin

# Step 4 - Remove Unused Packages
echo "Removing unused packages..."
./toolbox.flatimage find root -iname "*nvidia*" -exec rm -rf "{}" \; || true
rm -rf root/lib/modules
rm -rf root/lib/firmware

# Step 5 - Create required files/directories
mkdir -p root/userdata/system/logs
mkdir -p root/var/run

# Step 6 - Create FlatImage Layer from Batocera Root
echo "Creating FlatImage layer..."
./batocera.flatimage fim-layer create ./root root.layer
./batocera.flatimage fim-layer add root.layer

# Step 7 - Configure Environment
echo "Configuring environment..."
./batocera.flatimage fim-env add \
  'HOME=/userdata' \
  'USER=batocera' \
  'XDG_CONFIG_HOME=/userdata/system/configs' \
  'XDG_CACHE_HOME=/userdata/system/cache' \
  'XDG_DATA_HOME=/userdata/system/data'

# Step 8 - Set Boot Command
echo "Setting boot command..."
./batocera.flatimage fim-boot set emulationstation

# Step 9 - Desktop Integration (Optional)
echo "Setting up desktop integration..."
tee batocera.json <<-'EOF'
{
  "name": "batocera",
  "icon": "https://upload.wikimedia.org/wikipedia/commons/e/ef/Batocera-logo-art.png",
  "categories": ["Game"],
}
EOF

./batocera.flatimage fim-desktop setup ./batocera.json
./batocera.flatimage fim-desktop enable icon,entry,mimetype
./batocera.flatimage fim-notify on

echo "Batocera FlatImage setup complete!"
echo "Run with: ./batocera.flatimage"
```

---

## Next Steps

### Setup Bindings

* By default use RO, but for scraping use RW
* List bindings with `fim-bind list`
* Erase bindings with `fim-bind del <index>`

```bash
./batocera.flatimage fim-bind add ro '$FIM_DIR_DATA/batocera/roms' /userdata/roms
./batocera.flatimage fim-bind add ro '$FIM_DIR_DATA/batocera/bios' /userdata/bios
```

### Bios and Roms

In the same directory as `batocera.flatimage` create the rom/bios folders.

```bash
mkdir -p .batocera.flatimage.data/batocera/roms .batocera.flatimage.data/batocera/bios
```

Now copy the bios files and rom files to these directories, for example:

```txt
.batocera.flatimage.data
└── batocera
    ├── bios
    │   ├──cph1001.bin
    │   ├── ...
    │   ├──cph5502.bin
    │   └──cph7001.bin
    └── roms
        └── psx
            └── game.chd

```

---

## References

- [Batocera Linux Official Site](https://batocera.org/)
- [Batocera Documentation](https://wiki.batocera.org/)
- [FlatImage Layer Commands](../../cmd/layer.md)
- [FlatImage Permissions](../../cmd/perms.md)
