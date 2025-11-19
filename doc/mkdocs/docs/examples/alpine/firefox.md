# Alpine - Firefox

This example shows how to create a portable Firefox installation using Alpine Linux as the base distribution. Two methods are provided: using recipes for simplified installation, and manual package installation for full control.

---

## Method 1: Using Recipes

Recipes provide a simplified way to install common package groups with automatic dependency resolution.

### Step 1 - Download FlatImage

Download the Alpine Linux-based FlatImage:

```bash
wget -O firefox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
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

Install Xorg and other dependencies using pre-configured recipes:

```bash
./firefox.flatimage fim-recipe install firefox
```

### Step 5 - Set Boot Command

Configure Firefox as the default application:

```bash
./firefox.flatimage fim-boot set firefox
```

### Step 6 - Compress the Image

Commit all changes to a compressed read-only layer:

```bash
./firefox.flatimage fim-layer commit
```

---

## Method 2: Manual Package Installation

This method provides full control over package selection without using recipes.

### Step 1 - Download FlatImage

Download the Alpine Linux-based FlatImage:

```bash
wget -O firefox.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
chmod +x ./firefox.flatimage
```

### Step 2 - Configure Permissions

Grant Firefox access to necessary system resources.

```bash
./firefox.flatimage fim-perms set wayland,xorg,audio,gpu,network
```

### Step 3 - Update Package Database

Update the Alpine package database:

```bash
./firefox.flatimage fim-root apk update
```

### Step 4 - Install Xorg and Dependencies Manually

Install X11 server and libraries:

```bash
# Install Xorg server and utilities
./firefox.flatimage fim-root apk add --no-cache \
  xorg-server \
  xorg-server-common \
  xf86-input-libinput \
  xf86-input-evdev \
  xinit \
  xauth \
  xhost \
  xrandr \
  xrdb \
  xset

# Install X11 libraries
./firefox.flatimage fim-root apk add --no-cache \
  libx11 libxext libxrender libxrandr libxinerama \
  libxi libxfixes libxdamage libxcomposite libxcursor \
  libxkbcommon libxkbcommon-x11 libxtst libxscrnsaver \
  mesa mesa-gl mesa-dri-gallium mesa-egl mesa-gles
```

### Step 5 - Install GPU Drivers

Install GPU drivers for hardware acceleration:

```bash
# Mesa drivers (AMD, Intel, and software rendering)
./firefox.flatimage fim-root apk add --no-cache \
  mesa-va-gallium \
  mesa-vulkan-ati \
  mesa-vulkan-intel \
  vulkan-loader

# Additional driver support
./firefox.flatimage fim-root apk add --no-cache \
  libva libva-intel-driver \
  libdrm
```

### Step 6 - Install Firefox and Fonts

Install Firefox with font support:

```bash
./firefox.flatimage fim-root apk add --no-cache \
  firefox \
  font-noto \
  font-noto-emoji \
  ttf-dejavu \
  ttf-liberation
```

### Step 7 - Set Boot Command

Configure Firefox as the default application:

```bash
./firefox.flatimage fim-boot set /usr/bin/firefox
```

### Step 8 - Compress the Image

Commit all changes to a compressed read-only layer:

```bash
./firefox.flatimage fim-layer commit
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
# Download Firefox icon
wget -O firefox.svg "https://upload.wikimedia.org/wikipedia/commons/a/a0/Firefox_logo%2C_2019.svg"

# Create desktop integration configuration
tee firefox.json <<-'EOF'
{
  "name": "firefox",
  "icon": "./firefox.svg",
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