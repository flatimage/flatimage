# Execute a command as a regular user

## What is it?

`fim-version` or `fim-version-full` queries the current version of FlatImage.

## How to Use

You can use `./app.flatimage fim-help versoin` to get the following usage details:

```txt
fim-version : Displays version information of FlatImage
Usage: fim-version
Usage: fim-version-full
   <fim-version> : Displays the version information of the FlatImage distribution
   <fim-version-full> : Displays a detailed json output of the FlatImage distribution
```

You can use `./app.flatimage fim-version` to get the version of the FlatImage file:

```bash
$ ./app.flatimage fim-version
v1.0.8
```

To query more build details:

```bash
$ ./app.flatimage fim-version
{
  "COMMIT": "3508666",
  "DISTRIBUTION": "ALPINE",
  "TIMESTAMP": "20250505200900",
  "VERSION": "v1.0.8"
}
```

The output is given in the Json format.

## How it Works

During build time, the dockerfile of the respective distribution is given the `--build-arg` value of `FIM_DIST=DISTRIBUTION`, where the distribution can be either alpine, arch, or blueprint. The version is retrieved in the `CMakeLists.txt` file with a git command to get the latest tag. And lastly, the timestamp is built with the current date and time, in the format `--build-arg FIM_DIST=BLUEPRINT`.
