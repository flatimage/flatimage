@page conventions Conventions

Understanding these conventions will help you read the code more effectively:

## Namespace Naming {#conventions_namespace_naming}

- **All namespaces** use `ns_` prefix
- Examples: `ns_log`, `ns_parser`, `ns_filesystems::ns_controller`
- **Nested namespaces** follow logical module hierarchy: `ns_db::ns_portal::ns_daemon`
- **Rationale**: Prevents conflicts, makes namespaces visually distinct, reduces global namespace pollution

## Error Handling Pattern {#conventions_error_handling}

### Value<T> Type

The primary error handling mechanism is `Value<T>`, an enhanced wrapper around `std::expected<T, std::string>`:

```cpp
// Function returns Value<int>
Value<int> divide(int a, int b) {
  return_if(b == 0, Error("E::Division by zero"));
  return a / b;
}
```

### Key Macros

- **`Pop(expr)`** - Unwrap a `Value<T>` or return error
  - Used to extract the value or propagate error up the call stack
  - Example: `int result = Pop(some_operation());`

- **`Try(expr, ...)`** - Convert C++ exceptions to `Value<T>`
  - Wraps expressions that throw exceptions
  - Example: `auto result = Try(std::stoi("123"), "I::Conversion failed");`

- **`Catch(expr)`** - Similar to `Try()`, but yields the `Value` object.
  - Example: `auto opt = Catch(risky_function());`

- **`Error(fmt, ...)`** - Create an error value with automatic logging
  - Combines error creation with logging
  - Example: `return Error("E::Invalid configuration: {}", reason);`

## Logging Convention {#conventions_logging}

Always use the `logger()` macro with level prefix:

```cpp
logger("I::Starting application");
logger("W::Performance warning: operation took {} ms", duration);
logger("E::Failed to open file: {}", path);
```

- **`D::`** - Debug (debug information, requires `FIM_DEBUG=1`)
- **`I::`** - Info (informational messages)
- **`W::`** - Warning (non-fatal issues)
- **`E::`** - Error (normal failures)
- **`C::`** - Critical (Aborting failures)

@note Don't worry about remembering this, it is a compile-time check 🐉

## Conditional Control Flow Macros {#conventions_control_flow}

The project provides simplified macros for common control flow patterns with optional logging:

### return_if

Conditionally return from a function:

```cpp
// Return with value and logging
return_if(error_code != 0, -1, "E::Operation failed with code {}", error_code);

// Void function return
return_if(ptr == nullptr,, "E::Null pointer encountered");

// Return without logging
return_if(done, result);
```

### break_if

Conditionally break from a loop:

```cpp
for(auto& item : items) {
  break_if(item.is_invalid(), "E::Invalid item found: {}", item.name());
}
```

### continue_if

Conditionally continue to next iteration:

```cpp
for(int i = 0; i < size; i++) {
  continue_if(i % 2 == 0, "D::Skipping even index {}", i);
  process(items[i]);
}
```

## Enumeration Generation {#conventions_enum}

The `ENUM` macro generates enumerations with automatic string conversion:

```cpp
ENUM(OverlayType, BWRAP, OVERLAYFS, UNIONFS)

// Automatically provides:
// - Enum values: OverlayType::BWRAP, OverlayType::OVERLAYFS, etc.
// - to_string(): convert enum to string representation
// - from_string(): parse string back to enum
```

This is particularly useful for configuration values that need serialization/deserialization.

## Binary Unit Literals {#conventions_binary_literals}

For memory size calculations, use binary unit literal operators:

```cpp
constexpr size_t reserved_size = 4_mib;  // 4 megabytes
constexpr size_t cache_size = 512_kib;   // 512 kilobytes
```

Available operators:
- `_kib` - Kibibyte (1024 bytes)
- `_mib` - Mebibyte (1024 * 1024 bytes)
- `_gib` - Gibibyte (1024 * 1024 * 1024 bytes)

```