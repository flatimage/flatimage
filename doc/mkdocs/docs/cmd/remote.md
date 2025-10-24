# Configure Remote URL

## What is it?

The `fim-remote` command manages a remote URL for your FlatImage. This URL can be used to specify where recipes should be fetched from. The remote URL is stored directly in the FlatImage binary and can be queried or updated at any time.

## How to Use

The `fim-remote` command has three sub-commands: `set`, `show`, and `clear`.

```txt
fim-remote : Configure the remote URL for recipes
Usage: fim-remote <set> <url>
  <set> : Set the remote URL
  <url> : The remote URL to configure
Example: fim-remote set https://recipes.example.com/repo
Usage: fim-remote <show>
  <show> : Display the current remote URL
Usage: fim-remote <clear>
  <clear> : Clear the configured remote URL
```

### Set a Remote URL

Configure the remote URL for your FlatImage:

```bash
# Set a remote
./app.flatimage fim-remote set https://github.com/flatimage/recipes
```

### Show Current Remote URL

Display the currently configured remote URL:

```bash
./app.flatimage fim-remote show
```

**Example output when URL is set:**

```
https://github.com/flatimage/recipes
```

### Clear the Remote URL

Remove the configured remote URL:

```bash
./app.flatimage fim-remote clear
./app.flatimage fim-remote show
# No remote URL configured
```

## How it Works

The remote URL is stored directly in the FlatImage binary as embedded metadata. This design ensures:

1. **Persistence** - The URL remains part of the FlatImage across all operations
2. **Portability** - The remote configuration travels with the FlatImage file
3. **Simplicity** - No external configuration files needed

**Technical Details:**

- The remote URL is written to a dedicated reserved section in the ELF binary
- The data is stored as JSON for structured parsing
- Maximum URL length: 4 KiB (sufficient for most URL schemes)
- The URL can be accessed and modified without mounting filesystem layers