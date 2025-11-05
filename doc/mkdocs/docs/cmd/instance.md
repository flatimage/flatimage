# Multiple Instances

## What is it?

The `fim-instance` command allows you to manage and interact with multiple running instances of the same FlatImage container. Each instance is an independent containerized process that can be controlled separately, enabling parallel execution and inter-instance communication.

**Use Cases:**

- Running multiple instances of the same application simultaneously
- Executing commands in long-running background containers
- Testing different configurations in parallel instances
- Debugging running containers without stopping them
- Coordinating between multiple container instances
- Running one instance as root and another as regular user

**Key Concept:** The instance ID is assigned sequentially starting from 0 for each new FlatImage execution.

## How to Use

You can use `./app.flatimage fim-help instance` to get the following usage details:

```txt
fim-instance : Manage running instances
Usage: fim-instance <exec> <id> [args...]
  <exec> : Run a command in a running instance
  <id> : ID of the instance in which to execute the command
  <args> : Arguments for the 'exec' command
Example: fim-instance exec 0 echo hello
Usage: fim-instance <list>
  <list> : Lists current instances
```

### List Running Instances

Display all currently running FlatImage instances:

```bash
./app.flatimage fim-instance list
```

**Example output:**
```
0:2179490
1:2179708
```

Each line shows:

- **Instance ID** - Sequential number (0, 1, 2, ...)
- **Process ID** - Host system PID of the container

### Execute Commands in an Instance

Run commands in a specific running instance:

```bash
# Execute command in instance 0
./app.flatimage fim-instance exec 0 echo "Hello from instance 0"

# Execute command in instance 1
./app.flatimage fim-instance exec 1 pwd
```

The command executes within the target instance's container environment, inheriting its filesystem state and user context.

### Basic Multi-Instance Example

Run multiple instances and interact with them:

```bash
# Start first instance in background (as regular user)
./app.flatimage fim-exec sleep 1000 &
[1] 2179490

# Start second instance in background (as root)
./app.flatimage fim-root sleep 1000 &
[2] 2179708

# List running instances
./app.flatimage fim-instance list
0:2179490
1:2179708

# Check user in instance 0 (regular user)
./app.flatimage fim-instance exec 0 id -u
1000

# Check user in instance 1 (root)
./app.flatimage fim-instance exec 1 id -u
0

# Verify process isolation
./app.flatimage fim-instance exec 0 ps aux
# Shows only processes from instance 0

./app.flatimage fim-instance exec 1 ps aux
# Shows only processes from instance 1
```

### Instance Lifecycle

Understanding how instances start and stop:

```bash
# Instance starts when you execute a command
./app.flatimage fim-exec sleep 100 &
# Instance 0 created with PID 12345

# Instance continues until the main process exits
./app.flatimage fim-instance exec 0 echo "Still running"
# Works while sleep is active

# Instance stops when main process terminates
kill 12345
# Instance 0 no longer exists

# Verify instance is gone
./app.flatimage fim-instance list
# Empty or shows other instances only
```

### Working with Instance IDs

Instance IDs are assigned sequentially:

```bash
# Start three instances
./app.flatimage fim-exec sleep 100 &  # Gets ID 0
./app.flatimage fim-exec sleep 100 &  # Gets ID 1
./app.flatimage fim-exec sleep 100 &  # Gets ID 2

# List them
./app.flatimage fim-instance list
0:12345
1:12346
2:12347

# Stop instance 1
kill 12346

# List again - gap in IDs
./app.flatimage fim-instance list
0:12345
2:12347

# New instance gets next sequential ID (3, not 1)
./app.flatimage fim-exec sleep 100 &  # Gets ID 3

./app.flatimage fim-instance list
0:12345
2:12347
3:12348
```

**Note:** Instance IDs are not reused within the same FlatImage execution session.

## How it Works

FlatImage uses a dual-daemon architecture to enable inter-process communication between the host and running container instances.

**Technical Details:**

**Daemon System:**

FlatImage spawns two types of daemons:

1. **Host Daemon** (one per FlatImage execution)

   - Runs on the host system
   - Listens for commands from guest containers
   - Routes `fim-instance exec` commands to appropriate guest daemon

2. **Guest Daemon** (one per container instance)

   - Runs inside each container instance
   - Listens for commands from the host
   - Executes commands within its container context
   - Reports results back to the host

**Command Flow:**

When you run `fim-instance exec 0 command`:

1. FlatImage binary sends request to host daemon
2. Host daemon identifies target guest daemon (instance 0)
3. Host daemon forwards command to guest daemon 0's pipe
4. Guest daemon 0 executes command in its container
5. Guest daemon 0 sends output/exit code back to host daemon
6. Host daemon returns results to FlatImage binary
7. FlatImage binary displays output to user