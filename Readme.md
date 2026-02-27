# FdFileLib

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A high-performance C++17 file repository library using POSIX APIs (`open`, `read`, `write`, `fsync`, `flock`) and `mmap` for efficient I/O. Supports both **Variable-Length (JSON-like)** and **Strict Fixed-Length (Binary)** record formats.

## Features

| Feature | Description |
|---------|-------------|
| **High Performance** | Direct POSIX I/O with `mmap` for O(1) random access |
| **O(1) ID Lookup** | Internal hash map cache for blazing fast `findById()` |
| **Zero Vtable Overhead** | CRTP (Curiously Recurring Template Pattern) for fixed records |
| **C++17 Tuple-Based** | Modern field metadata using `std::tuple` and fold expressions |
| **RAII Safety** | Automatic cleanup for FD, mmap, and file locks |
| **External Modification Detection** | Auto-reload when external process modifies file |
| **File Locking** | `fcntl` based shared/exclusive locks for concurrency |
| **Upsert Semantics** | `save()` automatically inserts or updates |
| **149 Unit Tests** | Comprehensive GoogleTest coverage |

---

## Requirements

| Requirement | Version |
|-------------|---------|
| C++ Standard | C++17 |
| Compiler | GCC 7+, Clang 5+, Apple Clang 10+ |
| CMake | 3.16+ |
| Platform | POSIX (Linux/macOS) |
| Google Test | (optional, for tests) |

---

## Quick Start

### Installation

**Using CMake FetchContent (Recommended):**

```cmake
include(FetchContent)
FetchContent_Declare(
    fdfilelib
    GIT_REPOSITORY https://github.com/your-repo/fdfilelib.git
    GIT_TAG v1.0.0
)
# Examples and tests are automatically disabled when used as subdirectory
FetchContent_MakeAvailable(fdfilelib)

target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

**Using as subdirectory:**

```cmake
add_subdirectory(external/fdfilelib)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

**Using find_package (after installation):**

```cmake
find_package(fdfile REQUIRED)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

### Single Header Include

```cpp
#include <fdfile.hpp>  // Include everything
```

---

## Building from Source

```bash
# Clone
git clone <repo-url>
cd posixCppFdFileLibrary

# Build
mkdir build && cd build
cmake ..
cmake --build . -j

# Run tests
ctest --output-on-failure

# Install (optional)
cmake --install . --prefix /usr/local
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `FDFILE_BUILD_EXAMPLES` | ON* | Build example programs |
| `FDFILE_BUILD_TESTS` | ON* | Build unit tests |
| `FDFILE_INSTALL` | ON | Generate install target |

*\*When used as subdirectory, defaults to OFF*

---

## Project Structure

```
posixCppFdFileLibrary/
├── CMakeLists.txt              # Root CMake configuration
├── cmake/
│   └── fdfileConfig.cmake.in   # find_package() support template
│
├── fdFileLib/                  # Core Library
│   ├── fdfile.hpp              # Single header entry point
│   ├── record/
│   │   ├── RecordBase.hpp          # Base interface
│   │   ├── VariableRecordBase.hpp  # Variable-length (Virtual)
│   │   ├── FixedRecordBase.hpp     # Fixed-length (CRTP Template)
│   │   └── FieldMeta.hpp           # C++17 tuple-based field metadata
│   ├── repository/
│   │   ├── RecordRepository.hpp            # Repository interface
│   │   ├── VariableFileRepositoryImpl.hpp  # Variable-length repo
│   │   └── UniformFixedRepositoryImpl.hpp  # Fixed-length repo
│   └── util/
│       ├── UniqueFd.hpp        # RAII file descriptor
│       ├── MmapGuard.hpp       # RAII mmap wrapper
│       ├── FileLockGuard.hpp   # RAII fcntl file locking
│       └── textFormatUtil.hpp  # JSON-like parsing/formatting
│
├── examples/                   # Usage Examples
│   ├── main.cpp
│   └── records/
│       ├── FixedA.hpp, FixedB.hpp  # Fixed record examples
│       └── A.hpp, B.hpp            # Variable record examples
│
└── tests/                      # GoogleTest Unit Tests (149 tests)
    ├── CMakeLists.txt
    ├── unit/                   # Unit tests
    ├── scenario/               # Scenario tests
    └── integration/            # Integration tests
```

---

## Usage Guide

### 1. Fixed-Length Records (Recommended)

Fixed-length records are stored in binary format with `mmap` for maximum performance.

#### Key Features

| Feature | Implementation |
|---------|----------------|
| **O(1) Lookup** | Internal `unordered_map<id, index>` cache |
| **External Mod Detection** | `fstat()` checks mtime/size on every operation |
| **File Locking** | Shared lock (read), Exclusive lock (write) |
| **Auto Cache Rebuild** | On external file change, cache auto-refreshes |

#### Step 1: Define Your Record

```cpp
#pragma once
#include <fdfile.hpp>
#include <cstring>

class User : public FdFile::FixedRecordBase<User> {
public:
    char name[20];    // Fixed 20 bytes
    int64_t age;      // 20 digits (+/- + 19 digits)

private:
    auto fields() const {
        return std::make_tuple(
            FD_STR(name),  // String field macro
            FD_NUM(age)    // Numeric field macro
        );
    }

    void initMembers() {
        std::memset(name, 0, sizeof(name));
        age = 0;
    }

    // Generates: constructor, getId(), setId(), typeName(), etc.
    FD_RECORD_IMPL(User, "User", 10, 10)

public:
    User(const char* n, int64_t a, const char* id) {
        initMembers();
        if (n) std::strncpy(name, n, sizeof(name));
        age = a;
        setId(id);
        defineLayout();
    }
};
```

#### Step 2: Use Repository

```cpp
#include <fdfile.hpp>

int main() {
    std::error_code ec;
    FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);
    if (ec) return 1;

    // INSERT
    User alice("Alice", 30, "001");
    repo.save(alice, ec);

    // UPDATE (same ID)
    alice.age = 31;
    repo.save(alice, ec);  // Updates in-place

    // FIND BY ID (O(1) cache lookup)
    auto found = repo.findById("001", ec);
    if (found) {
        std::cout << found->name << ", " << found->age << "\n";
    }

    // OTHER OPERATIONS
    auto all = repo.findAll(ec);           // Get all
    bool exists = repo.existsById("001");  // Check existence
    size_t cnt = repo.count(ec);           // Count
    repo.deleteById("001", ec);            // Delete one
    repo.deleteAll(ec);                    // Delete all
}
```

---

### 2. Variable-Length Records

JSON-like format for flexibility. Uses virtual polymorphism.

```cpp
#include <fdfile.hpp>

class Config : public FdFile::VariableRecordBase {
public:
    std::string key, value;
    
    std::string id() const override { return key; }
    const char* typeName() const override { return "Config"; }
    std::unique_ptr<RecordBase> clone() const override {
        return std::make_unique<Config>(*this);
    }
    // Implement toKv() and fromKv() for serialization
};

// Requires prototype instances
std::vector<std::unique_ptr<VariableRecordBase>> protos;
protos.push_back(std::make_unique<Config>());
FdFile::VariableFileRepositoryImpl repo("config.txt", std::move(protos), ec);
```

---

## API Reference

### `UniformFixedRepositoryImpl<T>`

| Method | Description | Complexity |
|--------|-------------|------------|
| `save(record, ec)` | Upsert | O(1) cache + O(1) write |
| `findById(id, ec)` | Find by ID | **O(1)** (cache hit) |
| `findAll(ec)` | Get all | O(N) |
| `existsById(id, ec)` | Check exists | **O(1)** |
| `deleteById(id, ec)` | Delete one | O(N) shift |
| `deleteAll(ec)` | Clear all | O(1) |
| `count(ec)` | Count | O(1) |

### Macros

| Macro | Description |
|-------|-------------|
| `FD_STR(member)` | String field (char array) |
| `FD_NUM(member)` | Numeric field (int64_t, +/- 19 digits) |
| `FD_RECORD_IMPL(Class, Type, TypeLen, IdLen)` | Generate record methods |

---

## Testing

### Test Categories

| Category | Directory | Tests | Description |
|----------|-----------|-------|-------------|
| **Unit** | `tests/unit/` | 62 | Individual classes/functions |
| **Scenario** | `tests/scenario/` | 16 | Feature workflows |
| **Integration** | `tests/integration/` | 9 | Concurrency, permissions |
| **Corruption** | `tests/*.cpp` | 62 | File corruption scenarios |

### Run Tests

```bash
# All tests
cd build && ctest --output-on-failure

# Specific tests
./tests/fdfile_tests --gtest_filter="FieldMeta*"
./tests/fdfile_tests --gtest_filter="*Corruption*"

# Result: 100% tests passed, 0 tests failed out of 149
```

---

## Architecture

### CRTP Pattern

```
FixedRecordBase<Derived>   ← Template base
        ↑
        │ static_cast<Derived*>(this)
        │
      User                 ← Concrete type
```

### ID Cache Flow

```
findById("001")
    ↓
checkAndRefreshCache()
    ├── fstat() → mtime changed?
    │       ├── Yes → remapFile() + rebuildCache()
    │       └── No  → use existing cache
    ↓
idCache_.find("001") → O(1) lookup
    ↓
return record at index
```

---

## cmake/ Directory

The `cmake/fdfileConfig.cmake.in` file is a template for CMake package configuration. When you run `cmake --install`, it generates `fdfileConfig.cmake` that allows other projects to use this library via:

```cmake
find_package(fdfile REQUIRED)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

---

## License

MIT License - see [LICENSE](LICENSE) file for details.
