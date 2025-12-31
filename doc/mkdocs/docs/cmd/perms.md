# Configure Permissions

## What is it?

The `fim-perms` command provides granular control over which host system resources are accessible to applications running inside the FlatImage container. By default, FlatImage runs in a sandboxed environment with no access to host resources.

The permission system allows you to:

- **Grant only necessary access** - Applications get exactly what they need, nothing more
- **Enable functionality** - GUI apps need display access, network apps need connectivity
- **Customize isolation** - Different applications need different resource access

## Available Permissions

### all

**Special Permission**: Grants all available permissions at once.

**Behavior:**

- Enables all individual permissions simultaneously
- Cannot be combined with other permissions
- Provides complete host system access within sandboxing constraints

**Example:**

```bash
# Grant all permissions at once
./app.flatimage fim-perms add all
# Or use set
./app.flatimage fim-perms set all
```

---

### home
Access to the host `$HOME` directory.

**Binds:**

- `[rw]` `$HOME` → `$HOME`

**When to use:**

- Applications that work with user documents
- Text editors, office applications
- File managers
- Media players accessing user libraries
- Development tools needing project access

**Example:**

```bash
./app.flatimage fim-perms add home
./app.flatimage fim-exec ls ~/Documents
```

---

### media

Access to external storage devices and mount points.

**Binds:**

- `[rw]` `/media` → `/media`
- `[rw]` `/run/media` → `/run/media`
- `[rw]` `/mnt` → `/mnt`

**When to use:**

- Applications that work with external drives
- Backup utilities
- Media management software
- File transfer applications
- Data recovery tools

**Example:**

```bash
./app.flatimage fim-perms add media
./app.flatimage fim-exec ls /media/usb-drive
```

---

### audio

Access to audio subsystem for sound input/output.

**Binds:**

- `[rw]` `$XDG_RUNTIME_DIR/pulse/native` → `$XDG_RUNTIME_DIR/pulse/native`
- `[rw]` `$XDG_RUNTIME_DIR/pipewire-0` → `$XDG_RUNTIME_DIR/pipewire-0`

**When to use:**

- Media players and music applications
- Video conferencing software
- Games with sound
- Audio editing tools
- Screen recording with audio

**Example:**

```bash
./app.flatimage fim-perms add audio
./app.flatimage fim-exec mpv music.mp3
```

---

### wayland

Access to Wayland display server.

**Binds:**

- `[rw]` `$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY` → `$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY`

**Requirements:**

- `WAYLAND_DISPLAY` environment variable must be set on host

**When to use:**

- GUI applications on Wayland desktop
- Modern desktop environments (GNOME, KDE Plasma 6+)
- Applications that create windows
- Graphics applications

**Example:**

```bash
./app.flatimage fim-perms add wayland
./app.flatimage fim-exec firefox
```

---

### xorg
Access to X11 display server.

**Binds:**

- `[ro]` `$XAUTHORITY` → `$XAUTHORITY`

**Requirements:**

- `XAUTHORITY` and `DISPLAY` environment variables must be set on host

**When to use:**

- GUI applications on X11 desktop
- Legacy desktop environments
- Applications requiring X11 features
- Remote X11 sessions

**Example:**
```bash
./app.flatimage fim-perms add xorg
./app.flatimage fim-exec gimp
```

---

### dbus_user

Access to D-Bus session bus for inter-application communication.

**Binds:**

- `[rw]` `$DBUS_SESSION_BUS_ADDRESS` → `$DBUS_SESSION_BUS_ADDRESS`

**Requirements:**

- `DBUS_SESSION_BUS_ADDRESS` environment variable must be set on host

**When to use:**

- Applications that communicate with other apps
- Desktop integration features
- Firefox (for multiple instance communication)
- Notification support
- Desktop environment integration

**Example:**

```bash
./app.flatimage fim-perms add dbus_user
./app.flatimage fim-exec firefox
```

---

### dbus_system

Access to D-Bus system bus for system-level communication.

**Binds:**

- `[rw]` `/run/dbus/system_bus_socket` → `/run/dbus/system_bus_socket`

**When to use:**

- System monitoring tools
- Power management applications
- Network management utilities
- Hardware control applications

**Example:**

```bash
./app.flatimage fim-perms add dbus_system
./app.flatimage fim-exec system-monitor
```

---

### udev

Monitor device events and detect hardware changes.

**Binds:**

- `[rw]` `/run/udev` → `/run/udev`

**When to use:**

- Hardware detection applications
- Device management tools
- Applications that respond to device insertion/removal
- System utilities

**Example:**

```bash
./app.flatimage fim-perms add udev
./app.flatimage fim-exec hardware-manager
```

---

### usb

Access to USB devices.

**Binds:**

- `[dev]` `/dev/usb` → `/dev/usb`
- `[dev]` `/dev/bus/usb` → `/dev/bus/usb`

**When to use:**

- USB device communication
- Hardware programming tools
- Device firmware updates
- USB peripheral access

**Example:**

```bash
./app.flatimage fim-perms add usb
./app.flatimage fim-exec usb-tool
```

---

### input

Access to input devices (keyboard, mouse, joysticks, etc.).

**Binds:**

- `[dev]` `/dev/input` → `/dev/input`
- `[dev]` `/dev/uinput` → `/dev/uinput`

**When to use:**

- Games requiring gamepad input
- Input device configuration tools
- Emulators
- Accessibility applications

**Example:**

```bash
./app.flatimage fim-perms add input
./app.flatimage fim-exec game
```

---

### gpu

Access to GPU hardware acceleration.

**Binds:**

- `[dev]` `/dev/dri` → `/dev/dri`
- Symlinks NVIDIA drivers from host to container

**When to use:**

- Games requiring 3D acceleration
- Video editing applications
- 3D modeling software
- GPU-accelerated computation
- Hardware-accelerated video playback

**Example:**

```bash
./app.flatimage fim-perms add gpu
./app.flatimage fim-exec blender
```

---

### network

Configure network access for internet connectivity.

**Binds:**

- `[ro]` `/etc/host.conf` → `/etc/host.conf`
- `[ro]` `/etc/hosts` → `/etc/hosts`
- `[ro]` `/etc/nsswitch.conf` → `/etc/nsswitch.conf`
- `[ro]` `/etc/resolv.conf` → `/etc/resolv.conf`

**When to use:**

- Web browsers
- Download managers
- Network utilities
- Online games
- Update mechanisms
- Any application requiring internet

**Example:**

```bash
./app.flatimage fim-perms add network
./app.flatimage fim-exec curl https://example.com
```

---

### shm

Access to POSIX shared memory.

**Binds:**

- `[dev]` `/dev/shm` → `/dev/shm`

**When to use:**

- Applications using shared memory IPC
- Performance-critical applications
- Multi-process applications
- Some database systems

**Example:**

```bash
./app.flatimage fim-perms add shm
./app.flatimage fim-exec database-server
```

---

### optical

Access to optical drives (CD/DVD/Blu-ray).

**Binds:**

- `[dev]` `/dev/sr[0-255]` → `/dev/sr[0-255]`
- `[dev]` `/dev/sg[0-255]` → `/dev/sg[0-255]`

**When to use:**

- CD/DVD burning applications
- Media ripping tools
- Disc backup software
- Disc verification utilities

**Example:**

```bash
./app.flatimage fim-perms add optical
./app.flatimage fim-exec k3b
```

---

### dev

Access to all devices in `/dev` directory.

**Binds:**

- `[dev]` `/dev` → `/dev`

**When to use:**

- System administration tools
- Hardware diagnostic utilities
- Low-level device access
- When specific device permissions are insufficient.

**Example:**

```bash
./app.flatimage fim-perms add dev
./app.flatimage fim-exec hardware-diagnostic
```

---

**Environment Variable Defaults:**

If `XDG_RUNTIME_DIR` is undefined, it defaults to `/run/user/$(id -u)`.

**Bind Type Legend:**

- `[ro]` - Read-only: Container can read but not modify
- `[rw]` - Read-write: Container can read and modify
- `[dev]` - Device: Special device node binding

## How to Use

You can use `./app.flatimage fim-help perms` to get the following usage details:

```txt
fim-perms : Edit current permissions for the flatimage
Note: Permissions: all,audio,dbus_system,dbus_user,dev,gpu,home,input,media,network,optical,shm,udev,usb,wayland,xorg
Usage: fim-perms <add|del|set> <perms...>
  <add> : Allow one or more permissions
  <del> : Delete one or more permissions
  <set> : Replace all permissions with the specified set
  <perms...> : One or more permissions
Example: fim-perms add home,network,gpu
Example: fim-perms set wayland,audio,network
Note: The 'all' permission sets all available permissions and cannot be combined with other permissions
Usage: fim-perms <list|clear>
  <list> : Lists the current permissions
  <clear> : Clears all permissions
```

### Add Permissions

Grant one or more permissions to the container:

```bash
# Add single permission
./app.flatimage fim-perms add home

# Add multiple permissions
./app.flatimage fim-perms add home,network,audio
```

Existing permissions are preserved; new ones are added to the list.

### Set Permissions

Replace all permissions with a specific set:

```bash
# Reset to only these permissions
./app.flatimage fim-perms set wayland,audio,network
```

All previous permissions are removed; only the specified ones remain.

**Difference between add and set:**

```bash
# Current: home,network
./app.flatimage fim-perms add audio
# Result: home,network,audio

# Current: home,network,audio
./app.flatimage fim-perms set wayland,gpu
# Result: wayland,gpu (home,network,audio removed)
```

### List Permissions

Display currently configured permissions:

```bash
./app.flatimage fim-perms list
```

**Example output:**
```
home
network
audio
wayland
```

### Remove Permissions

Delete one or more specific permissions:

```bash
# Remove single permission
./app.flatimage fim-perms del home

# Remove multiple permissions
./app.flatimage fim-perms del home,network
```

Other permissions remain unchanged.

### Clear All Permissions

Remove all permissions, returning to fully isolated state:

```bash
./app.flatimage fim-perms clear
```

After clearing, the container has no access to host resources.

## How it Works

FlatImage uses [bubblewrap's](https://github.com/containers/bubblewrap) bind mount mechanism to selectively expose host resources to the container. Each permission translates to one or more bind mounts or device bindings.