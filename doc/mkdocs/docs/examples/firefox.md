# Create a Portable Firefox Installation

Here is the process to create a portable installation of firefox, that can be
stored in an external hard-drive to use across different computers without
hassle.

```
wget -O firefox.sh https://raw.githubusercontent.com/ruanformigoni/flatimage/refs/heads/master/examples/firefox.sh
chmod +x firefox.sh
./firefox.sh
```

This generates a `firefox.flatimage` package, which will store all the information in the
`.firefox.flatimage.config` hidden folder (~/.config, ~/.local, etc). You can
move this binary to an external hard-drive and use the same firefox installation
on different linux distributions without worrying about dependency/installation
issues, nothing is installed on the host, so your system remains clean.

Read the full script to create the firefox package [here](https://github.com/ruanformigoni/flatimage/blob/master/examples/firefox.sh).
