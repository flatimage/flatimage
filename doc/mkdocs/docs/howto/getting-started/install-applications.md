# Install Applications

Install software inside your FlatImage using package managers or recipes.

## Prerequisites

Grant network permission for package downloads:

```bash
./app.flatimage fim-perms add wayland,xorg,network,audio
```

## Using Recipes

Recipes install curated package sets with dependencies resolved automatically.

### Setup Remote Repository

```bash
./app.flatimage fim-remote set https://raw.githubusercontent.com/flatimage/recipes/master
```

### Install a Recipe

```bash
# Enable permissions
./app.flatimage fim-perms set wayland,xorg,audio,network

# Install firefox
./app.flatimage fim-recipe install firefox
```

## Using Package Managers

### Alpine Linux (apk)

```bash
# Update package index
./alpine.flatimage fim-root apk update

# Install packages
./alpine.flatimage fim-root apk add firefox font-noto
```

### Arch Linux (pacman)

```bash
# Update system
./arch.flatimage fim-root pacman -Syu

# Install packages
./arch.flatimage fim-root pacman -S firefox noto-fonts
```


## Verify Installation

Run the installed application:

```bash
# As regular user
./app.flatimage fim-exec firefox

# Check if binary exists
./app.flatimage fim-exec which firefox
```

## Learn More

- [fim-root](../../cmd/root.md) for root permissions in the container.
- [fim-exec](../../cmd/exec.md) to execute commands as a regular user.
- [fim-perms](../../cmd/perms.md) to configure permissions.
- [fim-remote](../../cmd/remote.md) to setup a remote to pull recipes from.
- [fim-recipe](../../cmd/recipe.md) to fetch and install recipes.
