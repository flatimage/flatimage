# Commit current changes to the FlatImage

## What is it?

`fim-commit` is a command to integrate the current changes with the FlatImage.

## How to Use

**Get the usage details** with `./app.flatimage fim-help commit`:

```txt
fim-commit : Compresses current changes and inserts them into the FlatImage
Usage: fim-commit
```

**Compress changes in a novel filesystem**

This command compresses the current changes in a novel [dwarfs](https://github.com/mhx/dwarfs)
filesystem and appends it to the FlatImage. The files in the filesystem are still writeable
through the default enabled overlay filesystems used by flatimage.

```bash
# Allow network access
./app.flatimage fim-perms add network,wayland,xorg,audio
# Install firefox
./app.flatimage fim-root pacman -Syu
./app.flatimage fim-root pacman -S firefox
# Compress firefox and its dependencies into a new layer, and include it in the flatimage
./app.flatimage fim-commit
```

## How it Works

`fim-commit` compresses the modified and/or newly created files from the
`overlay filesystem` into a new layer; afterwards, it includes this layer in the
top of the layer stack of the current image.

<p align="center">
  <img src="https://raw.githubusercontent.com/flatimage/docs/master/docs/image/commit.png"/>
</p>
