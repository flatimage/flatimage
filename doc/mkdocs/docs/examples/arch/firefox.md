# Arch - Firefox

This example shows how to create a portable Firefox installation using Arch Linux as the base distribution. Two methods are provided: using recipes for simplified installation, and manual package installation for full control.

---

## Method 1: Using Recipes

Recipes provide a simplified way to install common package groups with automatic dependency resolution.

### Step 1 - Download FlatImage

Download the Arch Linux-based FlatImage:

```bash
wget -O firefox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/arch-x86_64.flatimage
chmod +x ./firefox.flatimage
```

### Step 2 - Configure Recipe Remote

Set up the recipe repository URL:

```bash
./firefox.flatimage fim-remote set https://raw.githubusercontent.com/flatimage/recipes/master
```

### Step 3 - Configure Permissions

Grant Firefox access to necessary system resources:

```bash
./firefox.flatimage fim-perms set wayland,xorg,audio,gpu,network
```

### Step 4 - Install the Firefox Recipe

Install Firefox and dependencies using pre-configured recipes:

```bash
./firefox.flatimage fim-recipe install firefox
```

### Step 5 (Optional) - Enable Desktop Integration

Enable the desktop integration that was set up during recipe installation:

```bash
./firefox.flatimage fim-desktop enable entry,icon,mimetype
```

### Step 6 - Set Boot Command

Configure Firefox as the default application:

```bash
./firefox.flatimage fim-boot set firefox
```

### Step 7 - Compress the Image

Commit all changes to a compressed read-only layer:

```bash
./firefox.flatimage fim-layer commit binary
```

---

## Method 2: Manual Package Installation

This method provides full control over package selection without using recipes.

### Step 1 - Download FlatImage

Download the Arch Linux-based FlatImage:

```bash
wget -O firefox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/arch-x86_64.flatimage
chmod +x ./firefox.flatimage
```

### Step 2 - Configure Permissions

Grant Firefox access to necessary system resources.

```bash
./firefox.flatimage fim-perms set wayland,xorg,audio,gpu,network
```

### Step 3 - Update Package Database

Update the Arch package database:

```bash
./firefox.flatimage fim-root pacman -Syy --noconfirm
```

### Step 4 - Install Xorg and Dependencies Manually

Install X11 server and libraries:

```bash
# Install Xorg server and utilities
./firefox.flatimage fim-root pacman -S --noconfirm \
  xorg-server \
  xorg-apps \
  xorg-xinit \
  xorg-xrandr

# Install X11 libraries
./firefox.flatimage fim-root pacman -S --noconfirm \
  libx11 libxext libxrender libxrandr libxinerama \
  libxi libxfixes libxdamage libxcomposite libxcursor \
  libxkbcommon libxkbcommon-x11 libxtst libxss \
  mesa
```

### Step 5 - Install GPU Drivers

Install GPU drivers for hardware acceleration:

```bash
# Mesa drivers (AMD, Intel, and software rendering)
./firefox.flatimage fim-root pacman -S --noconfirm \
  vulkan-radeon \
  vulkan-intel \
  vulkan-icd-loader

# Additional driver support
./firefox.flatimage fim-root pacman -S --noconfirm \
  libva libva-intel-driver \
  xf86-video-amdgpu \
  xf86-video-intel
```

### Step 6 - Install Firefox and Fonts

Install Firefox with font support:

```bash
./firefox.flatimage fim-root pacman -S --noconfirm \
  firefox \
  noto-fonts \
  noto-fonts-emoji \
  ttf-dejavu \
  ttf-liberation
```

### Step 7 - Set Boot Command

Configure Firefox as the default application:

```bash
./firefox.flatimage fim-boot set firefox
```

### Step 8 - Compress the Image

Commit all changes to a compressed read-only layer:

```bash
./firefox.flatimage fim-layer commit binary
```

## Optional Additional Configuration

### Configure Alternative HOME

Set up the user environment and home directory:

```bash
./firefox.flatimage fim-exec mkdir -p /home/firefox
./firefox.flatimage fim-env add 'USER=firefox' \
  'HOME=/home/firefox' \
  'XDG_CONFIG_HOME=/home/firefox/.config' \
  'XDG_DATA_HOME=/home/firefox/.local/share'
```

### Desktop Integration

Set up desktop menu integration and icon:

```bash
# Create desktop integration configuration
tee firefox.json <<-'EOF'
{
  "name": "firefox",
  "icon": "https://upload.wikimedia.org/wikipedia/commons/a/a0/Firefox_logo%2C_2019.svg",
  "categories": ["Network", "WebBrowser"],
  "integrations": ["ENTRY", "MIMETYPE", "ICON"]
}
EOF

# Apply desktop integration
./firefox.flatimage fim-desktop setup ./firefox.json
./firefox.flatimage fim-desktop enable icon,mimetype,entry

# Enable startup notifications
./firefox.flatimage fim-notify on
```

---

## Running Firefox

After completing either method, run Firefox with:

```bash
./firefox.flatimage
```

All Firefox data (profiles, cache, settings) will be stored in the `.firefox.flatimage.data` directory alongside the executable, making it fully portable across different Linux distributions.