# Changelog

## v2.0.1

### Changed

- **Documentation** - Updated logo URL in README.md.

### Fixed

- **Boot Startup** - Check for fusermount3 on startup to ensure FUSE compatibility.
- **Default Recipe URL** - Fixed default recipe URL configuration.
- **Layer Commit Cleanup** - Made commit cleanup more permissive to handle file erasure failures gracefully.

## v2.0.0

### Added

**Commands**

- **fim-recipe** - Fetch and install package recipes with recursive dependency resolution.
    - Support for embedding desktop integration in recipes.
- **fim-remote** - Configure remote URL for recipe management.
- **fim-desktop**
    - **dump** - Export desktop entry, MIME types, and icon data from image.
    - **clean** - Option to clean desktop integration files from the system.
    - Support for URLs in desktop setup JSON files.
- **fim-boot**
    - **set**, **show**, and **clear** sub-commands for boot configuration.
- **fim-layer** - Layer management with multiple sub-commands:
    - **commit** - Commit changes with three modes:
        - **binary** - Commit changes directly to the FlatImage binary.
        - **layer** - Save changes as a numbered layer file in the layers directory.
        - **file** - Export changes to a specific file path.
    - **list** - List all embedded and external layers in format `index:offset:size:path`.
- **fim-overlay** - Select and manage filesystem overlay type (BWRAP, OverlayFS, UnionFS) for performance tuning.
- **fim-version**
    - **short** - Dumps a short version string.
    - **full** - Dumps a json with version and build data.
    - **deps** - Dumps a json with dependency information.
- **fim-instance** - Manage multiple instances with instance listing and targeted command execution.
- **fim-perms**
    - **add** - Add all permission types at once with the `all` argument.
    - **clear** - Option to clear all permission entries.
    - **Permissions**
        - **Optical** - Include optical drive access (`/dev/sr*` and `/dev/sg*`).
        - **Dev** - Add 'dev' permission to grant access to all devices in `/dev`.
        - **Shm** - Include `/dev/shm` permission for shared memory access.
- **fim-unshare** - Configure Linux namespace isolation with bubblewrap's unshare options.
    - Supports 6 namespace types: **user**, **ipc**, **pid**, **net**, **uts**, **cgroup**.
- **fim-env**
    - **clear** - Option to clear all environment variables.

**Build & Integration**

- **MkDocs CMake Integration** - Seamlessly integrate MkDocs documentation generation with CMake build system.
- **Doxygen Developer Documentation** - Create comprehensive Doxygen-based developer documentation for codebase architecture and API.
- **GitHub Actions Integration** - Reusable composite actions for building, testing (DocTest & UnitTest), and platform CLI setup.
- **Gitea CI Support** - Full CI/CD support for self-hosted Gitea instances with act_runner compatibility.

**Testing**
- [Doctest](https://github.com/doctest/doctest) - Integration with C++ testing suite.
- [Unittest](https://docs.python.org/3/library/unittest.html) - CLI tests with python's unittest.

**Misc**

- **Portal Host-to-Container CLI** - FIFO-based inter-process communication mechanism enabling commands to execute on host from within container.
- **Multi-Instance Management** - Multiple FlatImage instances with independent filesystem state.
- **Multiple Data Directories** - Run the same application binary with distinct data directories via `FIM_DIR_DATA`.
- **Custom FlatImage Binary Path Query** - `FIM_DIR_SELF` environment variable to query the absolute FlatImage binary path.
- **BLUEPRINT Distribution** - Added BLUEPRINT to the distribution enumeration for empty container templates.
- **Thread-Safe Timeout Implementations** - Linux blocking calls with configurable timeouts for improved reliability.
- **Fork-Safe Logger** - Thread-local storage logger with comprehensive documentation and PID tracking.
- **Daemon Mode for Subprocesses** - Optional subprocess daemon mode with grandchild PID return via shared memory.
- **Signal Detection in Subprocess** - Detect and report WIFSIGNALED and WIFSTOPPED conditions after waitpid.

### Changed

- **Configuration** - Made `config.hpp` as a single source of truth for FlatImage.
    - All configuration centralized in `config.hpp` for predictability and maintainability.
    - Moved redundant runtime configurations to compile-time.
    - Moved initialization of paths to `config.hpp`.
    - Filesystem configuration defined in `config.hpp`.
    - Work directory moved to host data directory and instance directory.
- **Environment Variable Storage** - Saved environment variable configurations persistently in the image.
- **Bindings Configuration Storage** - Saved mount binding configurations persistently in the image.
- **Boot Command Configuration Storage** - Saved boot command in the FlatImage binary.
- **Case Folding Configuration Storage** - Saved case folding configuration in the FlatImage binary.
- **Binary Dependencies** - Updated and optimized all embedded tool dependencies.
- **System Device Access** - Improved system device access.
- **Desktop Entry** - Improved desktop integration code.
    - Automatic desktop integration performed when bwrap is invoked.
- **XDG Data Home** - Use XDG_DATA_HOME in desktop integration when defined for better standard compliance.
- **Parser System** - Complete rework of parser with `std::expected` for better error handling.
    - Split parser into executor and parser components.
    - Enhanced parsing interface for better modularity.
- **Reserved Space System** - Reworked reserved binary data library for improved reliability.
    - Compile-time reserved space validation.
    - Log file path in case of read/write failures.
- **Subprocess Library** - Complete rework with streams and threads for improved I/O handling.
    - Default subprocess redirection to `/dev/null`.
    - Stream logic moved to linux library.
    - Make `Stream::Pipe` and daemon mode mutually exclusive.
- **Logger System** - Centralized logging configuration with fork-safe design.
    - Option to change log format based on PID.
    - Avoid resetting log level in `log.hpp`.
    - Configure output streams in `log.hpp`.
    - Compile-time fmt for logging.
    - Use `std::println` for native C++ output.
- **Error Handling** - Compile-time fmt for expected values and improved error propagation.
    - Propagate errors on Pop macro.
    - Log location for exceptions.
    - Configurable return type for `get_expected`.
- **Portal System** - Refactored portal architecture with improved daemon management.
    - Portal daemon started right before bwrap.
    - Detached portal daemon from shell.
    - Use (de-)serialization of portal objects.
- **Filesystem Controller** - Reworked filesystems controller for better layer management.
    - Moved utility functions to `utils.hpp`.
    - Replaced `FIM_[DIRS,FILES]_LAYER` with unified `FIM_LAYERS`.
    - Auto-switch to unionfs when bwrap overlays are used with casefold.
    - Cleanup work directory in filesystem controller.
- **Bwrap Integration** - Improved bubblewrap configuration and overlay handling.
    - Use regular constructor to reduce complexity on bwrap 'work' removal.
    - Removed 'work' bwrap overlay directory.
    - Make bwrap's return code transparent, or exit 125 on error.
- **User Identification** - Reworked user identify resolution for better containerized environment support.
    - Move bashrc to config and make UID and GID configurable.
    - Auto creation of passwd file.
- **Build System** - Enhanced cmake definitions setup and improved deployment scripts.

### Fixed

- **User Identification Setup** - Fixed user identification mechanism in containerized environment.
- **Duplicated Clamp Logic** - Removed duplicate clamp implementations in config.hpp and layers.hpp.
- **Desktop Entry Setup Warning** - Changed desktop entry setup errors to non-blocking warnings.
- **Help Message Formatting** - Standardized help messages to use 2 spaces instead of 3.
- **Desktop Clean JSON Zeroing** - Ensured JSON data is properly zeroed during desktop integration cleanup.
- **Subprocess Variable Error** - Fixed undefined variable error in subprocess.hpp.
- **Desktop Entry Name Sanitization** - Sanitized application names in desktop entry to prevent invalid characters.
- **CIOPFS Examples** - Fixed and included comprehensive example of CIOPFS usage in documentation.
- **Unused Read-Only Flag** - Removed unused is_readonly flag from configuration.
- **Missing Icon Path Handling** - Handled missing icon paths gracefully in desktop entry.
- **DwarFS File Inclusion** - Only include regular files that are readable in DwarFS layers.
- **PulseAudio-Bluetooth Root Ownership** - Handled PulseAudio-Bluetooth files that are only removable by root.
- **Reversed Filesystem Layers** - Fixed double-reversed filesystem layers redundancy.
- **Process Termination in Forks** - Replaced std::abort() with _exit() in child processes for proper cleanup.