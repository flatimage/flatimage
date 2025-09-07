# Manager FlatImage layers

## What is it?

`fim-layer` is a command to manage the layers of a FlatImage.

## How to Use

You can use `./app.flatimage fim-help layer` to get the following usage details:

```txt
fim-layer : Manage the layers of the current FlatImage
Usage: fim-layer <create> <in-dir> <out-file>
   <create> : Creates a novel layer from <in-dir> and save in <out-file>
   <in-dir> : Input directory to create a novel layer from
   <out-file> : Output file name of the layer file
Usage: fim-layer <add> <in-file>
   <add> : Includes the novel layer <in-file> in the image in the top of the layer stack
   <in-file> : Path to the layer file to include in the FlatImage
```

In this example, lets create a new layer that includes an executable to print
`hello world` on the terminal. The first step is to create the directory
structure for the layer, e.g.:

```bash
$ mkdir -p ./root/usr/bin
```

The root directory will overlay the root directory of the FlatImage, now lets
create our script and make it executable:

```bash
# Use bash to run the script
$ echo '#!/bin/bash' > ./root/usr/bin/hello-world.sh
# Print hello world with the script
$ echo 'echo "hello world"' >> ./root/usr/bin/hello-world.sh
# Make the script executable
$ chmod +x ./root/usr/bin/hello-world.sh
```

Now with the root directory configured, we can create the novel layer with:

```bash
# This command creates a layer from the 'root' folder and saves
# it to a file names 'layer'
$ ./app.flatimage fim-layer create ./root ./layer
```

The last step is to include the created layer in the FlatImage:

```bash
$ ./app.flatimage fim-layer add ./layer
```

Now we can test if the process succeeded with:

```bash
$ ./app.flatimage fim-exec hello-world.sh
hello world
$ ./app.flatimage fim-exec command -v hello-world.sh
/usr/bin/hello-world.sh
```


## How it Works

The `create` and `add` commands  demonstrated above are similar to the
`fim-commit` command, but allow the user to specify a specific directory to
create the novel layer from.

<p align="center">
  <img src="https://raw.githubusercontent.com/flatimage/docs/master/docs/image/commit.png"/>
</p>
