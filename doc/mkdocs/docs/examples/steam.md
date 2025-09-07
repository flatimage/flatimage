# Create a Portable Steam Installation

Here is the process to create a portable installation of steam, that can be
stored in an external hard-drive to use across different computers without
hassle.

```
wget -O steam.sh https://raw.githubusercontent.com/ruanformigoni/flatimage/refs/heads/master/examples/steam.sh
chmod +x steam.sh
./steam.sh
```

This generates a `steam.flatimage` package, which will store all the information in the
`.steam.flatimage.config` hidden folder (installed games, save data, ~/.config,
~/.local, etc). You can move this binary to an external hard-drive and use the
same steam installation on different linux distributions without worrying about
dependency/installation issues, nothing is installed on the host, so your system
remains clean.

Read the full script to create the steam package [here](https://github.com/ruanformigoni/flatimage/blob/master/examples/steam.sh).
