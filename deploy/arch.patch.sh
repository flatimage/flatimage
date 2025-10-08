#!/usr/bin/env bash
shopt -s nullglob

grep -rin -m1 -l "ConditionNeedsUpdate" /usr/lib/systemd/system | while read -r file; do
  echo "ConditionNeedsUpdate: $file"
  sed -Ei "s/ConditionNeedsUpdate=.*/ConditionNeedsUpdate=/" "$file"
done

mkdir -p /etc/pacman.d/hooks

tee /etc/pacman.d/hooks/{update-desktop-database.hook,30-update-mime-database.hook} <<-"END"
[Trigger]
Type = Path
Operation = Install
Operation = Upgrade
Operation = Remove
Target = *

[Action]
Description = Overriding the desktop file MIME type cache...
When = PostTransaction
Exec = /bin/true
END

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
Exec = /bin/sh -c 'find /usr/share/fonts/noto -mindepth 1 -type f -not -iname "notosans-*" -and -not -iname "notoserif-*" -exec rm "{}" \;'
END