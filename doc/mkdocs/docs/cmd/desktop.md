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
```

To setup the desktop integration for a flatimage package, the first step is to
create a `json` file with the integration data, assume we create a file named
`desktop.json` with the following contents:

```json
{
  "name": "MyApp",
  "icon": "./my_app.png",
  "categories": ["System","Audio"]
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

After the setup step, you can enable the integration selectively, `entry` refers
to the desktop entry in the start menu, `mimetype` refers to the file type that
appears in the file manager, `icon` is the application icon shown in the start
menu and the file manager. Here's how to enable everything:

```bash
$ ./app.flatimage fim-desktop enable entry,mimetype,icon
```

**Erase entries**

To erase all desktop entries and icons created by flatimage, you can use the
command:

```bash
$ find ~/.local/share -iname "*flatimage*" -exec rm -v "{}" \;
```

**xdg-open**

Flatimage redirects `xdg-open` commands to the host machine

Examples:

* Open a video file with the host default video player: `xdg-open my-file.mkv`
* Open a link with the host default browser: `xdg-open www.google.com`

## How it Works

FlatImage installs desktop entries in `$HOME/.local/share/applications`, icons
are installed in `$HOME/.local/share/icons` and mimetypes are installed in
`$HOME/.local/share/mime`. The user must define `XDG_DATA_HOME` to `$HOME/.local/share`
or `XDG_DATA_DIRS` to contain the path `$HOME/.local/share`.
