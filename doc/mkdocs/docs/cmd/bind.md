# Bind Host Paths

## What is it?

This is a mechanism to make specific paths from the host available inside the guest container.

## How to use

You can use `./app.flatimage fim-help bind` to get the following usage details:

```txt
Usage: fim-bind <add> <type> <src> <dst>
  <add> : Create a novel binding of type <type> from <src> to <dst>
  <type> : ro, rw, dev
  <src> : A file, directory, or device
  <dst> : A file, directory, or device
Usage: fim-bind <del> <index>
  <del> : Deletes a binding with the specified index
  <index> : Index of the binding to erase
Usage: fim-bind <list>
  <list> : Lists current bindings
```

---

To bind a device from the host to use in the guest:
```
$ ./app.flatimage fim-bind add dev /dev/my_device /dev/my_device
```
This will make `my_device` available inside the container.

---

To bind a file path from the host to the guest as read-only:
```
$ ./app.flatimage fim-bind add ro '$HOME/Music' /Music
```
This will make the `Music` directory available inside the container in `/Music`,
as read-only. Notice that `'$HOME/Music'` is inside single quotes, this means
that the `$HOME` variable will only expand when flatimage is executed; i.e.,
when another user executes the `app`, their `$HOME/Music` directory will be the
one bound to the container.

---

To bind a file path from the host to the guest as read-write:
```
$ ./app.flatimage fim-bind add rw '$HOME/Music' /Music
```
This will make the `Music` directory available inside the container in `/Music`,
as read-write.

---

You can list the current bindings with the `list` command:

```
$ ./app.flatimage fim-bind list
{
  "0": {
    "dst": "/dev/video0",
    "src": "/dev/video0",
    "type": "dev"
  },
  "1": {
    "dst": "/Music",
    "src": "$HOME/Music",
    "type": "ro"
  }
}
```

---

You can erase a binding with the `del` command, which takes and index as
argument.
```
$ ./app.flatimage fim-bind del 1
$ ./app.flatimage fim-bind list
{
  "0": {
    "dst": "/dev/video0",
    "src": "/dev/video0",
    "type": "dev"
  }
}
```


## How it works

FlatImage uses [bubblewrap's](https://github.com/containers/bubblewrap)
bind mechanisms to make devices and directories available in the guest
container.
