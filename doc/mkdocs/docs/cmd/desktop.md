# Desktop Integration

## What is it?

Desktop integration allows FlatImage applications to behave like native desktop applications by adding:

- **Menu entries** - Application appears in your desktop's start menu/application launcher
- **Application icons** - Custom icon displayed in menus and file managers
- **MIME type associations** - File manager shows FlatImage-specific file types with custom icons
- **File associations** - Double-click to open files with your FlatImage application

**Use Cases:**

- Making portable applications feel native to the desktop environment
- Creating professional application distributions
- Allowing users to launch applications from GUI menus instead of terminal
- Associating file types with your packaged application
- Improving discoverability of installed FlatImage applications

## How to Use

You can use `./app.flatimage fim-help desktop` to get the following usage details:

```txt
fim-desktop : Configure the desktop integration
Usage: fim-desktop <setup> <json-file>
  <setup> : Sets up the desktop integration with an input json file
  <json-file> : Path to the json file with the desktop configuration
Usage: fim-desktop <enable> [entry,mimetype,icon,none]
  <enable> : Enables the desktop integration selectively
  <entry> : Enables the start menu desktop entry
  <mimetype> : Enables the mimetype
  <icon> : Enables the icon for the file manager and desktop entry
  <none> : Disables desktop integrations
Usage: fim-desktop <clean>
  <clean> : Cleans the desktop integration files from XDG_DATA_HOME
Usage: fim-desktop <dump> <icon> <file>
  <dump> : Dumps the selected integration data
  <icon> : Dumps the desktop icon to a file
  <file> : Path to the icon file, the extension is appended automatically if not specified
Usage: fim-desktop <dump> <entry|mimetype>
  <dump> : Dumps the selected integration data
  <entry> : The desktop entry of the application
  <mimetype> : The mime type of the application
```

### Setup Desktop Integration

#### Step 1: Create Configuration File

Create a JSON file defining your desktop integration settings:

**Using a remote icon**: Downloaded during the setup process.

```json
{
  "name": "MyApp",
  "icon": "https://upload.wikimedia.org/wikipedia/commons/a/a0/Firefox_logo%2C_2019.svg",
  "categories": ["System", "Audio"],
  "integrations": ["ENTRY", "MIMETYPE", "ICON"]
}
```

**Using a local icon**: Read from a local file.

```json
{
  "name": "MyApp",
  "icon": "./my_app.png",
  "categories": ["System", "Audio"],
  "integrations": ["ENTRY", "MIMETYPE", "ICON"]
}
```

**Configuration Fields:**

- **name** - Application name (shown in menus and file managers)
- **icon** - Path or URL to icon file (PNG or SVG, relative to JSON file location)
- **categories** - Desktop menu categories (see [valid categories](https://specifications.freedesktop.org/menu-spec/latest/category-registry.html))
- **integrations** - Which integrations to enable (ENTRY, MIMETYPE, ICON)

**Common Category Combinations:**

- **Development tools**: `["Development"]`
- **Graphics applications**: `["Graphics", "2DGraphics"]`
- **Audio applications**: `["Audio", "AudioVideo"]`
- **Video applications**: `["Video", "AudioVideo"]`
- **Internet browsers**: `["Network", "WebBrowser"]`
- **Games**: `["Game"]`
- **Office applications**: `["Office"]`
- **System utilities**: `["System", "Utility"]`

#### Step 2: Apply Configuration

Apply the desktop integration configuration to your FlatImage:

```bash
# Apply the configuration
./app.flatimage fim-desktop setup ./desktop.json
```

This stores the integration metadata inside the FlatImage binary.

#### Step 3: Enable Integrations

Activate the desktop integration features:

```bash
# Enable all integration features
./app.flatimage fim-desktop enable entry,mimetype,icon
```

On the next execution, FlatImage will automatically create the necessary desktop files in `$XDG_DATA_HOME` (typically `~/.local/share`).

**Selective enabling:**

```bash
# Enable only menu entry and icon
./app.flatimage fim-desktop enable entry,icon

# Enable only MIME type
./app.flatimage fim-desktop enable mimetype

# Disable all integrations
./app.flatimage fim-desktop enable none
```

### Managing Integration Files

#### View Generated Desktop Entry

Inspect the generated .desktop file:

```bash
./app.flatimage fim-desktop dump entry
```

**Example output:**

```desktop
[Desktop Entry]
Name=MyApp
Type=Application
Comment=FlatImage distribution of "MyApp"
Exec="/path/to/app.flatimage" %F
Icon=flatimage_MyApp
MimeType=application/flatimage_MyApp;
Categories=Network;System;
```

**Key fields:**

- **Exec** - Full path to your FlatImage (auto-updates if you move the file)
- **Icon** - Icon identifier used by the desktop environment
- **Categories** - Menu categories (from your configuration)

#### View MIME Type Definition

Inspect the generated MIME type XML:

```bash
./app.flatimage fim-desktop dump mimetype
```

**Example output:**

```xml
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="application/flatimage_MyApp">
    <comment>FlatImage Application</comment>
    <glob weight="100" pattern="app.flatimage"/>
    <sub-class-of type="application/x-executable"/>
    <generic-icon name="application-flatimage"/>
  </mime-type>
</mime-info>
```

This makes your FlatImage file identifiable by the desktop environment.

#### Export the Icon

Extract the icon file for inspection or reuse:

```bash
# Export icon (extension added automatically)
./app.flatimage fim-desktop dump icon my-icon

# Creates: my-icon.png or my-icon.svg depending on original format
```

#### Remove Desktop Integration

Clean up all desktop integration files:

```bash
# Remove all generated files
./app.flatimage fim-desktop clean

# Optionally disable to prevent recreation
./app.flatimage fim-desktop enable none
```

This removes files from:
- `$XDG_DATA_HOME/applications/`
- `$XDG_DATA_HOME/mime/packages/`
- `$XDG_DATA_HOME/icons/hicolor/`

### Integration File Locations

Desktop integration creates files in standard XDG locations:

**Base Directory:**

- `$XDG_DATA_HOME` (defaults to `~/.local/share` if undefined)

**Desktop Entry:**

- `$XDG_DATA_HOME/applications/flatimage-MyApp.desktop`

**MIME Type Definitions:**

- `$XDG_DATA_HOME/mime/packages/flatimage-MyApp.xml` (application-specific)
- `$XDG_DATA_HOME/mime/packages/flatimage.xml` (generic FlatImage type)

**Application Icons (SVG):**

- `$XDG_DATA_HOME/icons/hicolor/scalable/apps/flatimage_MyApp.svg`
- `$XDG_DATA_HOME/icons/hicolor/scalable/mimetypes/application-flatimage_MyApp.svg`

**Application Icons (PNG):**

- `$XDG_DATA_HOME/icons/hicolor/{size}/apps/flatimage_MyApp.png`
- `$XDG_DATA_HOME/icons/hicolor/{size}/mimetypes/application-flatimage_MyApp.png`

Where `{size}` = 16, 22, 24, 32, 48, 64, 96, 128, 256

## How it Works

### Technical Details

**Desktop Entry Creation:**

When `entry` is enabled, FlatImage creates a `.desktop` file following the [Desktop Entry Specification](https://specifications.freedesktop.org/desktop-entry-spec/latest/). The entry includes:

- Absolute path to the FlatImage executable (auto-updates on execution)
- Application name and comment
- Icon identifier
- Categories for menu placement

**MIME Type Registration:**

When `mimetype` is enabled, FlatImage creates XML files following the [Shared MIME Info Specification](https://specifications.freedesktop.org/shared-mime-info-spec/latest/):

- Application-specific MIME type (matches exact filename)
- Generic FlatImage MIME type (matches `*.flatimage` pattern)

The MIME database updates automatically on first execution and when the filename changes.

**Icon Installation:**

When `icon` is enabled, FlatImage installs icon files for:

- Application launchers (apps directory)
- File type associations (mimetypes directory)

**SVG icons:** Installed to `scalable/` directory
**PNG icons:** Automatically scaled and installed to multiple size directories (16×16 through 256×256)

**Path Auto-Update:**

FlatImage checks if the desktop entry path matches the current executable path on every launch. If different:

1. Updates the `.desktop` file with new path
2. Updates the MIME type glob pattern if filename changed
3. Notifies desktop environment of changes

This ensures desktop integration remains functional even after moving or renaming the file.