# Desktop Integration

## What is it?

The desktop integration allows flatimage programs to integrate themselves into the
start menu, show the application icon in the file manager and also define its
own mimetype.

## How to Use

**Configure**

You can use `./app.flatimage fim-help desktop` to get the following usage details:

```
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

**Setup the desktop integration**

To setup the desktop integration for a flatimage package, the first step is to
create a `json` file with the integration data, assume we create a file named
`desktop.json` with the following contents:

```json
{
  "name": "MyApp",
  "icon": "./my_app.png",
  "categories": ["System","Audio"],
  "integrations": ["ENTRY", "MIMETYPE", "ICON"]
}
```

This example creates the integration data for the application `MyApp`, with the
icon file `my_app.png` located in the same folder as `desktop.json`. The
categories field is used for the desktop menu entry integration, a list of valid
categories is found
[here](https://specifications.freedesktop.org/menu-spec/latest/category-registry.html#main-category-registry).
Let's assume the json file is called `desktop.json` and the flatimage file is
called `app.flatimage`, the next command uses the `desktop.json` file to
configure the desktop integration.

```bash
$ ./app.flatimage fim-desktop setup ./desktop.json
```

**Enable/Disable integration files**

After the setup step, you can enable the integration selectively; `entry` refers
to the desktop entry in the start menu; `mimetype` refers to the mime package file
that defines the application type, usually shown in file managers in the 'type' column;
`icon` is the application icon shown in the start menu (entry) and the file manager (type).
Here's how to enable everything:

```bash
$ ./app.flatimage fim-desktop enable entry,mimetype,icon
```

To disable all desktop integrations, use:

```bash
$ ./app.flatimage fim-desktop enable none
```

**Dump integration files**

To dump the desktop entry:

```bash
# For the desktop entry
$ ./app.flatimage fim-desktop dump entry
[Desktop Entry]
Name=MyApp
Type=Application
Comment=FlatImage distribution of "MyApp"
Exec="/path/to/app.flatimage" %F
Icon=flatimage_MyApp
MimeType=application/flatimage_MyApp;
Categories=Network;System;
```

To dump the mime type `XML` file:

```bash
# For the mime type package file
$ ./app.flatimage fim-desktop dump mimetype
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

To dump the desktop application icon:

```bash
# This outputs the integrated icon to a file called out.[png or svg] depending on the icon format used
# during the setup step.
$ ./app.flatimage fim-desktop dump icon out
```

**Erase entries**

To erase all desktop integration files created by flatimage, use the command:

```bash
$ ./app.flatimage fim-desktop clean
# (optional) Disable desktop integration to avoid the creation of files on the next execution
$ ./app.flatimage fim-desktop enable none
```

**xdg-open**

Flatimage redirects `xdg-open` commands to the host machine

Examples:

* Open a video file with the host default video player: `xdg-open my-file.mkv`
* Open a link with the host default browser: `xdg-open www.google.com`

## How it Works

Consider the following integration setup:

```json
{
  "name": "MyApp",
  "icon": "./my_app.png",
  "categories": ["System","Audio"],
  "integrations": ["ENTRY", "MIMETYPE", "ICON"]
}
```

**XDG_DATA_HOME**

* If `XDG_DATA_HOME` is undefined, it falls back to `$HOME/.local/share`.
* If `HOME` is undefined, the integration does not take place.

**Desktop Entry**

Enabled with `fim-desktop enable entry`, this option creates a desktop entry in
`$XDG_DATA_HOME/applications/flatimage-MyApp.desktop` that executes the application
from its current path. If the file is moved or its name is changed, the next
execution updates the desktop entry with the correct path to the application.

**Mime Type**

Enabled with `fim-desktop enable mimetype`, this option creates two files:

1. `$XDG_DATA_HOME/mime/packages/flatimage-MyApp.xml`: This is a mime type specific for
the packaged application, it matches the base name of the full path and is required
to show an application specific icon in the file manager.
2. `$XDG_DATA_HOME/mime/packages/flatimage.xml`: This is a generic FlatImage mime type
that match files with the glob pattern `*.flatimage`.

The mime database is updated after the first execution after the desktop integration is
configured with `fim-desktop setup ...`, and also when the basename of the application file
changes.

**Icon**

This enables the integration of the `FlatImage` generic icon, and the application specific
icon.

For the application specific icons, in case they are in the `svg` format:

- `$XDG_DATA_HOME/icons/hicolor/scalable/mimetypes/application-flatimage_MyApp.svg`
- `$XDG_DATA_HOME/icons/hicolor/scalable/apps/flatimage_MyApp.svg`

For the application specific icons, in case they are in the `png` format:

- `$XDG_DATA_HOME/icons/hicolor/{}x{}/mimetypes/application-flatimage_MyApp.png`
- `$XDG_DATA_HOME/icons/hicolor/{}x{}/apps/application-flatimage_MyApp.png`

Where the placeholders `{}x{}` are replaced by equal values of `16,22,24,32,48,64,96,128,256`.