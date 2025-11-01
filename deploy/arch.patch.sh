#!/usr/bin/env bash

# Pacman hooks overridden for flatimage builds (categorized by functionality):
#
# Systemd & Init System:
#     20-systemd-sysusers.hook - System user creation (causes "Failed to resolve user" errors)
#     30-systemd-binfmt.hook - Binary format registration
#     30-systemd-catalog.hook - Systemd catalog updates
#     30-systemd-daemon-reload-system.hook - System daemon reload
#     30-systemd-daemon-reload-user.hook - User daemon reload
#     30-systemd-hwdb.hook - Udev hardware database updates
#     30-systemd-udev-reload.hook - Device manager configuration reload
#     30-systemd-restart-marked.hook - Restarting marked services
#     30-systemd-sysctl.hook - Kernel sysctl settings
#     30-systemd-tmpfiles.hook - Temporary file creation
#     30-systemd-update.hook - Systemd unit updates
#     systemd-daemon-reload.hook - System manager configuration reload
#
# System Services:
#     dbus-reload.hook - D-Bus configuration reload
#
# System Maintenance:
#     60-depmod.hook - Kernel module dependency updates
#
# Custom Cleanup Hooks:
#     cleanup-doc.hook - Remove documentation files
#     cleanup-fonts.hook - Remove non-essential Noto fonts
#     cleanup-locale.hook - Remove non-English locale files
#     cleanup-man.hook - Remove man pages
#     cleanup-pkgs.hook - Remove downloaded package files

shopt -s nullglob

grep -rin -m1 -l "ConditionNeedsUpdate" /usr/lib/systemd/system | while read -r file; do
  echo "ConditionNeedsUpdate: $file"
  sed -Ei "s/ConditionNeedsUpdate=.*/ConditionNeedsUpdate=/" "$file"
done

mkdir -p /etc/pacman.d/hooks

# ============================================================================
# Systemd & Init System
# ============================================================================

# 20-systemd-sysusers.hook
tee /etc/pacman.d/hooks/20-systemd-sysusers.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/sysusers.d/*.conf

[Action]
Description = Overriding system user creation...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-binfmt.hook
tee /etc/pacman.d/hooks/30-systemd-binfmt.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/binfmt.d/*.conf
Target = etc/binfmt.d/*.conf

[Action]
Description = Overriding binary format registration...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-catalog.hook
tee /etc/pacman.d/hooks/30-systemd-catalog.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/systemd/catalog/*

[Action]
Description = Overriding systemd catalog update...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-daemon-reload-system.hook
tee /etc/pacman.d/hooks/30-systemd-daemon-reload-system.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/systemd/system/*

[Action]
Description = Overriding system daemon reload...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-daemon-reload-user.hook
tee /etc/pacman.d/hooks/30-systemd-daemon-reload-user.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/systemd/user/*

[Action]
Description = Overriding user daemon reload...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-hwdb.hook
tee /etc/pacman.d/hooks/30-systemd-hwdb.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/udev/hwdb.d/*

[Action]
Description = Overriding udev hardware database update...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-udev-reload.hook
tee /etc/pacman.d/hooks/30-systemd-udev-reload.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/udev/rules.d/*
Target = usr/lib/udev/hwdb.d/*
Target = etc/udev/rules.d/*
Target = etc/udev/hwdb.d/*

[Action]
Description = Overriding device manager configuration reload...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-restart-marked.hook
tee /etc/pacman.d/hooks/30-systemd-restart-marked.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/systemd/system/*

[Action]
Description = Overriding marked systemd unit restart...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-sysctl.hook
tee /etc/pacman.d/hooks/30-systemd-sysctl.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/sysctl.d/*.conf
Target = etc/sysctl.d/*.conf

[Action]
Description = Overriding kernel sysctl settings...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-tmpfiles.hook
tee /etc/pacman.d/hooks/30-systemd-tmpfiles.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/tmpfiles.d/*.conf

[Action]
Description = Overriding temporary file creation...
When = PostTransaction
Exec = /bin/true
END

# 30-systemd-update.hook
tee /etc/pacman.d/hooks/30-systemd-update.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/systemd/system/*

[Action]
Description = Overriding systemd unit update...
When = PostTransaction
Exec = /bin/true
END

# systemd-daemon-reload.hook
tee /etc/pacman.d/hooks/systemd-daemon-reload.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/systemd/system/*

[Action]
Description = Overriding system manager configuration reload...
When = PostTransaction
Exec = /bin/true
END

# ============================================================================
# System Services
# ============================================================================

# dbus-reload.hook
tee /etc/pacman.d/hooks/dbus-reload.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/share/dbus-1/system.d/*
Target = usr/share/dbus-1/services/*

[Action]
Description = Overriding D-Bus configuration reload...
When = PostTransaction
Exec = /bin/true
END

# ============================================================================
# System Maintenance
# ============================================================================

# 60-depmod.hook
tee /etc/pacman.d/hooks/60-depmod.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = usr/lib/modules/*/kernel/*

[Action]
Description = Overriding kernel module dependency update...
When = PostTransaction
Exec = /bin/true
END

# ============================================================================
# Custom Cleanup Hooks
# ============================================================================

# cleanup-doc.hook
tee /etc/pacman.d/hooks/cleanup-doc.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = *

[Action]
Description = Cleaning up doc...
When = PostTransaction
Exec = /bin/sh -c 'rm -rf /usr/share/doc/*'
END

# cleanup-fonts.hook
tee /etc/pacman.d/hooks/cleanup-fonts.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = *

[Action]
Description = Cleaning up noto fonts...
When = PostTransaction
Exec = /bin/sh -c 'find /usr/share/fonts/noto -mindepth 1 -type f -not -iname "notosans-*" -and -not -iname "notoserif-*" -exec rm "{}" \; 2>/dev/null || true'
END

# cleanup-locale.hook
tee /etc/pacman.d/hooks/cleanup-locale.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = *

[Action]
Description = Cleaning up locale files...
When = PostTransaction
Exec = /bin/sh -c 'find /usr/share/locale -mindepth 1 -maxdepth 1 -type d -not -iname "en_us" -exec rm -rf "{}" \;'
END

# cleanup-man.hook
tee /etc/pacman.d/hooks/cleanup-man.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = *

[Action]
Description = Cleaning up man...
When = PostTransaction
Exec = /bin/sh -c 'rm -rf /usr/share/man/*'
END

# cleanup-pkgs.hook
tee /etc/pacman.d/hooks/cleanup-pkgs.hook <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = *

[Action]
Description = Cleaning up downloaded files...
When = PostTransaction
Exec = /bin/sh -c 'rm -rf /var/cache/pacman/pkg/*'
END