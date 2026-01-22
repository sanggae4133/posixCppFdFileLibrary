# FdFileLib Documentation

Welcome to the FdFileLib documentation! This library provides high-performance file-based record storage using POSIX APIs.

## Table of Contents

- [Getting Started](getting-started.md) - Installation and basic usage
- [Fixed Records](fixed-records.md) - High-performance binary records
- [Variable Records](variable-records.md) - Flexible JSON-like records
- [Repository Pattern](repository-pattern.md) - CRUD operations
- [API Reference](api-reference.md) - Complete API documentation
- [Architecture](architecture.md) - Internal design and patterns
- [Error Handling](error-handling.md) - Error handling patterns

## Quick Links

| Topic | Description |
|-------|-------------|
| [Getting Started](getting-started.md) | Installation, CMake setup, first example |
| [Fixed Records](fixed-records.md) | CRTP-based zero-vtable binary records |
| [Variable Records](variable-records.md) | Virtual polymorphism JSON-like records |
| [Repository Pattern](repository-pattern.md) | CRUD operations and caching |
| [API Reference](api-reference.md) | Detailed class and function documentation |

## Feature Overview

### Record Types

| Type | Format | Lookup | Use Case |
|------|--------|--------|----------|
| Fixed-Length | Binary | O(1) | High-performance, structured data |
| Variable-Length | JSON-like | O(N) | Flexible, schema evolution |

### Key Features

- **RAII Resource Management** - Automatic cleanup for file descriptors, mmap, locks
- **O(1) ID Lookup** - Internal hash map cache for fixed records
- **External Modification Detection** - Auto-reload on file changes
- **File Locking** - fcntl-based concurrent access control
- **Zero-Copy mmap** - Direct memory mapping for performance

## Version

Current version: **1.0.0**

```cpp
#include <fdfile/fdfile.hpp>

std::cout << FdFile::VERSION_STRING; // "1.0.0"
```
