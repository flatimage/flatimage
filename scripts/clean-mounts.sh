#!/usr/bin/env bash

mount | pcregrep -o1 "overlayfs on (.*) type" | xargs -I{} fusermount -u {}
mount | pcregrep -o1 "dwarfs on (.*) type" | xargs -I{} fusermount -u {}
mount | pcregrep -o1 ".* on (.*) type fuse.ext4" | xargs -I{} fusermount -u {}

