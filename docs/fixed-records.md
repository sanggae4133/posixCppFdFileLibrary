# Fixed-Length Records

Fixed-length records provide maximum performance through binary storage and memory mapping.

## Overview

| Feature | Description |
|---------|-------------|
| Storage Format | Binary (mmap) |
| Lookup Speed | O(1) via ID cache |
| Vtable Overhead | Zero (CRTP) |
| Use Case | High-performance, structured data |

## Defining a Record

### Basic Structure

```cpp
#include <fdfile/fdfile.hpp>
#include <cstring>

class Product : public FdFile::FixedRecordBase<Product> {
public:
    // Public data members
    char name[30];    // Fixed 30 bytes
    int64_t price;    // Fixed 20 bytes (sign + 19 digits)
    int64_t stock;

private:
    // Required: Field metadata tuple
    auto fields() const {
        return std::make_tuple(
            FD_STR(name),    // String field
            FD_NUM(price),   // Numeric field
            FD_NUM(stock)
        );
    }

    // Required: Initialize members
    void initMembers() {
        std::memset(name, 0, sizeof(name));
        price = 0;
        stock = 0;
    }

    // Required: Generate common methods
    // Args: ClassName, TypeName, TypeFieldLen, IdFieldLen
    FD_RECORD_IMPL(Product, "Product", 10, 10)

public:
    // Custom constructor
    Product(const char* n, int64_t p, int64_t s, const char* id) {
        initMembers();
        if (n) std::strncpy(name, n, sizeof(name));
        price = p;
        stock = s;
        setId(id);
        defineLayout();
    }
};
```

### Field Types

| Macro | Type | Length | Description |
|-------|------|--------|-------------|
| `FD_STR(member)` | `char[N]` | N bytes | Fixed-length string |
| `FD_NUM(member)` | `int64_t` | 20 bytes | Signed 64-bit integer |

### Record Layout

The record is serialized in this format:

```
[Type][,id:"][ID]["]{[field1]:[value1],[field2]:[value2],...}
```

Example:
```
Product   ,id:"P001      "{name:"Laptop              ",price:+0000000000001500000,stock:+0000000000000000050}
```

## Using the Repository

### Basic CRUD

```cpp
std::error_code ec;
FdFile::UniformFixedRepositoryImpl<Product> repo("products.db", ec);

// Create
Product laptop("Laptop", 1500000, 50, "P001");
repo.save(laptop, ec);

// Read by ID (O(1))
auto found = repo.findById("P001", ec);

// Read all
auto all = repo.findAll(ec);

// Update (same ID)
laptop.stock = 45;
repo.save(laptop, ec);

// Delete
repo.deleteById("P001", ec);

// Count
size_t count = repo.count(ec);
```

### Performance Characteristics

| Operation | Complexity | Notes |
|-----------|------------|-------|
| `save()` (insert) | O(1) | Append + cache update |
| `save()` (update) | O(1) | In-place write |
| `findById()` | O(1) | Cache lookup + mmap read |
| `findAll()` | O(N) | Sequential read |
| `deleteById()` | O(N) | Data shift required |
| `count()` | O(1) | Size calculation |
| `existsById()` | O(1) | Cache lookup |

## ID Cache

The repository maintains an internal cache for O(1) lookups:

```
findById("P001")
    ↓
checkAndRefreshCache()
    ├── fstat() → mtime changed?
    │       ├── Yes → remapFile() + rebuildCache()
    │       └── No  → use existing cache
    ↓
idCache_.find("P001") → O(1) lookup
    ↓
return record at index
```

### External Modification Detection

When another process modifies the file:

1. `fstat()` detects mtime/size change
2. Cache is invalidated
3. File is remapped
4. Cache is rebuilt

## Best Practices

### 1. Choose Appropriate Field Sizes

```cpp
// Good: Sized for actual data
char name[50];      // Enough for most names

// Bad: Wasteful
char name[1000];    // Too large for typical names
```

### 2. Use Meaningful Type and ID Lengths

```cpp
// FD_RECORD_IMPL(ClassName, TypeName, TypeLen, IdLen)
FD_RECORD_IMPL(User, "User", 10, 10)  // 10 bytes each

// For longer IDs (e.g., UUIDs)
FD_RECORD_IMPL(Order, "Order", 10, 36)  // UUID = 36 chars
```

### 3. Initialize Members

Always implement `initMembers()` to prevent garbage data:

```cpp
void initMembers() {
    std::memset(name, 0, sizeof(name));  // Clear string
    age = 0;                              // Initialize number
}
```

## CRTP Pattern

FdFileLib uses CRTP (Curiously Recurring Template Pattern) for zero-overhead polymorphism:

```
FixedRecordBase<Derived>   ← Template base
        ↑
        │ static_cast<Derived*>(this)
        │
      Product              ← Concrete type
```

Benefits:
- No virtual function table overhead
- Compile-time polymorphism
- Inline-able method calls

## Next Steps

- [Variable Records](variable-records.md) - For flexible, schema-evolving data
- [Repository Pattern](repository-pattern.md) - Advanced repository usage
- [API Reference](api-reference.md) - Complete class documentation
