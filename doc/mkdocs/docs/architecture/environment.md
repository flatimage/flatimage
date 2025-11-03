## What is it?

FlatImage provides a number of environment variables to query information about
filesystem paths and metadata.

## How to use

**Modifiable**:

* `FIM_DEBUG`       : If defined to 1, print debug messages.
* `FIM_MAIN_OFFSET` : Shows filesystem offset and exits.

**Filesystem Paths**:

* `FIM_DIR_GLOBAL`        : Temporary FlatImage directory, `/tmp/fim`
* `FIM_DIR_APP`           : Applications directory, `/tmp/fim/app/commit_timestamp`
* `FIM_DIR_APP_BIN`       : Application binaries directory, `$FIM_DIR_APP/bin`
* `FIM_DIR_INSTANCE`      : Directory of the current app instance, `$FIM_DIR_APP/instance/[pid]`
* `FIM_DIR_MOUNT`         : Directory of instance mount points, `$FIM_DIR_INSTANCE/mount`
* `FIM_DIR_RUNTIME`       : Runtime directories, `/tmp/fim/run`
* `FIM_DIR_RUNTIME_HOST`  : Read-only runtime host access, `$FIM_DIR_RUNTIME/host`
* `FIM_DIR_CONFIG`        : Configuration directory in the host machine, `.app.flatimage.config`
* `FIM_FILE_BINARY`       : Full path to the flatimage file

**FlatImage MetaData**

* `FIM_VERSION`           : The version of the flatimage package
* `FIM_DIST`              : The linux distribution name (alpine, arch)

## How it works

After the `boot` binary is copied to `/tmp/fim`, FlatImage forks and execves into the copied binary with the environment configuration defined by `relocate.hpp` and `config.hpp`.