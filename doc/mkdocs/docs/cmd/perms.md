# Configure permissions

## What is it?

It is a functionally to allow granular access to the host's resources. The
available permissions and their descriptions are as follows:

* home - Access to the host `HOME` directory, defined by the `HOME` environment
variable.
* media - Access to external storage devices.
    * Binds `[rw]` `/media -> /media`
    * Binds `[rw]` `/run/media -> /run/media`
    * Binds `[rw]` `/mnt -> /mnt`
* audio - Access to audio sockets.
    * Binds `[rw]` `$XDG_RUNTIME_DIR/pulse/native -> $XDG_RUNTIME_DIR/pulse/native`
    * Binds `[rw]` `$XDG_RUNTIME_DIR/pipewire-0 -> $XDG_RUNTIME_DIR/pipewire-0`.
* wayland - Access to the wayland socket.
    * Binds `[rw]` `$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY -> $XDG_RUNTIME_DIR/$WAYLAND_DISPLAY`
    * Requires defined variable `WAYLAND_DISPLAY` on the host.
* xorg - Access to the `xorg` socket.
    * Binds `[ro]` `$XAUTHORITY -> $XAUTHORITY`.
    * Requires defined variables `XAUTHORITY` and `DISPLAY` on the host.
* dbus_user - Allows access to the `session bus`, allowing desktop applications to
    interact with each other.
    * Binds `[rw]` `$DBUS_SESSION_BUS_ADDRESS -> $DBUS_SESSION_BUS_ADDRESS`.
    * Requires defined variable `DBUS_SESSION_BUS_ADDRESS`.
    * Firefox uses `dbus_user` to communicate with other instances (not a hard-requirement).
* dbus_system - System level messages
    * Binds `[rw]` `/run/dbus/system_bus_socket -> /run/dbus/system_bus_socket`.
* udev - Monitor device events, detect new hardware / hardware changes.
    * Binds `[rw]` `/run/udev -> /run/udev`.
* usb - Provides access to Universal Serial Bus `USB` devices.
    * Binds `[rw]` `/dev/usb -> /dev/usb`
    * Binds `[rw]` `/dev/bus/usb -> /dev/bus/usb`
* input - Binds input devices (joysticks, mouse, keyboard, etc)
    * Binds `[rw]` `/dev/input -> /dev/input`
    * Binds `[dev]` `/dev/uinput -> /dev/uinput`
* gpu - Allows access to GPU hardware
    * Binds `[rw]` `/dev/dri -> /dev/dri`
    * Symlinks nvidia drivers from the host to the container
* network - Configures network access
    * Binds `[ro]` `/etc/host.conf -> /etc/host.conf`
    * Binds `[ro]` `/etc/hosts -> /etc/hosts`
    * Binds `[ro]` `/etc/nsswitch.conf -> /etc/nsswitch.conf`
    * Binds `[ro]` `/etc/resolv.conf -> /etc/resolv.conf`
* dev - Binds the host `/dev` directory to the container.
    * Binds `[dev]` `/dev -> /dev`

If `XDG_RUNTIME_DIR` is undefined it defaults to `/run/user/$(id -u)`. The tag
after `Binds` indicates if the bind is `read-only [ro]`, `read-write [rw]` or a
`device [dev]`.

## How to use

You can use `./app.flatimage fim-help perms` to get the following usage details:

```txt
fim-perms : Edit current permissions for the flatimage
Note: Permissions: home,media,audio,wayland,xorg,dbus_user,dbus_system,udev,usb,input,gpu,network,dev
Usage: fim-perms <add|del> <perms...>
  <add> : Allow one or more permissions
  <del> : Delete one or more permissions
  <perms...> : One or more permissions
Example: fim-perms add home,network,gpu
Usage: fim-perms <list|clear>
  <list> : Lists the current permissions
  <clear> : Clears all permissions
```

---

To allow the access to a resource, use the `add` subcommand:

```
$ ./app.flatimage fim-perms add home
```

This will make the `home` directory of the host accessible from the container.

---

To reset all permissions to a specific list of permissions, use `set`:

```
$ ./app.flatimage fim-perms set audio,wayland,xorg,network
```

This command blocks all previous permissions and only allow `audio,wayland,xorg,network`.

---

To list currently allowed permissions, use `list`:

```
$ ./app.flatimage fim-perms list
```

---

To delete a specific permission, use `del`:

```
$ ./app.flatimage fim-perms del home
```

This remove access to the host home directory.

## How it works

FlatImage uses [bubblewrap's](https://github.com/containers/bubblewrap)
bind mechanisms to make devices and directories available in the guest
container.
