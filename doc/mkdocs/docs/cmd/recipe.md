# Manage Package Recipes

## What is it?

The `fim-recipe` command provides a convenient way to fetch, inspect, and install pre-configured package recipes from a remote repository. Recipes are JSON files that define collections of packages for specific purposes (like GPU support, audio tools, or video tools), making it easy to install multiple related packages with a single command.

**Use Cases:**

- Managing package sets without memorizing individual package names
- Installing common package groups (GPU drivers, audio support, development tools)
- Setting up consistent environments across multiple FlatImages distributions
- Discovering packages through recipe descriptions

**How Recipes Work:**

Recipes are simple JSON files stored on a remote repository. Each recipe contains:

- A description of what the packages provide
- A list of packages to install
- (Optional) A list of dependencies on other recipes

When you install a recipe, FlatImage automatically:

1. Downloads the recipe file and all its dependencies recursively
2. Validates that no cyclic dependencies exist
3. Extracts the package list from all recipes
4. Installs all packages using your distribution's package manager

## How to Use

You can use `./app.flatimage fim-help recipe` to get the following usage details:

```txt
fim-recipe : Fetch, inspect, and install recipes from a remote repository
Usage: fim-recipe <fetch> <recipes>
  <fetch> : Download one or more recipes without installing packages
  <recipes> : Name(s) of the recipe(s) to download (comma-separated for multiple)
Note: Recipes are downloaded from URL/DISTRO/latest/<recipe>.json to path_dir_host_config/recipes/DISTRO/latest/<recipe>.json
Example: fim-recipe fetch gpu
Example: fim-recipe fetch gpu,audio,xorg
Usage: fim-recipe <info> <recipes>
  <info> : Display information about one or more locally cached recipes
  <recipes> : Name(s) of the recipe(s) to inspect (comma-separated for multiple)
Example: fim-recipe info gpu
Example: fim-recipe info gpu,audio,xorg
Usage: fim-recipe <install> <recipes>
  <install> : Download one or more recipes and install their packages
  <recipes> : Name(s) of the recipe(s) to install (comma-separated for multiple)
Note: The remote URL must be configured using 'fim-remote set <url>'
Example: fim-recipe install gpu
Example: fim-recipe install gpu,audio,xorg
```

### Setting Up Remote URL

Before using recipes, configure the remote repository URL:

```bash
# Set the remote URL (required for fetching recipes)
./app.flatimage fim-remote set https://raw.githubusercontent.com/flatimage/recipes/master

# Verify the URL is configured
./app.flatimage fim-remote show
# Output: https://raw.githubusercontent.com/flatimage/recipes/master/
```

See the [fim-remote documentation](remote.md) for more details on managing remote URLs.

### Fetching Recipes

Download recipes from the remote repository to your local cache:

```bash
# Fetch a single recipe
./app.flatimage fim-recipe fetch gpu

# Fetch multiple recipes at once
./app.flatimage fim-recipe fetch gpu,audio,xorg
```

**Note:**

- The `fetch` command always overwrites cached recipes.
- Dependencies are automatically fetched recursively when you fetch a recipe.

### Inspecting Recipes

View information about locally cached recipes:

```bash
# Show information about a single recipe
./app.flatimage fim-recipe info gpu

# Example output:
# Recipe: gpu
# Location: /path/to/config/alpine/latest/gpu.json
# Description: GPU drivers and utilities for graphics acceleration
# Dependencies: 2
#   - xorg
#   - vulkan
# Package count: 5
# Packages:
#   - mesa-dri-gallium
#   - mesa-va-gallium
#   - mesa-vulkan-intel
#   - vulkan-loader
#   - libva-utils

# Inspect multiple recipes
./app.flatimage fim-recipe info gpu,audio
```

**Note:**

- The `info` command only reads from your local cache. If a recipe isn't cached, you'll need to fetch it first.
- Dependencies are listed if they exist in the recipe.

### Installing Recipes

Download recipes (if needed) and install all their packages:

```bash
# Install packages from a single recipe
./app.flatimage fim-recipe install gpu

# Install packages from multiple recipes
./app.flatimage fim-recipe install gpu,audio,xorg

# The install command will:
# 1. Use cached recipes if available, or fetch them if not
# 2. Extract all package names from the recipes
# 3. Install all packages using your distribution's package manager
```

**Installation Details:**

- **Alpine Linux:** Uses `apk add --no-cache --update-cache --no-progress`
- **Arch Linux:** Uses `pacman -Syu --noconfirm --needed`

The `install` command is efficient with caching - it only downloads recipes that aren't already cached locally.

**Dependency Handling:**

- All dependencies are fetched recursively before installation begins
- The system validates that no cyclic dependencies exist
- Packages are collected in the correct dependency order
- If a recipe is missing or has cyclic dependencies, the installation will fail before any packages are installed

### Complete Workflow Example

A typical workflow for setting up a FlatImage with recipes:

```bash
# 1. Configure the remote repository
./app.flatimage fim-remote set https://raw.githubusercontent.com/flatimage/recipes/master

# 2. Allow networking
./app.flatimage fim-perms add network

# 3. Fetch recipes to see what's available
./app.flatimage fim-recipe fetch gpu,audio,xorg

# 4. Inspect the recipes to see what packages they contain
./app.flatimage fim-recipe info gpu
./app.flatimage fim-recipe info development

# 5. Install the desired recipes
./app.flatimage fim-recipe install gpu,audio

# 6. Install more recipes
./app.flatimage fim-recipe install xorg
```

## How it Works

The recipe system provides a streamlined package management workflow built on top of your distribution's native package manager.

**Recipe Storage and Caching:**

Recipes are stored as JSON files with the following structure:
```json
{
    "description": "Karts. Nitro. Action! SuperTuxKart is a 3D open-source arcade racer with a variety of characters, tracks, and modes to play",
    "packages": ["supertuxkart"],
    "dependencies": ["xorg","gpu","audio"]
}
```

**Recipe Schema:**

- `description` (string, optional): Human-readable description of the recipe
- `dependencies` (array of strings, optional): List of other recipe names that this recipe depends on
- `packages` (array of strings, required): List of package names to install

**Storage Locations:**

- **Remote:** `{remote_url}/{distro}/latest/{recipe_name}.json`
- **Local Cache:** `{config_dir}/recipes/{distro}/latest/{recipe_name}.json`

Where:

- `{remote_url}` is the URL configured with `fim-remote set`
- `{distro}` is the distribution name (lowercase, e.g., "alpine" or "arch")
- `{recipe_name}` is the name of the recipe

**Command Behavior:**

1. **fetch** - Always downloads from remote, overwrites local cache. Recursively fetches all dependencies.
2. **info** - Reads only from local cache, fails if recipe not cached. Displays dependencies if present.
3. **install** - Uses local cached recipes if available, downloads recursively if missing; validates no cycles exist, resolves dependency order, then installs all packages.

**Distribution Support:**

The recipe system automatically adapts to your distribution:

- **Alpine Linux:** Uses the `apk` package manager.
- **Arch Linux:** Uses `pacman` package manager.