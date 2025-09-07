# Case insensitive filesystem

## What is it?

This command enables [ciopfs](https://www.brain-dump.org/projects/ciopfs) to use the filesystem in a case-insensitive manner.

## How to use

**Get the usage details** with `./app.flatimage fim-help casefold`:

```txt
fim-casefold : Enables casefold for the filesystem (ignore case)
Usage: fim-casefold <on|off>
   <on> : Enables casefold
   <off> : Disables casefold
```

**Case insensitive filesystem manipulation**

Case insensitivity only works with [unionfs-fuse](https://github.com/rpodgorny/unionfs-fuse) or [overlayfs-fuse](https://github.com/containers/fuse-overlayfs). It does not work with bwrap's native overlay option.

```bash
# Enable case folding
./app.flatimage fim-casefold on
# Create a test directory structure
./app.flatimage fim-exec mkdir -p /HelLo/WorLD
# Test if lower case path exists
./app.flatimage fim-exec ls -l /hello/world
total 0
```

**Temporary configuration**

It is possible to use an environment variable to enable case folding, this avoids changing the FlatImage binary.

```bash
# Enable case folding through an environment variable
export FIM_CASEFOLD=1
# Create a test directory structure
./app.flatimage fim-exec mkdir -p /HelLo/WorLD
# Test if lower case path exists
./app.flatimage fim-exec ls -l /hello/world
total 0
```

## How it works

The casefold configuration is written to the FlatImage binary. FlatImage checks binary section before starting, if enabled, the case insensitive filesystem is mounted to `$FIM_DIR_CONFIG/casefold`. This directory is then set as the root of [bubblewrap](https://github.com/containers/bubblewrap).