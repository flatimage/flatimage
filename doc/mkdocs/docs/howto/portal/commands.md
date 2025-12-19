# Portal: Transparent Command Execution

Run commands between the host and container using the transparent portal mechanism and multi-instance system.

## What is the Portal?

The portal system enables bidirectional command execution between the host and container:

1. **Guest-to-Host**: Execute host commands from inside the container using `fim_portal`
2. **Host-to-Guest**: Execute container commands from the host using `fim-instance`

Both mechanisms preserve I/O streams and exit codes, making execution transparent.

## Basic Usage

### Guest-to-Host: Running Host Commands from Container

Inside the container, use `fim_portal` to run host commands:

```bash
# Enter the container
./app.flatimage

# Inside container - check if host has a command
[flatimage] / > fim_portal sh -c 'command -v firefox'
# Output: /usr/bin/firefox

# The return code is also forwarded
[flatimage] / > echo $?
# Output: 0

# Run host application
[flatimage] / > fim_portal firefox

# Run with arguments
[flatimage] / > fim_portal firefox https://example.com
```

### Host-to-Guest: Running Container Commands from Host

From the host, use `fim-instance exec` to run commands in a container instance:

```bash
# Start a container instance in the background
./app.flatimage fim-exec sleep 1000 &

# List running instances
./app.flatimage fim-instance list
# Output: 0:12345

# Execute command in instance 0
./app.flatimage fim-instance exec 0 sh -c 'echo "Hello from $FIM_DIST"'
# Output: Hello from ALPINE
```

## Quick Examples

### Guest-to-Host Examples

#### Check Host Environment

```bash
# Inside container
[flatimage] / > fim_portal whoami
# Output: user

[flatimage] / > fim_portal uname -r
# Output: 6.1.0-40-amd64

[flatimage] / > fim_portal sh -c 'cat /etc/*-release'
# Output:
# NAME="Arch Linux"
# PRETTY_NAME="Arch Linux"
# ...
# LOGO=archlinux-logo
```

#### Use Host Tools

Edit container files with host editor:

```bash
# Edit container file with host vim
[flatimage] / > fim_portal vim /tmp/container-file.txt

# Use host VS Code
[flatimage] / > fim_portal code /workspace/project

# Open host browser
[flatimage] / > fim_portal xdg-open https://example.com
```

### Host-to-Guest Examples

#### Inspect Running Container

```bash
# Start a long-running container
./app.flatimage fim-exec sleep 1000 &

# Check what's running inside
./app.flatimage fim-instance exec 0 ps aux

# View container filesystem
./app.flatimage fim-instance exec 0 ls -la /

# Check container environment
./app.flatimage fim-instance exec 0 env
```

#### Install Packages in Running Container

```bash
# Start container in background
./app.flatimage fim-root sleep 1000 &

# Install package in running container (Alpine)
./app.flatimage fim-instance exec 0 apk add curl

# Install package (Arch)
./app.flatimage fim-instance exec 0 pacman -S wget

# Verify installation
./app.flatimage fim-instance exec 0 which curl
```

## I/O Redirection

Both portal directions support standard I/O redirection:

### Guest-to-Host I/O

```bash
# Pipe to host command
[flatimage] / > echo "test" | fim_portal cat
test

# Redirect output
[flatimage] / > fim_portal echo "data" > /tmp/output.txt

# Redirect error
[flatimage] / > fim_portal some-command 2> /tmp/errors.txt

# Pipe between container and host
[flatimage] / > cat /container/data.txt | fim_portal grep pattern
```

### Host-to-Guest I/O

```bash
# Pipe to container command
echo "test data" | ./app.flatimage fim-instance exec 0 grep test
test data

# Redirect output from container
./app.flatimage fim-instance exec 0 cat /etc/os-release > container-info.txt

# Redirect error output
./app.flatimage fim-instance exec 0 failing-command 2> errors.log

# Chain commands
./app.flatimage fim-instance exec 0 cat /data.txt | sort | uniq > processed.txt
```

## Exit Codes

Both mechanisms preserve exit codes from executed commands:

### Guest-to-Host Exit Codes

```bash
# Successful command
[flatimage] / > fim_portal true
[flatimage] / > echo $?
0

# Failed command
[flatimage] / > fim_portal false
[flatimage] / > echo $?
1
```

### Host-to-Guest Exit Codes

```bash
# Successful command
./app.flatimage fim-instance exec 0 true
echo $?
0

# Failed command
./app.flatimage fim-instance exec 0 false
echo $?
1
```

## Learn More

- [Portal: Commands](../portal/commands.md) - Complete portal documentation
- [Architecture: Portal](../../architecture/portal.md) - Technical details of portal system
- [fim-instance](../../cmd/instance.md) - Complete instance management documentation
- [fim-exec](../../cmd/exec.md) - Execute commands as user inside container
- [fim-root](../../cmd/root.md) - Execute commands as root inside container