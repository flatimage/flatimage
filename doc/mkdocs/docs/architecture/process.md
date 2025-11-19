# Process Management

## Overview

The subprocess system provides a high-level interface for spawning and managing child processes. It abstracts the complexities of `fork()`, `execve()`, and pipe management into a fluent builder pattern that handles stream redirection, I/O threading, and automatic resource cleanup.

**Key Features:**

- **Builder Pattern**: Fluent API for process configuration
- **Stream Modes**: Inherit parent streams, redirect through pipes, or silence output
- **Automatic Cleanup**: RAII ensures processes are waited on, preventing zombies
- **I/O Threading**: Detached threads handle bidirectional communication
- **Callbacks**: User-defined hooks execute before and after fork

## Architecture

### Components

**Subprocess (Builder)**

- Configures process parameters using method chaining
- Accumulates arguments, environment variables, and stream settings
- Spawns the process and transfers ownership to a Child handle

**Child (RAII Handle)**

- Manages the lifetime of a spawned process
- Provides `wait()` to retrieve exit code and `kill()` to send signals
- Destructor automatically waits for process, preventing zombies

**Stream Modes**

- **Inherit**: Child uses parent's stdin/stdout/stderr directly
- **Pipe**: Creates pipes with detached I/O threads for bidirectional communication
- **Null**: Silences output by redirecting to `/dev/null`

**I/O Threads (Pipe Mode Only)**

- Automatically spawned as detached threads
- Handle stdin writes and stdout/stderr reads concurrently
- Terminate when child process exits or pipes close

## Process Spawning Lifecycle

The subprocess spawning process involves configuration, forking, and execution across parent and child processes:

```mermaid
flowchart LR
    Start([spawn called]) --> Configuration

    subgraph Configuration["Pre-Fork Setup"]
        direction TB
        C1["Validate arguments<br/>and configuration"]
        C2["Create pipes if<br/>Stream::Pipe mode"]
        C3["Prepare environment<br/>variables"]

        C1 --> C2
        C2 --> C3
    end

    Configuration --> Fork["fork() system call"]

    Fork --> ParentPath
    Fork --> ChildPath

    subgraph ParentPath["Parent Process"]
        direction TB
        P1["Close child's<br/>pipe ends"]
        P2["Spawn I/O threads<br/>if Stream::Pipe"]
        P3["Execute parent<br/>callback if set"]
        P4["Return Child handle<br/>to caller"]

        P1 --> P2
        P2 --> P3
        P3 --> P4
    end

    subgraph ChildPath["Child Process"]
        direction TB
        CH1["Close parent's<br/>pipe ends"]
        CH2["Redirect stdio<br/>to pipes via dup2"]
        CH3["Setup death signal<br/>if configured"]
        CH4["Execute child<br/>callback if set"]
        CH5["execve replaces<br/>process image"]

        CH1 --> CH2
        CH2 --> CH3
        CH3 --> CH4
        CH4 --> CH5
    end

    ParentPath --> Runtime
    ChildPath --> Runtime

    subgraph Runtime["Concurrent Execution"]
        direction TB
        R1["I/O threads shuttle<br/>data via pipes"]
        R2["Child process executes<br/>program logic"]
        R3["Parent continues<br/>with Child handle"]

        R1 -.-> R2
    end

    Runtime --> Cleanup

    subgraph Cleanup["Process Termination"]
        direction TB
        CL1["Child exits"]
        CL2["I/O threads detect<br/>EOF and terminate"]
        CL3["Parent calls wait<br/>to reap child"]
        CL4["Exit code returned"]

        CL1 --> CL2
        CL2 --> CL3
        CL3 --> CL4
    end

    Cleanup --> End([Cleanup complete])

    style Configuration fill:#E3F2FD
    style ParentPath fill:#E8F5E9
    style ChildPath fill:#FFF3E0
    style Runtime fill:#F3E5F5
    style Cleanup fill:#FFE4B5
    style Start fill:#90EE90
    style End fill:#FFB6C6
```

**Process Flow:**

1. **Configuration**: Validates configuration and creates pipes if needed
2. **Fork**: Kernel creates a duplicate process; both continue from the same point
3. **Parent Path**: Closes unused pipe ends, spawns I/O threads, returns handle to user
4. **Child Path**: Closes unused pipe ends, redirects stdio, executes callbacks, calls execve
5. **Runtime**: Parent and child execute concurrently with I/O flowing through pipes
6. **Cleanup**: Child exits, threads terminate on EOF, parent waits for child


## Stream Modes

The subprocess system supports three stream handling modes that control how I/O is managed:

```mermaid
flowchart LR
    Start{Stream<br/>Mode?} --> Inherit
    Start --> Pipe
    Start --> Null

    subgraph Inherit["Stream::Inherit"]
        direction TB
        I1["Child uses parent's<br/>stdin/stdout/stderr"]
        I2["Direct terminal access"]
        I3["No pipes or threads"]
    end

    subgraph Pipe["Stream::Pipe"]
        direction TB
        P1["Pipes created for<br/>stdin/stdout/stderr"]
        P2["Detached I/O threads<br/>handle communication"]
        P3["Optional log file output"]
    end

    subgraph Null["Stream::Null"]
        direction TB
        N1["Redirect stdout/stderr<br/>to /dev/null"]
        N2["Silent execution"]
        N3["No output captured"]
    end

    style Inherit fill:#E8F5E9
    style Pipe fill:#E3F2FD
    style Null fill:#FFE4B5
    style Start fill:#F3E5F5
```

**Mode Characteristics:**

- **Inherit**: Child uses parent's streams directly. Best for interactive programs or when terminal access is required. No overhead.

- **Pipe**: Creates pipes with detached threads for bidirectional communication. Best for capturing output, feeding input, or logging. Threads automatically terminate when child exits.

- **Null**: Redirects output to `/dev/null` for silent execution. Best for background processes where output is not needed.

## I/O Communication (Pipe Mode)

When Stream::Pipe mode is used, the system automatically creates pipes and threads for bidirectional communication:

```mermaid
flowchart LR
    Start([Pipe Mode]) --> PipeSetup

    subgraph PipeSetup["Pipe Creation"]
        direction TB
        PS1["Create stdin pipe<br/>read/write ends"]
        PS2["Create stdout pipe<br/>read/write ends"]
        PS3["Create stderr pipe<br/>read/write ends"]

        PS1 --> PS2
        PS2 --> PS3
    end

    PipeSetup --> ThreadSpawn

    subgraph ThreadSpawn["Thread Management"]
        direction TB
        T1["Spawn stdin write thread<br/>reads from parent stream"]
        T2["Spawn stdout read thread<br/>writes to parent stream"]
        T3["Spawn stderr read thread<br/>writes to parent stream"]

        T1 --> T2
        T2 --> T3
    end

    ThreadSpawn --> IOFlow

    subgraph IOFlow["I/O Flow"]
        direction TB
        IO1["Write thread:<br/>parent stream → child stdin"]
        IO2["Read threads:<br/>child stdout/stderr → parent streams"]
        IO3["Optional logging<br/>to file"]

        IO1 -.-> IO2
        IO2 -.-> IO3
    end

    IOFlow --> Termination

    subgraph Termination["Thread Termination"]
        direction TB
        TM1["Child exits"]
        TM2["Pipes close EOF"]
        TM3["Threads detect EOF<br/>and terminate"]

        TM1 --> TM2
        TM2 --> TM3
    end

    Termination --> End([Cleanup complete])

    style PipeSetup fill:#E3F2FD
    style ThreadSpawn fill:#E8F5E9
    style IOFlow fill:#FFF3E0
    style Termination fill:#FFE4B5
    style Start fill:#90EE90
    style End fill:#FFB6C6
```

**Thread Behavior:**

- **stdin Writer**: Reads from parent's input stream and writes to child's stdin pipe. Monitors child process to detect termination.

- **stdout/stderr Readers**: Read from child's output pipes and write to parent's streams. Optionally log all output to file. Filter line endings and empty lines.

- **Automatic Termination**: All threads are detached and terminate automatically when the child exits (EOF on pipes).

## Resource Management

The subprocess system uses RAII (Resource Acquisition Is Initialization) to ensure proper cleanup:

```mermaid
flowchart LR
    Start([User Code]) --> Builder

    subgraph Builder["Builder Phase"]
        direction TB
        B1["Create Subprocess<br/>with program path"]
        B2["Configure with_args<br/>with_env, etc."]
        B3["No resources allocated<br/>safe to destroy"]

        B1 --> B2
        B2 --> B3
    end

    Builder --> Spawn["spawn() called"]

    Spawn --> Resources

    subgraph Resources["Resource Allocation"]
        direction TB
        R1["Fork child process"]
        R2["Create pipes if needed"]
        R3["Spawn I/O threads<br/>if Pipe mode"]
        R4["Return Child handle"]

        R1 --> R2
        R2 --> R3
        R3 --> R4
    end

    Resources --> Active

    subgraph Active["Active Process"]
        direction TB
        A1["Child handle owns PID"]
        A2["I/O threads running"]
        A3["Can call wait or kill"]

        A1 -.-> A2
        A2 -.-> A3
    end

    Active --> Cleanup

    subgraph Cleanup["Automatic Cleanup"]
        direction TB
        C1["child->wait retrieves<br/>exit code"]
        C2["Destructor ensures<br/>wait is called"]
        C3["PID invalidated<br/>prevents zombies"]
        C4["Threads terminate<br/>on EOF"]

        C1 --> C2
        C2 --> C3
        C3 --> C4
    end

    Cleanup --> End([Resources freed])

    style Builder fill:#E3F2FD
    style Resources fill:#E8F5E9
    style Active fill:#FFF3E0
    style Cleanup fill:#FFE4B5
    style Start fill:#90EE90
    style End fill:#FFB6C6
```

**Key Guarantees:**

- **RAII Pattern**: Child destructor automatically waits for process, preventing zombie processes
- **Single Ownership**: Each Child handle exclusively owns one process PID
- **Thread Safety**: Detached I/O threads auto-terminate when child exits
- **No Resource Leaks**: Pipes and file descriptors are explicitly closed in both parent and child


## Usage Patterns

### Basic Process Execution

```cpp
// Simple command execution
auto result = Subprocess("/bin/ls")
    .with_args("-la", "/tmp")
    .spawn()
    ->wait();

if (result && *result == 0) {
    std::cout << "Success\n";
}
```

### Capturing Output

```cpp
// Capture stdout to string
std::ostringstream output;
auto child = Subprocess("/usr/bin/ps")
    .with_stdio(Stream::Pipe)
    .with_streams(std::cin, output, std::cerr)
    .with_args("aux")
    .spawn();

child->wait();
std::cout << "Captured: " << output.str() << "\n";
```

### Logging to File

```cpp
// Log all output to file
auto child = Subprocess("/usr/bin/make")
    .with_log_file("/tmp/build.log")
    .with_args("all")
    .spawn();

auto exit_code = child->wait();
```

### Custom Callbacks

```cpp
// Child callback: change directory before execve
Subprocess("/usr/bin/app")
    .with_callback_child([](ArgsCallbackChild args) {
        if (chdir("/tmp") < 0) {
            _exit(1);
        }
    })
    .spawn();

// Parent callback: track PID
pid_t child_pid;
Subprocess("/usr/bin/daemon")
    .with_callback_parent([&child_pid](ArgsCallbackParent args) {
        child_pid = args.child_pid;
        std::cout << "Spawned: " << args.child_pid << "\n";
    })
    .spawn();
```

### Process Supervision

```cpp
// Ensure child dies with parent
pid_t my_pid = getpid();
auto daemon = Subprocess("/usr/bin/service")
    .with_die_on_pid(my_pid)
    .spawn();

// Parent exits/crashes -> child receives SIGKILL
```

### Silent Execution

```cpp
// Background process with no output
auto background = Subprocess("/usr/bin/backup")
    .with_stdio(Stream::Null)
    .with_args("--full")
    .spawn();
```

### Concurrent Execution

```cpp
// Spawn multiple children concurrently
auto child1 = Subprocess("/bin/worker").with_args("task1").spawn();
auto child2 = Subprocess("/bin/worker").with_args("task2").spawn();
auto child3 = Subprocess("/bin/worker").with_args("task3").spawn();

// Wait for all
child1->wait();
child2->wait();
child3->wait();
```
