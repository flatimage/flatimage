# Commit Changes

Save your installed applications and configurations permanently.

## What is fim-layer commit?

The `fim-layer commit` command compresses your current filesystem changes (installed packages, modified files) into a permanent layer. Without committing, changes only persist in the temporary writable layer.

## Basic Usage

### Commit to Binary

Embed changes directly into the FlatImage executable:

```bash
# Set the compression level (0-9)
FIM_COMPRESSION_LEVEL=7
# Commit changes to binary
./app.flatimage fim-layer commit binary
```

Now `app.flatimage` is a portable application in a single file!

## What Gets Committed?

The commit captures all changes in the writable layer:

- Installed packages
- Created files and directories

## After Committing

- Changes are permanent and compressed
- The temporary writable layer is cleared
- The FlatImage size increases by the compressed layer size

## Learn More

- [fim-layer](../cmd/layer.md) for advanced layer management.
