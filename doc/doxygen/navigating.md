@mainpage Overview

This guide will help you efficiently explore the FlatImage C++23 codebase documentation.
Whether you're fixing bugs, adding features, or understanding the architecture, knowing
how to navigate this documentation will save you time and effort.

[TOC]

---

# Getting Started

If you're new to FlatImage development, start here:

1. **Read the [Main Page](@ref index)** - Overview of architecture and key features
2. **Explore [Conventions](@ref conventions)** - See the coding conventions used by the codebase.
3. **Explore [Modules](@ref modules)** - See how the codebase is organized by functionality
4. **Check [Building](@ref building)** - Learn how to build FlatImage from source

---

# Finding What You Need

## By Topic

The documentation is organized into logical modules. Click on any section in the
[Modules Overview](@ref modules_overview) to see all related files:

- **[Core System](@ref mod_core)** - Configuration system, boot process, error handling macros
- **[Reserved Space](@ref mod_reserved)** - 4MB ELF-embedded configuration space (permissions, boot command, etc.)
- **[Filesystems](@ref mod_filesystems)** - DwarFS, OverlayFS, UnionFS, CIOPFS implementations
- **[Sandbox](@ref mod_sandbox)** - Bubblewrap integration and 15 permission types
- **[Parser](@ref mod_parser)** - Command-line parsing and 17+ fim-* command implementations
- **[Portal](@ref mod_portal)** - FIFO-based IPC system for host-container communication
- **[Database](@ref mod_database)** - Runtime state management (recipes, bindings, desktop integration)
- **[Library](@ref mod_library)** - Reusable utilities (logging, subprocess, ELF parsing, image processing)
- **[Std Extensions](@ref mod_std)** - C++ standard library extensions (Value, concepts, string utilities)

## By Code Element

Use the top navigation bar to browse by specific element types:

[**Classes**](annotated.html) - All class and struct definitions
- Organized alphabetically by namespace and name
- Each entry shows class hierarchy, member functions, and detailed descriptions
- Examples: `ns_bwrap::Bwrap`, `ns_filesystems::ns_controller::Controller`

[**Files**](files.html) - Browse the source code directory structure
- Click **File List** to see all header files
- Click **File Members** to search across all files for:
  - Functions (global and namespace-level)
  - Variables, constants, typedefs
  - Enumerations and enum values
  - Macros and preprocessor definitions

@note Click on '**Go to the source code of this file**' below the dependency graphs to view the actual source code with syntax highlighting.

[**Namespaces**](namespaces.html) - Code organized by namespace
- All FlatImage namespaces use `ns_` prefix
- Nested namespaces shown with hierarchy
- Examples: `ns_log`, `ns_parser::ns_cmd`, `ns_filesystems::ns_dwarfs`

---

# Search Features

## Quick Search

Use the **search box** in the top-right corner for instant results:

- Type **class names**: `Bwrap`, `Controller`, `CmdBoot`
- Type **function names**: `mount`, `read`, `write`, `execute`
- Type **file names**: `bwrap.hpp`, `controller.hpp`, `expected.hpp`
- Type **namespaces**: `ns_reserved`, `ns_filesystems`, `ns_portal`

---

# Working with Classes

When viewing a class page, you'll find:

## Class Overview Section

- **Brief description** - One-line summary of the class purpose
- **Detailed description** - Usage examples, design rationale, notes
- **Inheritance diagram** - Visual representation of class hierarchy
- **Collaboration diagram** - Shows relationships with other classes

## Member Sections

- **Public Types** - Nested types, enums, and typedefs
- **Public Member Functions** - Methods available to users
- **Public Attributes** - Public data members (rare in FlatImage)
- **Protected/Private Members** - Implementation details (collapsed by default)

## Navigation Within Classes

- Click **method names** to jump to detailed documentation
- Use **[more...]** links to expand collapsed sections
- Check **See also** sections for related classes and functions

@note Explore the [Bwrap class](@ref ns_bwrap::Bwrap)

---

# Working with Files

Each file page shows:

## File Information

- **File path** - Location in source tree
- **Brief description** - Purpose of the file
- **Include dependencies** - What this file includes (clickable graph)
- **Included by** - What files include this one (reverse dependencies)

## File Contents

- **Namespaces** - Namespaces defined in this file
- **Classes** - Classes and structs defined here
- **Functions** - Free functions and namespace-level functions
- **Variables** - Global variables and constants
- **Typedefs** - Type aliases and typedefs
- **Enumerations** - Enum types defined in this file
- **Macros** - Preprocessor macro definitions

## Source Code

- Click **Go to the source code** to view syntax-highlighted code
- Click line numbers to get permanent links to specific lines

@note Check out `expected.hpp` for essential error handling macros like `Pop()` and `Try()`.

---

# Understanding Graphs

Doxygen generates several types of visual graphs:

## Include Dependency Graphs

- **Include graphs** show what a file includes (dependencies going down)
- **Included by graphs** show what includes a file (dependencies going up)
- Click on boxes to navigate to those files
- Useful for understanding compile dependencies

## Class Inheritance Graphs

- Show base classes (parents) above
- Show derived classes (children) below
- Solid arrows indicate public inheritance
- Dashed arrows indicate protected/private inheritance

## Collaboration Graphs

- Show how classes interact with each other
- Arrows indicate "uses" or "contains" relationships
- Helpful for understanding object composition

@note Hover over graph nodes to see tooltips with brief descriptions.