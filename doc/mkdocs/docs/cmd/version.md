# Execute a command as a regular user

## What is it?

This command queries the version and build information of FlatImage and its dependencies.

## How to Use

You can use `./app.flatimage fim-help version` to get the following usage details:

```txt
fim-version : Displays version information of FlatImage
Usage: fim-version <short|full|deps>
  <fim-version> : Displays the version information of the FlatImage distribution
  <short> : Displays the version as a string
  <full> : Displays the version and build information in json
  <deps> : Displays dependencies metadata in json
```

You can use `short` to get the version of the FlatImage program:

```bash
$ ./app.flatimage fim-version short
v1.0.8
```

To query more build details use `full`:

```bash
$ ./app.flatimage fim-version full
{
  "COMMIT": "3508666",
  "DISTRIBUTION": "ALPINE",
  "TIMESTAMP": "20250505200900",
  "VERSION": "v1.0.8"
}
```

To query information about dependencies, use `deps`:
```bash
{
  "bash": {
    "licenses": [
      {
        "type": "GPL-3.0",
        "url": "https://www.gnu.org/licenses/gpl.html"
      }
    ],
    "sha": "78d8c15771695fe64a04b1ff1017057fa1d82e1f6a46506d5ba79e40ba8bc514",
    "source": "http://ftpmirror.gnu.org/gnu/bash/bash-5.3.tar.gz",
    "version": "5.3.0(1)-release"
  },
  ...
```

Use [jq](https://github.com/jqlang/jq) to select and print specific fields, for example:
```bash
$ ./app.flatimage fim-version deps | jq -r .bash.version
5.3.0(1)-release
```

## How it Works

During build time, the dockerfile of the respective distribution is given the `--build-arg` value of `FIM_DIST=DISTRIBUTION` and `FIM_METADATA_DEPS=json...`; the former defines the distribution name and
the latter defines binary dependency metadata used by the `interface.hpp` source file. The version is
retrieved in the `CMakeLists.txt` file with a git command to get the latest version tag. And lastly,
the timestamp is built with the current date and time.