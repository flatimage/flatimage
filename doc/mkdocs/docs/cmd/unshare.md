# Configure Namespace Unsharing

## What is it?

The `fim-unshare` command provides granular control over namespace isolation for applications running inside the FlatImage container. By default, FlatImage containers share certain Linux namespaces with the host system.

Resources:

- [namespaces(7) — Linux manual page](https://man7.org/linux/man-pages/man7/namespaces.7.html)
- [The 7 most used Linux namespaces ](https://www.redhat.com/en/blog/7-linux-namespaces)
- [What Are Namespaces and cgroups, and How Do They Work?](https://blog.nginx.org/blog/what-are-namespaces-cgroups-how-do-they-work)

## How to Use

You can use `./app.flatimage fim-help unshare` to get the following usage details:

```txt
fim-unshare : Configure namespace unsharing options for isolation
Note: Unshare options: all,user,ipc,pid,net,uts,cgroup
Note: USER and CGROUP use '-try' variants in bubblewrap for permissiveness
Usage: fim-unshare <add|del|set> <options...>
  <add> : Enable one or more unshare options
  <del> : Remove one or more unshare options
  <set> : Replace all unshare options with the specified set
  <options...> : One or more unshare options (comma-separated)
Example: fim-unshare add ipc,pid
Example: fim-unshare set user,ipc,net
Note: The 'all' option enables all available unshare options and cannot be combined with others
Usage: fim-unshare <list|clear>
  <list> : Lists the current unshare options
  <clear> : Clears all unshare options
```

### Add Unshare Options

Enable one or more namespace unsharing options:

```bash
# Add single unshare option
./app.flatimage fim-unshare add ipc

# Add multiple unshare options
./app.flatimage fim-unshare add ipc,pid,uts
```

Existing unshare options are preserved; new ones are added to the list.

### Set Unshare Options

Replace all unshare options with a specific set:

```bash
# Reset to only these unshare options
./app.flatimage fim-unshare set user,ipc,net
```

All previous unshare options are removed; only the specified ones remain.

**Difference between add and set:**

```bash
# Current: ipc,pid
./app.flatimage fim-unshare add uts
# Result: ipc,pid,uts

# Current: ipc,pid,uts
./app.flatimage fim-unshare set user,net
# Result: user,net (ipc,pid,uts removed)
```

### List Unshare Options

Display currently configured unshare options:

```bash
./app.flatimage fim-unshare list
```

**Example output:**
```
ipc
pid
user
```

### Remove Unshare Options

Delete one or more specific unshare options:

```bash
# Remove single unshare option
./app.flatimage fim-unshare del ipc

# Remove multiple unshare options
./app.flatimage fim-unshare del ipc,pid
```

Other unshare options remain unchanged.

### Clear All Unshare Options

Remove all unshare options, returning to default namespace sharing:

```bash
./app.flatimage fim-unshare clear
```

After clearing, the container shares all namespaces with the host (default behavior).

## How it Works

FlatImage uses [bubblewrap's](https://github.com/containers/bubblewrap) unshare options to create isolated Linux namespaces. Each unshare option translates to a corresponding bubblewrap flag:

| Unshare Option | Bubblewrap Flag | Description |
|----------------|-----------------|-------------|
| `user` | `--unshare-user-try` | User namespace isolation (permissive) |
| `ipc` | `--unshare-ipc` | IPC namespace isolation |
| `pid` | `--unshare-pid` | PID namespace isolation |
| `net` | `--unshare-net` | Network namespace isolation |
| `uts` | `--unshare-uts` | UTS namespace isolation |
| `cgroup` | `--unshare-cgroup-try` | Cgroup namespace isolation (permissive) |