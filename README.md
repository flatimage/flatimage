<p align="center">
  <img src="https://raw.githubusercontent.com/flatimage/docs/master/docs/image/icon.png" width=150px/>
</p>

# What is FlatImage?

FlatImage packages your entire application—code, dependencies, and configuration—into a single executable that runs sandboxed on any Linux distribution. No installation, no external files, no compatibility issues.

## What Makes FlatImage Different?

🔒 **Sandboxed by Default**
Granular permissions (network, GPU, home, audio...). Default: zero access, fully isolated.

📦 **Self-Contained**
All config embedded in the ELF binary's reserved space.

⚡ **Fast & Compact**
DwarFS compression delivers high ratios with on-the-fly decompression.

✨ **Truly Portable**
Static linking + embedded tools. One file runs on any Linux distro without dependencies.

🔧 **Reconfigurable After Build**
Change permissions, environment, boot commands, and bindings post-creation without rebuilding.

🗂️ **Multiple Filesystem Backends**
Switch between OverlayFS (fast), UnionFS (compatible), BWRAP (native), or CIOPFS (case-insensitive) at runtime.

🎮 **Case-Insensitive Filesystem**
Windows-style case folding for Wine/Proton compatibility. Toggle with `fim-casefold on`.

🔌 **Portal IPC System**
Transparent host-guest communication. Execute host commands from containers with full I/O redirection.

🧱 **Layered Architecture**
Stack compressed layers with `fim-layer commit`. Copy-on-write, incremental builds, immutable base layers.

📦 **Package Recipes**
Install curated package sets with dependency resolution: `fim-recipe install gpu,audio,xorg`.

🔗 **Runtime Bind Mounts**
Map host paths dynamically without rebuilding: `fim-bind add rw '$HOME/Documents' /Documents`.

🚀 **Multi-Instance Support**
Run multiple isolated instances simultaneously. Execute in specific instances with `fim-instance`.

🖥️ **Desktop Integration**
Auto-generated menu entries, MIME types, and icons. Paths auto-update on file moves.

## Try It in 30 Seconds

```bash
# Download and run a complete Alpine Linux environment
wget https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
chmod +x alpine.flatimage
./alpine.flatimage  # You're now inside an isolated Alpine container!
```

# Documentation

📚 **User Documentation**: https://flatimage.github.io/docs

💻 **Developer Documentation**: https://flatimage.github.io/docs-developer

# Getting Started

## Download a distribution

Download one of the following distributions of a FlatImage package:

- [Alpine Linux](https://archlinux.org) is a complete `MUSL` subsystem with the `apk` package manager. [Download](https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage)

- [Arch Linux](https://archlinux.org) is a complete `GNU` subsystem with the `pacman` package manager. [Download](https://github.com/flatimage/flatimage/releases/latest/download/arch.flatimage)

- Blueprint is an empty FlatImage so you can build your own Linux sub-system. [Download](https://github.com/flatimage/flatimage/releases/latest/download/blueprint.flatimage)

## Enter the Container

The following commands download and enter the alpine container.

```bash
# Download the container
$ wget -O alpine.flatimage https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
# Make it executable
$ chmod +x ./alpine.flatimage
# Enter the container
$ ./alpine.flatimage
[flatimage-alpine] / > echo hello
hello
```
To exit the container, just press `CTRL+D`.

## Configure Permissions

By default, no permissions are set for the container. To allow one or more permissions use the `fim-perms` command.

```bash
$ ./alpine.flatimage fim-perms add xorg,wayland,network
```

The permissions `xorg` and `wayland` allow applications to create novel windows which appear in the host system. The `network` permission allows network access to applications inside the container.

## Execute Commands Once

To execute commands without entering the container, use `fim-exec` and `fim-root`. These commands bring up the container, execute your command, and bring down the container after the command is finished.

`fim-root` executes the command as the root user.

```bash
# Allow network access
$ ./alpine.flatimage fim-perms add xorg,wayland,network,audio
# Using 'fim-root' to install firefox in the alpine image
$ ./alpine.flatimage fim-root apk add firefox
```

`fim-exec` executes the command as a regular user.

```bash
# Using 'fim-exec' to run firefox as a regular user
$ ./alpine.flatimage fim-exec firefox font-noto
```

## Configure the Default Boot Command

`fim-boot` configures the default boot command, by default it is `bash`.

```bash
# Configure the boot command
$ ./alpine.flatimage fim-boot set firefox
# Opens firefox
$ ./alpine.flatimage
```

## Commit Changes

`fim-layer` compresses and saves your installed applications. FlatImage offers three flexible commit modes:

### Binary Mode - Self-Contained Distribution
Embed layers directly in the executable for maximum portability:

```bash
# Install Firefox
$ ./alpine.flatimage fim-root apk add firefox
# Commit to binary
$ ./alpine.flatimage fim-layer commit binary
# Rename and share as a single file
$ mv ./alpine.flatimage ./firefox.flatimage
```

**Perfect for:** Distributing portable apps, creating standalone executables

### Layer Mode - Modular Development
Save layers to a managed directory with auto-increment naming:

```bash
# Install development tools
$ ./alpine.flatimage fim-root apk add vim git gcc
# Save as layer-000.layer
$ ./alpine.flatimage fim-layer commit layer
# Install more tools
$ ./alpine.flatimage fim-root apk add nodejs npm
# Save as layer-001.layer
$ ./alpine.flatimage fim-layer commit layer
```

Layers are stored in `.alpine.flatimage.data/layers/` and are **automatically mounted** on every run.

**Perfect for:** Development workflows, organizing modular packages, testing configurations

### File Mode - Reusable Packages
Create shareable layer files with custom names:

```bash
# Build a GPU layer
$ ./alpine.flatimage fim-root apk add mesa vulkan-loader
$ ./alpine.flatimage fim-layer commit file ./shared/gpu-support.layer

# Share with others or use in multiple images
$ FIM_LAYERS=./shared/gpu-support.layer ./another-app.flatimage
```

**Perfect for:** Sharing layers between projects and version control.

## Case-Insensitive File System

`fim-casefold` enables filesystem case-insensitivity. **Linux** filesystems are **case-sensitive** by default. This means:

- `file.txt`, `File.txt`, and `FILE.txt` are treated as three completely different files
- You can have all three in the same directory simultaneously
- This applies to most common Linux filesystems like ext4, XFS, and Btrfs

**Example:**
```bash
$ touch readme.txt
$ touch README.txt
$ ls -1
readme.txt
README.txt
```

**Windows** filesystems (NTFS, FAT32) are **case-insensitive** but **case-preserving**. This means:
- `file.txt` and `File.txt` refer to the **same file**
- The system preserves the capitalization you used when creating the file
- You cannot have two files in the same directory that differ only by case
- Commands and file access treat names as case-insensitive.

To make FlatImage's filesystem case insensitive:
```bash
# Enable case-insensitivity
$ ./alpine.flatimage fim-casefold on
# Create create a README file.
$ ./alpine.flatimage fim-root touch /READme.md
# Try to access it as 'readme.md'
$ ./alpine.flatimage fim-root stat /readME.md
  File: /readME.md
  Size: 0               Blocks: 0          IO Block: 4096   regular empty file
Device: 61h/97d Inode: 26          Links: 1
Access: (0644/-rw-r--r--)  Uid: (    0/ UNKNOWN)   Gid: (    0/    root)
Access: 2025-10-11 19:14:21.106722076 +0000
Modify: 2025-10-11 19:14:21.106722076 +0000
Change: 2025-10-11 19:14:21.107484017 +0000
```

The current implementation is not **case-preserving**, the file names are always created in lower-case. Thus, if disabled with `fim-casefold off`, the file in the previous example will be accessible only as `readme.md` until casefolding is turned back on.

# Related and Similar Projects

- [RunImage](https://github.com/VHSgunzo/runimage): Portable single-file Linux container in unprivileged user namespaces.
- [AppBundle](https://github.com/xplshn/pelf): The .AppImage alternative designed for Linux, BSDs and more!
- [NixAppImage](https://github.com/pkgforge/nix-appimage): Nix-based AppImages.
- [Flatpak](https://flatpak.org/): Linux application sandboxing and distribution framework.
- [Conty](https://github.com/Kron4ek/Conty): Container-based portable apps.
- [binctr](https://github.com/genuinetools/binctr): Fully static, unprivileged, self-contained, containers as executable binaries.
- [nix-bundle](https://github.com/matthewbauer/nix-bundle): Bundle Nix derivations to run anywhere!
- [AppImage](https://appimage.org/): Linux apps that run anywhere.
- [exodus](https://github.com/Intoli/exodus): Painless relocation of Linux binaries–and all of their dependencies–without containers.
- [statifier](https://statifier.sourceforge.net/): Statifier is a tool for creating portable self-containing Linux executable.
- [bubblewrap](https://github.com/containers/bubblewrap): Low-level unprivileged sandboxing tool used by Flatpak and similar projects.
- [Proot](https://github.com/proot-me/proot): chroot, mount --bind, and binfmt_misc without privilege/setup for Linux.