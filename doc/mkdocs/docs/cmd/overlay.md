# Select the overlay filesystem

## What is it?

FlatImage allows writing over compressed read-only file systems with a read-write overlay.
This overlay uses either [fuse](https://man7.org/linux/man-pages/man4/fuse.4.html)
or [namespaces](https://man7.org/linux/man-pages/man7/namespaces.7.html) to create multiple
file system layers that are seen as one through a mount point.

## How to use

You can use `./app.flatimage fim-help bind` to get the following usage details:

```txt
fim-overlay : Show or select the default overlay filesystem
Usage: fim-overlay <set> <overlayfs|unionfs|bwrap>
  <set> : Sets the default overlay filesystem to use
  <overlayfs> : Uses 'fuse-overlayfs' as the overlay filesystem
  <unionfs> : Uses 'unionfs-fuse' as the overlay filesystem
  <bwrap> : Uses 'bubblewrap' native overlay options as the overlay filesystem
Usage: fim-overlay <show>
  <show> : Shows the current overlay filesystem
```

**Permanent Selection**

You can permanently select the default overlay file system through with the `fim-overlay set`
command, for example:

```bash
# For bwrap overlay options (default)
./app.flatimage fim-overlay set bwrap
# For unionfs-fuse
./app.flatimage fim-overlay set unionfs
# For fuse-overlayfs
./app.flatimage fim-overlay set overlayfs
```

**Temporary Selection**

The selection may also be temporary through the `FIM_OVERLAY` environment variable:

```bash
# For bwrap overlay options (default)
export FIM_OVERLAY=bwrap
# For unionfs-fuse
export FIM_OVERLAY=unionfs
# For fuse-overlayfs
export FIM_OVERLAY=overlayfs
```

## How it works

You selection changes the tool used to overlay the file systems on top of each other,
the tools used are:

* [bubblewrap](https://github.com/containers/bubblewrap/pull/547)
* [unionfs-fuse](https://github.com/rpodgorny/unionfs-fuse)
* [fuse-overlayfs](https://github.com/containers/fuse-overlayfs)

Each `fim-commit` command creates a novel layer, and layers are stacked from oldest to newest. That
way, novel changes are always on top, with the non-committed changes in the upper directory
always taking precedence.