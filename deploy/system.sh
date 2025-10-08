function _system_alpine()
{
  local dir_root="${1:?dist is undefined}"
  local dist="${2:?dist is undefined}"
  # Un-mount in case of a failure in a previous run
  umount "$dir_root/etc/resolv.conf" "$dir_root/etc/hosts" || true
  # Pull alpine
  rm -rf "$dir_root"
  wget http://dl-cdn.alpinelinux.org/alpine/v3.12/main/x86_64/apk-tools-static-2.10.8-r1.apk
  tar zxf apk-tools-static-*.apk
  ./sbin/apk.static --arch x86_64 -X http://dl-cdn.alpinelinux.org/alpine/latest-stable/main/ -U --allow-untrusted --root /tmp/"$dist" --initdb add alpine-base
  rm -rf "${dir_root:?dir_root is undefined}"/dev "$dir_root"/target
  # Touch sources
  { sed -E 's/^\s+://' | tee "$dir_root"/etc/apk/repositories; } <<-END
    :http://dl-cdn.alpinelinux.org/alpine/v3.16/main
    :http://dl-cdn.alpinelinux.org/alpine/v3.16/community
    :#http://dl-cdn.alpinelinux.org/alpine/edge/main
    :#http://dl-cdn.alpinelinux.org/alpine/edge/community
    :#http://dl-cdn.alpinelinux.org/alpine/edge/testing
	END
  # Include additional paths in PATH
  export PATH="$PATH:/sbin:/bin"
  # Remove mount dirs that may have leftover files
  rm -rf "${dir_root:?dir_root is undefined}"/{tmp,proc,sys,dev,run}
  # Create required mount point folders
  mkdir -p "$dir_root"/{tmp,proc,sys,dev,run/media,mnt,media,home}
  # Create required files for later binding
  rm -f "$dir_root"/etc/{host.conf,hosts,passwd,group,nsswitch.conf,resolv.conf}
  touch "$dir_root"/etc/{host.conf,hosts,passwd,group,nsswitch.conf,resolv.conf}
  # Update packages
  touch "$dir_root"/etc/resolv.conf "$dir_root"/etc/hosts
  mount --bind /etc/resolv.conf "$dir_root"/etc/resolv.conf
  mount --bind /etc/hosts "$dir_root"/etc/hosts
  chroot "$dir_root" /bin/sh -c 'apk update'
  chroot "$dir_root" /bin/sh -c 'apk upgrade'
  chroot "$dir_root" /bin/sh -c 'apk add bash alsa-utils alsa-utils-doc alsa-lib alsaconf alsa-ucm-conf pulseaudio pulseaudio-alsa' || true
  umount "$dir_root"/etc/resolv.conf "$dir_root"/etc/hosts
  # Create fim dir
  mkdir -p "$dir_root"/fim
  # Create config dir
  mkdir -p "$dir_root"/fim/config
}

function _system_arch()
{
  local dir_root="${1:?dist is undefined}"
  local dist="${2:?dist is undefined}"
  # Fetch bootstrap
  git clone "https://github.com/ruanformigoni/arch-bootstrap.git"
  # Build
  sed -Ei 's|^\s+curl|curl --retry 5|' ./arch-bootstrap/arch-bootstrap.sh
  sed 's/^/-- /' ./arch-bootstrap/arch-bootstrap.sh
  ./arch-bootstrap/arch-bootstrap.sh "$dir_root"
  # Update mirrorlist
  cp "$FIM_DIR/sources/arch.list" "$dir_root"/etc/pacman.d/mirrorlist
  # Enable multilib
  gawk -i inplace '/#\[multilib\]/,/#Include = (.*)/ { sub("#", ""); } 1' "$dir_root"/etc/pacman.conf
  chroot "$dir_root" /bin/bash -c "pacman -Sy"
  # Audio & video
  pkgs_va+=("alsa-lib lib32-alsa-lib alsa-plugins lib32-alsa-plugins libpulse")
  pkgs_va+=("lib32-libpulse alsa-tools alsa-utils")
  pkgs_va+=("pipewire lib32-pipewire pipewire-pulse")
  pkgs_va+=("pipewire-jack lib32-pipewire-jack pipewire-alsa")
  pkgs_va+=("wireplumber")
  chroot "$dir_root" /bin/bash -c "pacman -S ${pkgs_va[*]} --noconfirm"
  # Required for nvidia
  chroot "$dir_root" /bin/bash -c "pacman -S --noconfirm libvdpau lib32-libvdpau"
  # Avoid segfaults on some OpenGL applications
  chroot "$dir_root" /bin/bash -c "pacman -S --noconfirm libxkbcommon lib32-libxkbcommon"
  # Pacman hooks
  ## Avoid taking too long on 'Arming ConditionNeedsUpdate' and 'Update MIME database'
  cp "$FIM_DIR"/deploy/arch.patch.sh "$dir_root"/patch.sh
  chmod +x "$dir_root"/patch.sh
  chroot "$dir_root" /bin/bash -c /patch.sh
  rm "$dir_root"/patch.sh
  # Input
  chroot "$dir_root" /bin/bash -c "pacman -S libusb sdl2 lib32-sdl2 --noconfirm"
  # fakeroot & fakechroot
  chroot "$dir_root" /bin/bash -c "pacman -S git fakeroot fakechroot binutils --noconfirm"
  # Clear cache
  chroot "$dir_root" /bin/bash -c "pacman -Scc --noconfirm"
  rm -rf "$dir_root"/var/cache/pacman/pkg/*
  # Configure Locale
  sed -i 's/#en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' "$dir_root"/etc/locale.gen
  chroot "$dir_root" /bin/bash -c "locale-gen"
  echo "LANG=en_US.UTF-8" > "$dir_root"/etc/locale.conf
  # Create share symlink
  ln -sf /usr/share "$dir_root"/share
  # Create fim dir
  mkdir -p "$dir_root"/fim
  # Create config dir
  mkdir -p "$dir_root"/fim/config
  # Compile flatimage
  _docker_run "ARCH"
  # Remove mount dirs that may have leftover files
  rm -rf "${dir_root:?dir_root is undefined}"/{tmp,proc,sys,dev,run}
  # Create required mount points if not exists
  mkdir -p "$dir_root"/{tmp,proc,sys,dev,run/media,mnt,media,home}
  # Create required files for later binding
  rm -f "$dir_root"/etc/{host.conf,hosts,passwd,group,nsswitch.conf,resolv.conf}
  touch "$dir_root"/etc/{host.conf,hosts,passwd,group,nsswitch.conf,resolv.conf}
}