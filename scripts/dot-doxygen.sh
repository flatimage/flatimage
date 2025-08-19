#!/bin/sh

exec "$(command -v dot)" -Gsplines=ortho -Gconcentrate=true  -Gnodesep=0.5 -Granksep=0.6 "$@"