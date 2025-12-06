# Arch - Steam

This example shows how to create a portable Steam FlatImage using Arch Linux as the base distribution. The image works on any Linux distribution, stores all configurations in a portable folder, and doesn't pollute the user's HOME directory. Two methods are provided: using recipes for simplified installation, and manual package installation for full control.

---

## Method 1: Using Recipes

Recipes provide a simplified way to install common package groups with automatic dependency resolution.

### Step 1 - Download FlatImage

Download the Arch Linux-based FlatImage:

```bash
wget -O steam.flatimage https://github.com/flatimage/flatimage/releases/latest/download/arch.flatimage
chmod +x ./steam.flatimage
```

### Step 2 - Configure Recipe Remote

Set up the recipe repository URL:

```bash
./steam.flatimage fim-remote set https://raw.githubusercontent.com/flatimage/recipes/master
```

### Step 3 - Configure Permissions

Grant Steam access to necessary system resources:

```bash
./steam.flatimage fim-perms set wayland,xorg,audio,gpu,network,dbus_user
```

### Step 4 - Install Steam Using Recipes

Install Steam and dependencies using pre-configured recipes:

```bash
./steam.flatimage fim-recipe install steam
```

### Step 5 - Set Boot Command

Configure Steam as the default application:

```bash
./steam.flatimage fim-boot set steam
```

### Step 6 - Compress the Image

Commit all changes to a compressed read-only layer:

```bash
./steam.flatimage fim-layer commit binary
```

---

## Method 2: Manual Package Installation

This method provides full control over package selection without using recipes.

### Step 1 - Download FlatImage

Download the Arch Linux-based FlatImage:

```bash
wget -O steam.flatimage https://github.com/flatimage/flatimage/releases/latest/download/arch.flatimage
chmod +x ./steam.flatimage
```

### Step 2 - Configure Permissions

Grant Steam access to necessary system resources.

```bash
./steam.flatimage fim-perms set wayland,xorg,audio,gpu,network,dbus_user
```

### Step 3 - Update Package Database

Update the Arch package database:

```bash
./steam.flatimage fim-root pacman -Syy --noconfirm
```

### Step 4 - Install Xorg and Dependencies Manually

Install X11 server and libraries:

```bash
./steam.flatimage fim-root pacman -S --noconfirm xorg xorg-apps \
  mesa lib32-mesa glxinfo lib32-gcc-libs gcc-libs pcre freetype2 lib32-freetype2 \
  libx11 libxext libxrender libxrandr libxinerama \
  libxi libxfixes libxdamage libxcomposite libxcursor \
  libxkbcommon libxkbcommon-x11 libxtst libxss
```

### Step 5 - Install GPU Drivers

Install GPU drivers for hardware acceleration:

```bash
# Amd drivers
./steam.flatimage fim-root pacman -S --noconfirm xf86-video-amdgpu vulkan-radeon lib32-vulkan-radeon vulkan-tools
# Intel drivers
./steam.flatimage fim-root pacman -S --noconfirm xf86-video-intel vulkan-intel lib32-vulkan-intel vulkan-tools
```

### Step 6 - Install Steam

Install Steam:

```bash
./steam.flatimage fim-root pacman -S --noconfirm steam
```

### Step 7 - Set Boot Command

Configure Steam as the default application:

```bash
./steam.flatimage fim-boot set steam
```

### Step 8 - Compress the Image

Commit all changes to a compressed read-only layer:

```bash
./steam.flatimage fim-layer commit binary
```

## Optional Additional Configuration

### Configure Alternative HOME

Set up the user environment and home directory:

```bash
./steam.flatimage fim-exec mkdir -p /home/steam
./steam.flatimage fim-env add 'USER=steam' \
  'HOME=/home/steam' \
  'XDG_CONFIG_HOME=/home/steam/.config' \
  'XDG_DATA_HOME=/home/steam/.local/share'
```

### Desktop Integration

Set up desktop menu integration and icon:

```bash
# Create desktop integration configuration
tee steam.json <<-'EOF'
{
  "name": "steam",
  "icon": "https://upload.wikimedia.org/wikipedia/commons/8/83/Steam_icon_logo.svg",
  "categories": ["Game"],
  "integrations": ["ENTRY", "MIMETYPE", "ICON"]
}
EOF

# Apply desktop integration
./steam.flatimage fim-desktop setup ./steam.json
./steam.flatimage fim-desktop enable icon,mimetype,entry

# Enable startup notifications
./steam.flatimage fim-notify on
```
## Running Steam

After completing either method, run Steam with:

```bash
./steam.flatimage
```

All steam data will be stored in the `.steam.flatimage.data` directory alongside the executable, making it fully portable across different Linux distributions.