# Case-Insensitive Filesystem

## What is it?

The `fim-casefold` command enables case-insensitive filesystem behavior inside the FlatImage container. This makes the filesystem treat `File.txt`, `file.txt`, and `FILE.TXT` as the same file, similar to how Windows and macOS handle filenames.

**Use Cases:**

- Running Windows applications through Wine or similar compatibility layers
- Working with cross-platform projects that expect case-insensitive behavior
- Dealing with software that has inconsistent file casing in code
- Running legacy applications that assume case-insensitive filesystems

### Understanding Filesystem Case Sensitivity

#### Linux (Case-Sensitive by Default)

Most Linux filesystems (ext4, XFS, Btrfs) are **case-sensitive**:

```bash
# These are three DIFFERENT files
touch readme.txt
touch README.txt
touch ReadMe.txt

ls -1
README.txt
ReadMe.txt
readme.txt

# Each can exist simultaneously
cat readme.txt    # Opens readme.txt
cat README.txt    # Opens README.txt (different file)
```

#### Windows/macOS (Case-Insensitive)

Windows (NTFS, FAT32) and macOS (APFS) filesystems are **case-insensitive** but **case-preserving**:

```powershell
# These all refer to the SAME file
echo "test" > readme.txt
type README.TXT    # Works - reads the same file
type ReadMe.Txt    # Also works - same file

# Cannot create files that differ only by case
echo "different" > README.txt   # Overwrites the existing file
```

#### Why This Matters

Many applications, especially those ported from Windows, assume case-insensitive behavior:

- **Wine** - Windows applications expect Windows filesystem behavior
- **Game engines** - May reference assets with inconsistent casing
- **Cross-platform projects** - Code written on Windows may use wrong casing
- **Legacy software** - Older applications often have hard-coded case assumptions

## How to Use

You can use `./app.flatimage fim-help casefold` to get the following usage details:

```txt
fim-casefold : Enables casefold for the filesystem (ignore case)
Usage: fim-casefold <on|off>
  <on> : Enables casefold
  <off> : Disables casefold
```

### Enable Case-Insensitive Filesystem

Activate case folding permanently:

```bash
# Enable case-insensitive behavior
./app.flatimage fim-casefold on
```

**Example: Accessing files with any case**

```bash
# Enable case folding
./app.flatimage fim-casefold on

# Create a file with mixed case
./app.flatimage fim-exec touch /tmp/MyDocument.txt

# Access it using different casing
./app.flatimage fim-exec cat /tmp/mydocument.txt      # Works
./app.flatimage fim-exec cat /tmp/MYDOCUMENT.TXT      # Also works
./app.flatimage fim-exec cat /tmp/MyDoCuMeNt.TxT      # Still works

# List the file
./app.flatimage fim-exec ls /tmp/
mydocument.txt    # Note: stored in lowercase
```

### Disable Case-Insensitive Filesystem

Return to standard Linux case-sensitive behavior:

```bash
./app.flatimage fim-casefold off
```

**Important:** Files created while case folding was enabled will be stored in lowercase. After disabling, they are only accessible by their lowercase names.

### Temporary Configuration

Use an environment variable for temporary case folding without modifying the FlatImage:

```bash
# Enable for a single session
export FIM_CASEFOLD=1
./app.flatimage fim-exec touch /MyFile.txt
./app.flatimage fim-exec ls /myfile.txt  # Works

# In a new terminal (without the variable)
./app.flatimage fim-exec ls /myfile.txt   # Works (file is lowercase)
./app.flatimage fim-exec ls /MyFile.txt   # Fails (case-sensitive now)
```

**When to use temporary configuration:**

- Testing case folding before making it permanent
- One-off execution of applications that need case-insensitive filesystem
- Sharing FlatImage files where different users have different needs
- Debugging case-sensitivity issues

### Requires Specific Overlay Filesystems

Case folding only works with FUSE-based overlay filesystems. FlatImage auto-switches to `unionfs-fuse` if case folding is enabled and the current overlay option is `bwrap overlays`.

See [overlay documentation](./overlay.md) for more details.

## How it Works

FlatImage uses [ciopfs](https://www.brain-dump.org/projects/ciopfs) (Case-Insensitive On Purpose File System) to provide case-insensitive filesystem behavior.

**Implementation Details:**

1. When case folding is enabled, the configuration is written to the FlatImage binary
2. On startup, FlatImage checks if case folding is enabled
3. If enabled and a FUSE overlay is in use, ciopfs mounts the filesystem to `$FIM_DIR_CONFIG/casefold`
4. This case-insensitive mount becomes the root filesystem for [bubblewrap](https://github.com/containers/bubblewrap)
5. All filesystem operations inside the container are case-insensitive

**Why FUSE is Required:**

Bubblewrap's native overlay implementation operates at the kernel level and doesn't provide hooks for case-insensitive behavior. FUSE (Filesystem in Userspace) allows ciopfs to intercept and modify filesystem operations, converting all lookups to lowercase before passing them to the underlying filesystem.

**Storage Details:**

- Files are stored in lowercase in the actual filesystem layers
- Case conversion happens at runtime by ciopfs
- The case folding configuration is stored in the FlatImage binary metadata
- No additional storage overhead (beyond the ciopfs mount point)