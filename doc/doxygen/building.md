@page building Building

FlatImage bundles several external tools to provide its core functionality. During compilation, these tools are embedded into the binary using C++23's `#embed` feature, ensuring a single source of truth for both the build process and source code. The tools are sourced from `https://github.com/flatimage/tools`, which acts as a curated proxy to prevent breaking changes and allow for behavioral patches when needed.

[TOC]

## Building FlatImage #{build_build}

Requires root permissions for sub-system setup with `chroot` and a docker setup.

```bash
sudo ./deploy/flatimage.sh arch      # Arch Linux
sudo ./deploy/flatimage.sh alpine    # Alpine Linux
sudo ./deploy/flatimage.sh blueprint # Blueprint
```

## Environment Variables {#build_vars}

CMake requires these variables to be set:

- `FIM_DIST` - Target distribution (ARCH, ALPINE, or BLUEPRINT)
- `FIM_RESERVED_SIZE` - Reserved configuration space (default: 4,194,304 bytes)
- `FIM_FILE_TOOLS` - Path to tools.json (embedded tool definitions)
- `FIM_FILE_META` - Path to meta.json (metadata configuration)

## Build Process Steps {#build_steps}

The build script performs the following stages:

1. **Fetch Binaries**: Downloads pre-compiled tools from `https://github.com/flatimage/tools/releases`
   - Includes: bash, busybox, bwrap, ciopfs, dwarfs, overlayfs, unionfs, ImageMagick
   - Compresses tools with UPX (except DwarFS)
   - Generates `tools.json` and metadata JSON files

2. **Docker Compilation**: Builds distribution-specific images
   - Compiles boot binary with embedded tools using `#embed`
   - Generates magic patcher utility and portal binaries
   - Produces janitor cleanup process

3. **Distribution Setup**: Creates filesystem roots (requires root)
   - **Alpine**: Initializes base system from Alpine Linux
   - **Arch**: Bootstraps Arch Linux with multilib support
   - **Blueprint**: Creates empty template
   - Both add required mount points, config directories, and MIME definitions

4. **DwarFS Compression**: Compresses the distribution root filesystem
   - Creates compressed layer 0 image with high compression ratio
   - Optimized for on-the-fly decompression

5. **ELF Assembly**: Concatenates components into single binary
   - Boot binary + embedded tools + compressed filesystem
   - Calculates reserved space offset and patches with `objcopy`
   - Appends reserved configuration space (default 4 MB)
   - Appends filesystem image with size metadata

6. **Distribution**: Generates final artifact
   - SHA256 checksum for verification
   - Placed in `dist/` directory

## Post-Build Output {#post_build}

The `dist/` directory contains:
- `{distribution}.flatimage`: Single-file FlatImage binary
- `{distribution}.flatimage.sha256sum`: Checksum for integrity verification