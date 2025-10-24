<p align="center">
  <img src="https://raw.githubusercontent.com/flatimage/docs/master/docs/image/icon.png" width=150px/>
</p>

# What is FlatImage?

FlatImage, is a hybrid of [Flatpak](https://github.com/flatpak/flatpak)
sandboxing with [AppImage](https://github.com/AppImage/AppImageKit) portability.

FlatImage use case is twofold:

* Flatimage is a package format, it includes a piece of software with all its
    dependencies for it work with across several linux distros (both Musl and
    GNU). Unlike `AppImage`, FlatImage runs the application in a container by
    default, which increases portability and compatibility at the cost of file
    size.

* Flatimage is a portable container image that requires no superuser permissions to run.

The diverse `GNU/Linux` ecosystem includes a vast array of distributions, each
with its own advantages and use cases. This can lead to cross-distribution
software compatibility challenges. FlatImage addresses these issues by:

* Utilizing its own root directory, enabling dynamic libraries with hard-coded
    paths to be packaged alongside the software without
    [binary patching](https://github.com/AppImage/AppImageKit/wiki/Bundling-Windows-applications).
* Running the application in its own gnu (or musl) environment, therefore, not using host
    libraries that might be outdated/incompatiblesystem with the application.

It simplifies the task of software packaging by enforcing the philosophy that it
should be as simple as setting up a container. This is an effort for the
end-user to not depend on the application developer to provide the portable
binary (or to handle how to package the application, dependencies and create a
runner script). It also increases the quality of life of the package developer,
simplifying the packaging process of applications.

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

By default, no permissions are set for the container. To allow one or more permissions use the [fim-perms](./cmd/perms.md) command.

```bash
$ ./alpine.flatimage fim-perms add xorg,wayland,network
```

The permissions `xorg` and `wayland` allow applications to create novel windows which appear in the host system. The `network` permission allows network access to applications inside the container.

## Execute Commands Once

To execute commands without entering the container, use `fim-exec` and `fim-root`. These commands bring up the container, execute your command, and bring down the container after the command is finished.

[fim-root](./cmd/root.md) executes the command as the root user.

```bash
# Allow network access
$ ./alpine.flatimage fim-perms add xorg,wayland,network,audio
# Using 'fim-root' to install firefox in the alpine image
$ ./alpine.flatimage fim-root apk add firefox
```

[fim-exec](./cmd/exec.md) executes the command as a regular user.

```bash
# Using 'fim-exec' to run firefox as a regular user
$ ./alpine.flatimage fim-exec firefox font-noto
```

## Configure the Default Boot Command

[fim-boot](./cmd/boot.md) configures the default boot command, by default it is `bash`.

```bash
# Configure the boot command
$ ./alpine.flatimage fim-boot set firefox
# Opens firefox
$ ./alpine.flatimage
```

## Commit Changes

[fim-commit](./cmd/commit.md) is used compress and save the installed applications to inside the image.

```bash
# Commit changes
$ ./alpine.flatimage fim-commit
# Rename application
$ mv ./alpine.flatimage ./firefox.flatimage
```

## Case-Insensitive File System

[fim-casefold](./cmd/casefold.md) enables filesystem case-insensitivity. **Linux** filesystems are **case-sensitive** by default. This means:

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

# Motivations

1. The idea of this application sprung with the challenge to package software
   and dynamic libraries, such as `wine`, when there are hard-coded paths. The
   best solution is invasive
   [https://github.com/AppImage/AppImageKit/wiki/Bundling-Windows-applications](https://github.com/AppImage/AppImageKit/wiki/Bundling-Windows-applications)
   , which patches the binaries of wine directly to use a custom path for the
   32-bit libraries (an implementation of this concept is available
   [here](https://github.com/ruanformigoni/wine)), not only that, it requires to
   patch the `32-bit` pre-loader `ld-linux.so` as well, however, sometimes it
   still fails to execute properly. This is an over exceeding complexity for the
   end-user, which should package applications with no effort; `FlatImage`
   changes the root filesystem the application runs in, to a minimal gnu
   subsystem, and with that, it solves the previous issues with dynamic
   libraries no workarounds required. No host libraries are used, which
   decreases issues of portable applications working on one machine and not in
   other.

1. The fragmentation of the linux package management is considerable in modern
   times, e.g., `apt`, `pip`, `npm`, and more. To mitigate this issue
   `FlatImage` can perform the installation through the preferred package
   manager, and turn the program into an executable file, that can run in any
   linux distribution. E.g.: The user of `FlatImage` can create a binary of
   `youtube-dl`, from the `pip` package manager, without having either pip or
   python installed on the host operating system.

1. Some applications are offered as pre-compiled compressed tar files
   (tarballs), which sometimes only work when installed on the root of the
   operating system. However, doing so could hinder the operating system
   integrity, to avoid this issue `FlatImage` can install tarballs into itself
   and turn them into a portable binary.


# Related and Similar Projects

- [AppBundle](https://github.com/xplshn/pelf)
- [AppImage](https://appimage.org/)
- [NixAppImage](https://github.com/pkgforge/nix-appimage)
- [RunImage](https://github.com/VHSgunzo/runimage)
- [Flatpak](https://flatpak.org/)
- [Conty](https://github.com/Kron4ek/Conty)
- [binctr](https://github.com/genuinetools/binctr)
- [exodus](https://github.com/Intoli/exodus)
- [statifier](https://statifier.sourceforge.net/)
- [nix-bundle](https://github.com/matthewbauer/nix-bundle)
- [bubblewrap](https://github.com/containers/bubblewrap)
- [Proot](https://github.com/proot-me/proot)