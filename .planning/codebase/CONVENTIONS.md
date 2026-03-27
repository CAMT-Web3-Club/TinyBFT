# Coding Conventions - TinyBFT

## Language Standards
- **C**: C11 (`-std=c11`)
- **C++**: C++14 (`-std=c++14`)
- **No Exceptions**: C++ exceptions are disabled for embedded compatibility.

## Naming Conventions
- **Classes/Structs**: `PascalCase` (e.g., `class Replica`)
- **Functions/Methods**: `PascalCase` (e.g., `void SendMessage()`)
- **Variables**: `snake_case` (e.g., `int max_size`)
- **Member Variables**: `snake_case_` (trailing underscore, e.g., `int node_id_`)
- **Constants**: `SCREAMING_SNAKE_CASE` (e.g., `MAX_MESSAGE_SIZE`)
- **Enums**: `PascalCase` (e.g., `TransportType::UDP`)
- **Files**: `snake_case` (e.g., `replica_handler.cc`)

## Formatting
- **Indentation**: 4 spaces (no tabs).
- **Line Length**: 100 characters maximum.
- **Braces**: Allman style (braces on new lines).
- **Namespaces**: All library code must reside in the `libbyzea` namespace.

## Error Handling & Memory
- **Return Codes**: Use `int` (0 for success, negative for failure).
- **Assertions**: Use `th_assert()` for internal invariants.
- **Allocation**: Prefer static allocation (`STATIC_LOG_ALLOCATOR`) or `new`/`delete` for objects. No `malloc` in C++ code if possible.
