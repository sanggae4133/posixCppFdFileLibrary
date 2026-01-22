# Repository Pattern

FdFileLib implements the Repository pattern for data access abstraction.

## Interface

Both fixed and variable repositories implement `RecordRepository<T>`:

```cpp
template <typename T>
class RecordRepository {
public:
    virtual bool save(const T& record, std::error_code& ec) = 0;
    virtual bool saveAll(const std::vector<const T*>& records, std::error_code& ec) = 0;
    virtual std::vector<std::unique_ptr<T>> findAll(std::error_code& ec) = 0;
    virtual std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) = 0;
    virtual bool deleteById(const std::string& id, std::error_code& ec) = 0;
    virtual bool deleteAll(std::error_code& ec) = 0;
    virtual size_t count(std::error_code& ec) = 0;
    virtual bool existsById(const std::string& id, std::error_code& ec) = 0;
};
```

## Operations

### Save (Upsert)

Insert or update based on ID:

```cpp
User alice("Alice", 30, "001");
repo.save(alice, ec);  // Insert

alice.age = 31;
repo.save(alice, ec);  // Update (same ID)
```

### Find by ID

```cpp
auto found = repo.findById("001", ec);
if (found) {
    std::cout << found->name << std::endl;
}
```

### Find All

```cpp
auto all = repo.findAll(ec);
for (const auto& record : all) {
    std::cout << record->getId() << std::endl;
}
```

### Exists by ID

```cpp
if (repo.existsById("001", ec)) {
    std::cout << "User exists" << std::endl;
}
```

### Delete by ID

```cpp
repo.deleteById("001", ec);
```

### Delete All

```cpp
repo.deleteAll(ec);
```

### Count

```cpp
size_t total = repo.count(ec);
```

### Save All (Batch)

```cpp
User alice("Alice", 30, "001");
User bob("Bob", 25, "002");

std::vector<const User*> users = {&alice, &bob};
repo.saveAll(users, ec);
```

## Error Handling

All operations use `std::error_code`:

```cpp
std::error_code ec;

repo.save(record, ec);
if (ec) {
    std::cerr << "Save failed: " << ec.message() << std::endl;
    // Handle error
}
```

Common error conditions:
- File I/O errors
- Invalid record size
- Corrupt data
- Lock acquisition failure

## Concurrency

### File Locking

Operations use fcntl file locks:

| Operation | Lock Type |
|-----------|-----------|
| `save()` | Exclusive |
| `saveAll()` | Exclusive |
| `findById()` | Shared |
| `findAll()` | Shared |
| `deleteById()` | Exclusive |
| `deleteAll()` | Exclusive |
| `count()` | Shared |
| `existsById()` | Shared |

### Multi-Process Safety

```cpp
// Process A
FdFile::UniformFixedRepositoryImpl<User> repoA("users.db", ec);
repoA.save(alice, ec);  // Gets exclusive lock

// Process B (concurrent)
FdFile::UniformFixedRepositoryImpl<User> repoB("users.db", ec);
repoB.save(bob, ec);    // Waits for lock, then writes
```

## Caching

### Fixed Repository Cache

The fixed repository maintains an ID cache:

```cpp
// First call: builds cache
auto found1 = repo.findById("001", ec);  // O(N) cache build

// Subsequent calls: cache hit
auto found2 = repo.findById("002", ec);  // O(1) lookup
```

### Cache Invalidation

Cache is automatically refreshed when:

1. File mtime changes (external modification)
2. File size changes
3. Delete operations (index shift)

### Variable Repository Cache

Variable repositories also cache loaded records:

```cpp
// First findAll: loads from file
auto all1 = repo.findAll(ec);

// Subsequent findAll: returns cached data
auto all2 = repo.findAll(ec);
```

## Type-Specific Queries (Variable Records)

```cpp
// Find all records of type
template <typename SubT>
std::vector<std::unique_ptr<SubT>> findAllByType(std::error_code& ec);

// Find by ID with type check
template <typename SubT>
std::unique_ptr<SubT> findByIdAndType(const std::string& id, std::error_code& ec);
```

Example:

```cpp
auto users = repo.findAllByType<User>(ec);
auto config = repo.findByIdAndType<Config>("db.host", ec);
```

## Performance Comparison

| Operation | Fixed | Variable |
|-----------|-------|----------|
| `save()` insert | O(1) | O(1) append |
| `save()` update | O(1) | O(N) rewrite |
| `findById()` | O(1) | O(N) |
| `findAll()` | O(N) | O(N) |
| `deleteById()` | O(N) shift | O(N) rewrite |
| `count()` | O(1) | O(1) cached |

## Best Practices

### 1. Handle Errors

```cpp
if (!repo.save(record, ec)) {
    // Log and handle error
    logger.error("Save failed: {}", ec.message());
}
```

### 2. Batch Operations

```cpp
// Better: One call
std::vector<const User*> batch = {&u1, &u2, &u3};
repo.saveAll(batch, ec);

// Worse: Multiple calls
repo.save(u1, ec);
repo.save(u2, ec);
repo.save(u3, ec);
```

### 3. Check Existence Before Delete

```cpp
// Only delete if exists
if (repo.existsById(id, ec)) {
    repo.deleteById(id, ec);
}
```

## Next Steps

- [Fixed Records](fixed-records.md) - Binary record details
- [Variable Records](variable-records.md) - Text record details
- [API Reference](api-reference.md) - Complete API documentation
