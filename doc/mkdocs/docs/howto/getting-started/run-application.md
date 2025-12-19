# Run Applications

Execute applications inside your FlatImage container.

## One-Time Execution

Use `fim-exec` to run commands without entering the container:

```bash
# Run Firefox once
./app.flatimage fim-exec firefox

# Run with arguments
./app.flatimage fim-exec firefox https://example.com

# Run shell commands
./app.flatimage fim-exec sh -c 'ls -la /usr/bin | grep python'
```

## Set Default Boot Command

Configure what runs when you execute the FlatImage without arguments.

### Set the Boot Command

```bash
# Set Firefox as default
./app.flatimage fim-boot set firefox

# Now this launches Firefox directly
./app.flatimage
```

### With Pre-configured Arguments

```bash
# Always start Firefox in private mode
./app.flatimage fim-boot set firefox --private-window

# Launch with the pre-configured argument
./app.flatimage https://example.com
# Equivalent to: firefox --private-window https://example.com
```

### View Current Boot Command

```bash
./app.flatimage fim-boot show
```

Output:
```json
{
  "args": ["--private-window"],
  "program": "firefox"
}
```

### Clear Boot Command

Revert to default bash shell:

```bash
./app.flatimage fim-boot clear
```

## Complete Example

Create a portable application launcher:

```bash
# Download and setup
wget https://github.com/flatimage/flatimage/releases/latest/download/alpine.flatimage
chmod +x alpine.flatimage

# Configure permissions
./alpine.flatimage fim-perms add network,xorg,wayland,audio

# Install Firefox
./alpine.flatimage fim-root apk add firefox font-noto

# Set as boot command
./alpine.flatimage fim-boot set firefox

# Commit everything
./alpine.flatimage fim-layer commit binary

# Rename for clarity
mv alpine.flatimage firefox.flatimage

# Now it works like a native executable
./firefox.flatimage
./firefox.flatimage https://example.com
```

## Learn More

- [fim-perms](../../cmd/perms.md) for all available permissions.
- [fim-exec](../../cmd/exec.md) - Execute commands as regular user
- [fim-root](../../cmd/root.md) - Execute commands as root
- [fim-boot](../../cmd/boot.md) - Configure default boot command
