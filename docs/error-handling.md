# Error Handling

FdFileLib uses `std::error_code` for all error handling.

## Basic Pattern

```cpp
std::error_code ec;

repo.save(record, ec);
if (ec) {
    std::cerr << "Error: " << ec.message() << std::endl;
    // Handle error
}
```

## Error Categories

### Standard Error Codes

| Code | Meaning |
|------|---------|
| `std::errc::invalid_argument` | Invalid parameter |
| `std::errc::bad_file_descriptor` | Invalid file descriptor |
| `std::errc::result_out_of_range` | Value out of range |

### System Error Codes

I/O operations may return system errno values:

| Code | Meaning |
|------|---------|
| `ENOENT` | File not found |
| `EACCES` | Permission denied |
| `ENOSPC` | No space left on device |
| `EIO` | I/O error |
| `ENOMEM` | Out of memory |

## Common Scenarios

### File Open Failure

```cpp
std::error_code ec;
FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);

if (ec) {
    if (ec == std::errc::no_such_file_or_directory) {
        // File will be created automatically, this shouldn't happen
    } else if (ec.value() == EACCES) {
        std::cerr << "Permission denied" << std::endl;
    } else {
        std::cerr << "Open failed: " << ec.message() << std::endl;
    }
}
```

### Record Size Mismatch

```cpp
// File contains records of different size
std::error_code ec;
repo.save(record, ec);

if (ec == std::errc::invalid_argument) {
    std::cerr << "Record size mismatch - file may be corrupt" << std::endl;
}
```

### Corrupt Data

```cpp
auto record = repo.findById("001", ec);

if (ec) {
    std::cerr << "Read failed: " << ec.message() << std::endl;
    // May indicate corrupt file
}

if (!record) {
    // Record not found (not an error)
}
```

## Exception-Free Design

FdFileLib doesn't throw exceptions from repository operations:

```cpp
// Always returns, never throws
bool success = repo.save(record, ec);

// Check ec for errors
if (!success || ec) {
    // Handle error
}
```

### Internal Exceptions

`FieldMeta::set()` may throw `std::runtime_error` for invalid numeric format:

```cpp
try {
    record.deserialize(buf, ec);
} catch (const std::runtime_error& e) {
    // Invalid numeric field format
    std::cerr << "Parse error: " << e.what() << std::endl;
}
```

## Error Checking Patterns

### Simple Check

```cpp
if (ec) {
    // Error occurred
}
```

### Detailed Check

```cpp
if (ec) {
    std::cerr << "Error code: " << ec.value() << std::endl;
    std::cerr << "Message: " << ec.message() << std::endl;
    std::cerr << "Category: " << ec.category().name() << std::endl;
}
```

### Return Value Check

```cpp
if (!repo.save(record, ec) || ec) {
    // Save failed
}

auto found = repo.findById("001", ec);
if (!found) {
    if (ec) {
        // Read error
    } else {
        // Record not found (not an error)
    }
}
```

## Macro for Error Checking

Useful pattern for examples and tests:

```cpp
#define CHECK_EC(ec, context)                                    \
    if (ec) {                                                    \
        std::cerr << context << ": " << ec.message() << std::endl; \
        return false;                                            \
    }

bool doSomething() {
    std::error_code ec;
    
    repo.save(record, ec);
    CHECK_EC(ec, "Save record");
    
    auto found = repo.findById("001", ec);
    CHECK_EC(ec, "Find record");
    
    return true;
}
```

## Best Practices

### 1. Always Check After Operations

```cpp
repo.save(record, ec);
if (ec) { /* handle */ }

auto found = repo.findById(id, ec);
if (ec) { /* handle */ }
```

### 2. Clear Error Code Before Reuse

```cpp
std::error_code ec;

repo.save(r1, ec);
if (ec) { /* handle */ }

ec.clear();  // Optional - operations clear ec internally

repo.save(r2, ec);
if (ec) { /* handle */ }
```

### 3. Log Meaningful Context

```cpp
if (ec) {
    logger.error("Failed to save user {} to {}: {}", 
                 user.getId(), 
                 filePath, 
                 ec.message());
}
```

### 4. Handle vs Propagate

```cpp
// Handle locally
bool saveUser(const User& user) {
    std::error_code ec;
    repo.save(user, ec);
    if (ec) {
        logger.warn("Save failed, will retry");
        return false;
    }
    return true;
}

// Propagate to caller
bool saveUser(const User& user, std::error_code& ec) {
    return repo.save(user, ec);
}
```
