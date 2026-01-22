# Architecture

Internal design patterns and architecture of FdFileLib.

## Directory Structure

```
posixCppFdFileLibrary/
├── include/fdfile/          # Public headers
│   ├── fdfile.hpp           # Single entry point
│   ├── record/              # Record types
│   │   ├── RecordBase.hpp
│   │   ├── FieldMeta.hpp
│   │   ├── FixedRecordBase.hpp
│   │   └── VariableRecordBase.hpp
│   ├── repository/          # Repository implementations
│   │   ├── RecordRepository.hpp
│   │   ├── UniformFixedRepositoryImpl.hpp
│   │   └── VariableFileRepositoryImpl.hpp
│   └── util/                # Utility classes
│       ├── UniqueFd.hpp
│       ├── MmapGuard.hpp
│       ├── FileLockGuard.hpp
│       └── textFormatUtil.hpp
├── src/                     # Implementation files
│   └── VariableFileRepositoryImpl.cpp
├── cmake/                   # CMake package config
├── docs/                    # Documentation
├── examples/                # Example code
└── tests/                   # Unit tests
```

## Design Patterns

### CRTP (Curiously Recurring Template Pattern)

Used for fixed-length records to achieve zero-vtable polymorphism:

```cpp
template <typename Derived>
class FixedRecordBase {
    bool serialize(char* buf) const {
        // Call derived class method without virtual dispatch
        static_cast<const Derived*>(this)->__getFieldValue(i, buf);
    }
};

class User : public FixedRecordBase<User> {
    // No vtable created
};
```

Benefits:
- No virtual function overhead
- Inline-able method calls
- Compile-time type safety

### Repository Pattern

Abstracts data persistence:

```cpp
template <typename T>
class RecordRepository {
    virtual bool save(const T& record, std::error_code& ec) = 0;
    virtual std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) = 0;
    // ...
};
```

### RAII (Resource Acquisition Is Initialization)

All resources are managed automatically:

```cpp
// File descriptors
detail::UniqueFd fd(::open(path, O_RDWR));
// Automatically closed when fd goes out of scope

// Memory mapping
detail::MmapGuard mmap(ptr, size);
// Automatically unmapped

// File locks
detail::FileLockGuard lock(fd, Mode::Exclusive, ec);
// Automatically unlocked
```

## Component Diagram

```
┌─────────────────────────────────────────────────────────┐
│                     Application                          │
├─────────────────────────────────────────────────────────┤
│  ┌─────────────────────┐  ┌─────────────────────────┐  │
│  │  User-Defined       │  │  User-Defined           │  │
│  │  FixedRecord        │  │  VariableRecord         │  │
│  └──────────┬──────────┘  └───────────┬─────────────┘  │
│             │                         │                 │
│  ┌──────────▼──────────┐  ┌───────────▼─────────────┐  │
│  │  FixedRecordBase    │  │  VariableRecordBase     │  │
│  │  (CRTP Template)    │  │  (Virtual Interface)    │  │
│  └──────────┬──────────┘  └───────────┬─────────────┘  │
│             │                         │                 │
│  ┌──────────▼──────────┐  ┌───────────▼─────────────┐  │
│  │  UniformFixed       │  │  VariableFile           │  │
│  │  RepositoryImpl     │  │  RepositoryImpl         │  │
│  └──────────┬──────────┘  └───────────┬─────────────┘  │
│             │                         │                 │
│  ┌──────────▼─────────────────────────▼─────────────┐  │
│  │              Utility Classes                      │  │
│  │  UniqueFd │ MmapGuard │ FileLockGuard            │  │
│  └──────────────────────────────────────────────────┘  │
│                          │                              │
├──────────────────────────▼──────────────────────────────┤
│                    POSIX APIs                           │
│  open() │ read() │ write() │ mmap() │ fcntl() │ close()│
└─────────────────────────────────────────────────────────┘
```

## Data Flow

### Fixed Record Write

```
User::serialize()
    │
    ▼
FixedRecordBase::serialize()
    │
    ├── Copy format template
    ├── Write type name
    ├── Write ID
    └── Write fields (via CRTP)
    │
    ▼
UniformFixedRepositoryImpl::save()
    │
    ├── Acquire exclusive lock
    ├── Check external modifications
    ├── Find by ID in cache
    │   ├── Found → Update in-place
    │   └── Not found → Append
    ├── Update cache
    └── Release lock
```

### Fixed Record Read

```
UniformFixedRepositoryImpl::findById()
    │
    ├── Acquire shared lock
    ├── Check external modifications
    │   └── Changed → Rebuild cache
    ├── Lookup in cache (O(1))
    ├── Read from mmap
    ├── Deserialize
    └── Release lock
    │
    ▼
User::deserialize()
    │
    ▼
Return std::unique_ptr<User>
```

## Caching Strategy

### ID Cache (Fixed Records)

```cpp
std::unordered_map<std::string, size_t> idCache_;
// Maps ID → record index

// Cache invalidation triggers:
// 1. fstat() detects mtime change
// 2. fstat() detects size change
// 3. deleteById() shifts indices
```

### Record Cache (Variable Records)

```cpp
std::vector<std::unique_ptr<VariableRecordBase>> cache_;
bool cacheValid_ = false;

// Cache invalidation triggers:
// 1. stat() detects mtime change
// 2. stat() detects size change
// 3. Any write operation
```

## File Locking

Using POSIX fcntl advisory locks:

```cpp
struct flock fl;
fl.l_type = F_RDLCK;   // Shared (read)
fl.l_type = F_WRLCK;   // Exclusive (write)
fl.l_type = F_UNLCK;   // Unlock
fl.l_whence = SEEK_SET;
fl.l_start = 0;
fl.l_len = 0;          // Entire file

fcntl(fd, F_SETLKW, &fl);  // Blocking wait
```

## Memory Mapping

For fixed records, mmap provides zero-copy access:

```cpp
void* ptr = mmap(nullptr, size, 
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 fd, 0);

// Direct memory access to file contents
char* record = static_cast<char*>(ptr) + index * recordSize;

// Sync changes to disk
msync(ptr, size, MS_SYNC);

// Cleanup
munmap(ptr, size);
```

## Thread Safety

- File locks provide multi-process safety
- Single-threaded within a process (no internal mutex)
- For multi-threaded access, use external synchronization

## Extension Points

### Custom Record Types

Implement either:
- `FixedRecordBase<T>` for binary records
- `VariableRecordBase` for text records

### Custom Repositories

Implement `RecordRepository<T>` interface:
- Different storage backends (SQLite, network, etc.)
- Custom caching strategies
- Different serialization formats
