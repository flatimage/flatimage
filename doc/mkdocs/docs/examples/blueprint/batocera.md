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
wget -O toolbox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
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
wget -O batocera.flatimage https://github.com/flatimage/flatimage/releases/latest/download/blueprint.flatimage
# Make it executable
chmod +x ./batocera.flatimage
# Set permissions
./batocera.flatimage fim-perms set media,audio,wayland,xorg,udev,dbus_user,usb,input,gpu,network
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
./toolbox.flatimage find root -iname "*nvidia*" -exec rm -rf "{}" \;
# Remote kernel modules
rm -rf root/lib/modules
# Remote firmware files
rm -rf root/lib/firmware
```

---

## Step 5 - Create FlatImage Layer from Batocera Root

Create a layer from the root directory and append it to the image.

```bash
# Create layer
./batocera.flatimage fim-layer create ./root root.layer
# Append layer
./batocera.flatimage fim-layer add root.layer
```

---

## Step 6 - Configure Environment

Set up the Batocera environment:

```bash
# Create Batocera user home directory
./batocera.flatimage fim-exec mkdir -p /userdata
# Set environment variables
./batocera.flatimage fim-env add \
  'HOME=/userdata' \
  'USER=batocera' \
  'XDG_CONFIG_HOME=/userdata/system/.config' \
  'XDG_DATA_HOME=/userdata/system/.local/share'
```

---

## Step 7 - Set Boot Command

Configure Batocera's startup command as `emulationstation`:

```bash
./batocera.flatimage fim-boot set emulationstation
```

---

## Step 8 - Desktop Integration (Optional)

Create desktop integration for launching Batocera:

```bash
# Create desktop integration configuration
tee batocera.json <<-'EOF'
{
  "name": "batocera",
  "icon": "https://upload.wikimedia.org/wikipedia/commons/e/ef/Batocera-logo-art.png",
  "categories": ["Game"],
  "integrations": ["ENTRY", "ICON", "MIMETYPE"]
}
EOF

# Apply desktop integration
./batocera.flatimage fim-desktop setup ./batocera.json
./batocera.flatimage fim-desktop enable icon,entry,mimetype

# Enable startup notifications
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
wget -O toolbox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
chmod +x ./toolbox.flatimage

# Configure the toolbox
./toolbox.flatimage fim-bind add rw $PWD $PWD
./toolbox.flatimage fim-perms set network
./toolbox.flatimage fim-root apk add wget curl xz squashfs-tools-ng util-linux coreutils findutils || true
./toolbox.flatimage fim-boot set bash -c '"$@"' --

# Step 1 - Download Blueprint FlatImage
echo "Downloading Blueprint FlatImage..."
wget -O batocera.flatimage https://github.com/flatimage/flatimage/releases/latest/download/blueprint.flatimage
chmod +x ./batocera.flatimage
./batocera.flatimage fim-perms set media,audio,wayland,xorg,udev,dbus_user,usb,input,gpu,network

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
./toolbox.flatimage find root -iname "*nvidia*" -exec rm -rf "{}" \;
rm -rf root/lib/modules
rm -rf root/lib/firmware

# Step 5 - Create FlatImage Layer from Batocera Root
echo "Creating FlatImage layer..."
./batocera.flatimage fim-layer create ./root root.layer
./batocera.flatimage fim-layer add root.layer

# Step 6 - Configure Environment
echo "Configuring environment..."
./batocera.flatimage fim-exec mkdir -p /userdata
./batocera.flatimage fim-env add \
  'HOME=/userdata' \
  'USER=batocera' \
  'XDG_CONFIG_HOME=/userdata/system/.config' \
  'XDG_DATA_HOME=/userdata/system/.local/share'

# Step 7 - Set Boot Command
echo "Setting boot command..."
./batocera.flatimage fim-boot set emulationstation

# Step 8 - Desktop Integration (Optional)
echo "Setting up desktop integration..."
tee batocera.json <<-'EOF'
{
  "name": "batocera",
  "icon": "https://upload.wikimedia.org/wikipedia/commons/e/ef/Batocera-logo-art.png",
  "categories": ["Game"],
  "integrations": ["ENTRY", "ICON", "MIMETYPE"]
}
EOF

./batocera.flatimage fim-desktop setup ./batocera.json
./batocera.flatimage fim-desktop enable icon,entry,mimetype
./batocera.flatimage fim-notify on

echo "Batocera FlatImage setup complete!"
echo "Run with: ./batocera.flatimage"
```

---

## Advanced Customization

### Adding ROMs

Copy ROM files into the Batocera userdata directory:

```bash
# Copy ROMs from host
# ROMs are stored in the .batocera.flatimage.data directory
cp -r ~/roms/* ./.batocera.flatimage.data/userdata/roms/
```

---

## References

- [Batocera Linux Official Site](https://batocera.org/)
- [Batocera Documentation](https://wiki.batocera.org/)
- [FlatImage Layer Commands](../../cmd/layer.md)
- [FlatImage Permissions](../../cmd/perms.md)
