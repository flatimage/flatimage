# Create a Portable Steam Executable

This example shows how to create a portable steam FlatImage which works directly on a hard-drive for usage in different computers, and also keeps all it's configurations in the same folder, no poluting the user's HOME directory.

## Step 1 - Get FlatImage

Download the arch linux binary:

```bash
wget -O steam.flatimage https://github.com/flatimage/flatimage/releases/latest/download/arch.flatimage
chmod +x ./steam.flatimage
```

## Step 2 - Configure permissions

This steps configures what Steam can access on the host. This ranges from media folders, creating windows, devices, inter-process communication, usb devices, graphics cards, and network access.

```bash
./steam.flatimage fim-perms set media,audio,wayland,xorg,udev,dbus_user,usb,input,gpu,network
```

## Step 3 - Install dependencies

This step installs steam dependencies, Xorg server, and video drivers. Install both Amd and Intel drivers, to use the image in computers with different GPU vendors. Nvidia drivers are symlinked from the host, no need to install.

```bash
# Update package lists
./steam.flatimage fim-root pacman -Syyu --noconfirm
# Xorg
./steam.flatimage fim-root pacman -S --noconfirm xorg-server mesa lib32-mesa glxinfo lib32-gcc-libs gcc-libs pcre freetype2 lib32-freetype2
# Amd drivers
./steam.flatimage fim-root pacman -S --noconfirm xf86-video-amdgpu vulkan-radeon lib32-vulkan-radeon vulkan-tools
# Intel drivers
./steam.flatimage fim-root pacman -S --noconfirm xf86-video-intel vulkan-intel lib32-vulkan-intel vulkan-tools
```

## Step 4 - Install Steam

This step installs steam and sets it as the default start-up application.

```bash
# Install the steam package
./steam.flatimage fim-root pacman -S --noconfirm steam
# Clear cache
./steam.flatimage fim-root pacman -Scc --noconfirm
# Set command to run by default
./steam.flatimage fim-boot set /usr/bin/steam
```

## Step 5 - Configure Environment Variables

This step defines steam's home directory inside the container, the user name, and the configuration and [XDG](https://wiki.archlinux.org/title/XDG_Base_Directory) directories.

```bash
./steam.flatimage fim-exec mkdir -p /home/steam
./steam.flatimage fim-env add 'USER=steam' 'HOME=/home/steam' 'XDG_CONFIG_HOME=/home/steam/.config' 'XDG_DATA_HOME=/home/steam/.local/share'
```


## Step 6 - Configure Desktop Integration (Optional)

This step configures the desktop integrations to show the applications menu Steam entry and file manager icon. It also enables a notification when the application is started.

```bash
# Download steam icon
wget -O steam.svg https://upload.wikimedia.org/wikipedia/commons/8/83/Steam_icon_logo.svg
# Create configuration json
tee steam.json <<-'EOF'
{
"name": "steam",
"icon": "./steam.svg",
"categories": ["Game"],
"integrations": ["ENTRY", "MIMETYPE", "ICON"]
}
EOF
# Configure the image with the created json
./steam.flatimage fim-desktop setup ./steam.json
# Notify the application has started (optional)
./steam.flatimage fim-notify on
```

## Step 7 - Compress the Image

This step compresses the steam installation to a read-only layer.

```bash
./steam.flatimage fim-commit
```

Read the full script to create the steam package [here](https://github.com/ruanformigoni/flatimage/blob/master/examples/steam.sh).
