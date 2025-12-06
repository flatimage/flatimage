# Desktop Integration

Make your FlatImage applications appear in desktop menus with icons.

## Quick Setup

### 1. Create Configuration File

Create `desktop.json`:

```json
{
  "name": "MyApp",
  "icon": "./icon.png",
  "categories": ["Network", "WebBrowser"],
}
```

The field `integrations` is optional, it does the same as the 'enable' command, e.g., `"integrations": ["ENTRY", "MIMETYPE", "ICON"]`.

**Fields:**

- `name` - Application name shown in menus
- `icon` - Path or URL to icon file (JPG, PNG, or SVG)
- `categories` - Desktop menu categories
- `integrations` - Which integrations to enable

### 2. Apply Configuration

```bash
./app.flatimage fim-desktop setup ./desktop.json
```

### 3. Enable Integration

```bash
./app.flatimage fim-desktop enable entry,mimetype,icon
```

Done! Your application now appears in desktop menus.

## Common Categories

Choose appropriate categories for your application:

**Web Browsers:**
```json
"categories": ["Network", "WebBrowser"]
```

**Games:**
```json
"categories": ["Game"]
```

**Media Players:**
```json
"categories": ["AudioVideo", "Player"]
```

**Development Tools:**
```json
"categories": ["Development"]
```

**Graphics Applications:**
```json
"categories": ["Graphics", "2DGraphics"]
```

**Office Applications:**
```json
"categories": ["Office"]
```

See [valid categories](https://specifications.freedesktop.org/menu-spec/latest/category-registry.html) for complete list.

## Integration Options

Enable specific features:

```bash
# Enable only menu entry and icon
./app.flatimage fim-desktop enable entry,icon

# Enable all features
./app.flatimage fim-desktop enable entry,mimetype,icon

# Disable all integrations
./app.flatimage fim-desktop enable none
```

## Complete Example

Create a Firefox launcher with desktop integration:

```bash
# Download and setup
wget https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
chmod +x alpine.flatimage

# Configure permissions
./alpine.flatimage fim-perms set wayland,xorg,audio,network

# Install Firefox
./alpine.flatimage fim-remote set https://raw.githubusercontent.com/flatimage/recipes/master
./alpine.flatimage fim-recipe install firefox

# Set boot command
./alpine.flatimage fim-boot set firefox

# Create desktop configuration
cat > desktop.json << 'EOF'
{
  "name": "Firefox for Work",
  "icon": "https://upload.wikimedia.org/wikipedia/commons/a/a0/Firefox_logo%2C_2019.svg",
  "categories": ["Network", "WebBrowser"]
}
EOF

# Apply desktop integration
./alpine.flatimage fim-desktop setup ./desktop.json
./alpine.flatimage fim-desktop enable entry,icon,mimetype

# Commit changes
./alpine.flatimage fim-layer commit binary

# Rename
mv alpine.flatimage firefox.flatimage
```

Now Firefox appears in your application menu!

## Verify Integration

Check the generated integration files:

```bash
# Desktop entry
./firefox.flatimage fim-desktop dump entry
# Mimetype
./firefox.flatimage fim-desktop dump mimetype
```

View installed icon:

```bash
./firefox.flatimage fim-desktop dump icon ./exported-icon
```

## Remove Integration

Clean up desktop files:

```bash
# Remove all integration files
./firefox.flatimage fim-desktop clean

# Disable to prevent recreation
./firefox.flatimage fim-desktop enable none
```

## Where Files Are Created

Desktop integration creates files in `~/.local/share/`:

- `applications/flatimage-MyApp.desktop` - Menu entry
- `mime/packages/flatimage-MyApp.xml` - MIME types
- `icons/hicolor/*/apps/flatimage_MyApp.*` - Application icons
- `icons/hicolor/*/mimetypes/application-flatimage_MyApp.*` - File type icons

## Learn More

See [fim-desktop](../cmd/desktop.md) for complete documentation including:

- MIME type associations
- Auto-updating paths
- Technical details