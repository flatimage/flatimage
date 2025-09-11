# Environment Variables

## What is it?

This facility is used to define environment variables that are defined in the container.

## How to use

You can use `./app.flatimage fim-help env` to get the following usage details:

```
fim-env : Define environment variables in FlatImage
Usage: fim-env <add|set> <'key=value'...>
  <add> : Include a novel environment variable
  <set> : Redefines the environment variables as the input arguments
  <'key=value'...> : List of variables to add or set
Example: fim-env add 'APP_NAME=hello-world' 'HOME=/home/my-app'
Usage: fim-env <clear>
  <clear> : Clears configured environment variables
Usage: fim-env <list>
  <list> : Lists configured environment variables
```

The `env` command allows you to set new environment variables as so:

```bash
$ ./app.flatimage fim-env add 'MY_NAME=user' 'MY_STATE=sad'
```

To use these variables in the default boot command:

```bash
$ ./app.flatimage fim-exec sh -c 'echo "My name is $MY_NAME and I am $MY_STATE"'
My name is user and I am sad
```

To list the set variables:
```bash
$ ./app.flatimage fim-env list
MY_NAME=user
MY_STATE=sad
```

To delete the set variables:
```bash
$ ./app.flatimage fim-env del MY_NAME MY_STATE
```

## How it works

The defined environment variables are passed to [bubblewrap](https://github.com/containers/bubblewrap).

