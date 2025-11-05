#!/usr/bin/env bash

######################################################################
# @author      : Ruan E. Formigoni (ruanformigoni@gmail.com)
# @file        : _build
# @created     : Monday Jan 23, 2023 21:18:26 -03
#
# @description : Tools to build the filesystem
######################################################################

#shellcheck disable=2155

set -xe

shopt -s nullglob extglob

FIM_DIR_SCRIPT=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
FIM_DIR="$(dirname "$FIM_DIR_SCRIPT")"
FIM_DIR_BUILD="$FIM_DIR"/build
# 4MB of reserved space
FIM_RESERVED_SIZE="$(echo "4 * (2^20)" | bc)"
BINARIES=(
  fim_portal
  fim_portal_daemon
  fim_bwrap_apparmor
  fim_janitor
  bash
  busybox
  bwrap
  ciopfs
  dwarfs_aio
  overlayfs
  unionfs
  magick
)
FIM_FILE_TOOLS="/flatimage/build/tools.json"
FIM_FILE_META="/flatimage/build/meta.json"

# shellcheck source=/dev/null
source "${FIM_DIR_SCRIPT}/system.sh"

function _fetch_binaries()
{
  mkdir -p bin meta

  # Fetch tools
  for binary in "${BINARIES[@]}"; do
    [[ ! -f ./bin/"$binary" ]] || continue
    [[ ! "$binary" =~ fim_ ]] || continue
    wget -nc -O ./bin/"$binary" "https://github.com/flatimage/tools/releases/download/7c31ed8/$binary-x86_64"
    wget -nc -O ./meta/"$binary".json "https://github.com/flatimage/tools/releases/download/7c31ed8/$binary-metadata.json"
    chmod 755 ./bin/"$binary"
  done

  # Dwarfs symlinks
  ln -s dwarfs_aio bin/mkdwarfs
  ln -s dwarfs_aio bin/dwarfs

  jo bash="$(< ./meta/bash.json)" \
    busybox="$(< ./meta/busybox.json)" \
    bwrap="$(< ./meta/bwrap.json)" \
    ciopfs="$(< ./meta/ciopfs.json)" \
    dwarfs_aio="$(< ./meta/dwarfs_aio.json)" \
    overlayfs="$(< ./meta/overlayfs.json)" \
    unionfs="$(< ./meta/unionfs.json)" \
    magick="$(< ./meta/magick.json)" \
    | tr -d '\n' > "$FIM_DIR_BUILD/meta.json"

  # Tools to include in the source with #embed
  jo -a "${BINARIES[@]}" > "$FIM_DIR_BUILD"/tools.json

  # # Setup xdg scripts
  # cp "$FIM_DIR"/src/xdg/xdg-* ./bin

  # Compress binaries
  upx -6 --no-lzma ./bin/!(*dwarfs*)
}

# Concatenates binary files and filesystem to create fim image
# $1 = Path to system image
# $2 = Output file name
function _create_elf()
{
  local img="$1"
  local out="$2"

  # Boot is the program on top of the image
  cp bin/boot "$out"
  size_boot=$(du -b "$out" | awk '{print $1}')
  # Patch offset size to filesystems
  FIM_RESERVED_OFFSET=$((FIM_RESERVED_OFFSET + size_boot))
  for binary in "${BINARIES[@]}"; do
    size_binary="$(du -b "bin/$binary" | awk '{print $1}')"
    FIM_RESERVED_OFFSET=$((FIM_RESERVED_OFFSET + size_binary + 8))
  done
  echo "FIM_RESERVED_OFFSET: $FIM_RESERVED_OFFSET"
  objcopy --update-section .fim_reserved_offset=<( perl -e 'print pack("L<", shift)' "$FIM_RESERVED_OFFSET" ) "$out"
  # Patch magic bytes
  "$FIM_DIR_BUILD"/magic "$out"
  # Append binaries
  for binary in "${BINARIES[@]}"; do
    size_binary="$(du -b "bin/$binary" | awk '{print $1}')"
    size_hex_binary="$(echo "$size_binary" | xargs -I{} printf "%016x\n" {})"
    # Write binary size
    # uint64_t = 8 bytes
    for byte_index in $(seq 0 7 | sort -r); do
      local byte="${size_hex_binary:$(( byte_index * 2)):2}"
      echo -ne "\\x$byte" >> "$out"
    done
    cat "bin/$binary" >> "$out"
  done
  # Create reserved space
  dd if=/dev/zero of="$out" bs=1 count="$FIM_RESERVED_SIZE" oflag=append conv=notrunc
  # Write size of image rightafter
  size_img="$( du -b "$img" | awk '{print $1}' | xargs -I{} printf "%016x\n" {} )"
  for byte_index in $(seq 0 7 | sort -r); do
    local byte="${size_img:$(( byte_index * 2)):2}"
    echo -ne "\\x$byte" >> "$out"
  done
  # Write image
  cat "$img" >> "$out"

}

function _docker_run()
{
  local fim_dist="${1:?fim_dist parameter missing}"
  # Compile flatimage
  (
    cd "$FIM_DIR"
    docker build . \
      --build-arg FIM_RESERVED_SIZE="$FIM_RESERVED_SIZE" \
      --build-arg FIM_FILE_TOOLS="$FIM_FILE_TOOLS" \
      --build-arg FIM_FILE_META="$FIM_FILE_META" \
      --build-arg FIM_DIST="$fim_dist" \
      -t flatimage-boot \
      -f docker/Dockerfile.boot
    docker run --rm -v "$FIM_DIR_BUILD":"/host" flatimage-boot cp /flatimage/build/src/boot /host/bin
    docker run --rm -v "$FIM_DIR_BUILD":"/host" flatimage-boot cp /flatimage/build/src/magic /host/magic
    docker run --rm -v "$FIM_DIR_BUILD":"/host" flatimage-boot cp /flatimage/src/janitor/fim_janitor /host/bin
  )
  # Compile and include portal
  (
    cd "$FIM_DIR"
    docker build . -t "flatimage-portal" -f docker/Dockerfile.portal
    docker run --rm -v "$FIM_DIR_BUILD":/host "flatimage-portal" cp /fim/dist/fim_portal /fim/dist/fim_portal_daemon /host/bin
  )
  # Compile and include bwrap-apparmor
  (
    cd "$FIM_DIR"
    docker build . -t flatimage-bwrap-apparmor -f docker/Dockerfile.bwrap_apparmor
    docker run --rm -v "$FIM_DIR_BUILD":/host "flatimage-bwrap-apparmor" cp /fim/dist/fim_bwrap_apparmor /host/bin
  )
}

function _package()
{
  local dir_root="${1:?dir_root is undefined}"
  local dir_dist="${2:?dir_dist is undefined}"
  local dist="${3:?dist is undefined}"
  # Create distribution directory
  mkdir -p "$dir_dist"
  # Set permissions to non-root
  chown -R 1000:1000 "$dir_root"
  # MIME
  mkdir -p "$dir_root/fim/desktop"
  cp "$FIM_DIR"/mime/icon.svg      "$dir_root/fim/desktop"
  cp "$FIM_DIR"/mime/flatimage.xml "$dir_root/fim/desktop"
  # Create layer 0 compressed filesystem
  ./bin/mkdwarfs -i "$dir_root" -o "$dist".layer
  # Create elf
  _create_elf "$dist".layer "$dist".flatimage
  # Create sha256sum
  sha256sum "$dist.flatimage" > "$dir_dist/$dist.flatimage.sha256sum"
  mv "$dist.flatimage" "$dir_dist"
}

# Creates an blueprint subsystem
function _create_subsystem_blueprint()
{
  local dist="blueprint"
  # Fetch static binaries, creates a bin folder
  _fetch_binaries
  # Create fim and config dirs
  mkdir -p /tmp/"$dist"/fim/config
  # Compile flatimage
  _docker_run "BLUEPRINT"
  # Package
  _package /tmp/"$dist" ./dist "$dist"
}

# Creates an alpine subsystem
# Requires root permissions
function _create_subsystem_alpine()
{
  local dist="alpine"
  # Fetch static binaries, creates a bin folder
  _fetch_binaries
  # Pull alpine
  _system_alpine /tmp/"$dist" "$dist"
  # Compile flatimage
  _docker_run "ALPINE"
  # Package
  _package /tmp/"$dist" ./dist "$dist"
}

# Creates an arch subsystem
# Requires root permissions
function _create_subsystem_arch()
{
  local dist="arch"
  # Fetch static binaries, creates a bin folder
  _fetch_binaries
  # Pull arch linux
  _system_arch /tmp/"$dist" "$dist"
  # Compile flatimage
  _docker_run "ARCH"
  # Package
  _package /tmp/"$dist" ./dist "$dist"
}

function main()
{
  rm -rf "$FIM_DIR_BUILD"
  mkdir "$FIM_DIR_BUILD"
  cd "$FIM_DIR_BUILD"

  case "$1" in
    "arch") _create_subsystem_arch ;;
    "alpine") _create_subsystem_alpine ;;
    "blueprint") _create_subsystem_blueprint ;;
    *) echo "Invalid option $2" >&2 ;;
  esac
}

main "$@"