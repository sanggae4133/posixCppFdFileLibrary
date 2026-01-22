# API Reference

Complete API documentation for FdFileLib.

## Core Components

### `FdFile::FixedRecordBase<Derived>`

Template base class for fixed-length records using CRTP.

```cpp
template <typename Derived>
class FixedRecordBase {
public:
    size_t recordSize() const;
    bool serialize(char* buf) const;
    bool deserialize(const char* buf, std::error_code& ec);

protected:
    void defineStart();
    void defineType(size_t len);
    void defineId(size_t len);
    void defineField(const char* key, size_t valLen, bool isString);
    void defineEnd();
};
```

#### Methods

| Method | Description |
|--------|-------------|
| `recordSize()` | Returns total record size in bytes |
| `serialize(buf)` | Writes record to buffer |
| `deserialize(buf, ec)` | Reads record from buffer |

### `FdFile::VariableRecordBase`

Virtual base class for variable-length records.

```cpp
class VariableRecordBase : public RecordBase {
public:
    virtual void toKv(std::vector<std::pair<std::string, 
                      std::pair<bool, std::string>>>& out) const = 0;
    virtual bool fromKv(const std::unordered_map<std::string, 
                        std::pair<bool, std::string>>& kv,
                        std::error_code& ec) = 0;
    virtual std::unique_ptr<RecordBase> clone() const = 0;
};
```

#### Virtual Methods

| Method | Description |
|--------|-------------|
| `id()` | Returns record's unique identifier |
| `typeName()` | Returns record type name |
| `toKv(out)` | Serializes to key-value pairs |
| `fromKv(kv, ec)` | Deserializes from key-value pairs |
| `clone()` | Creates a deep copy |

---

## Repositories

### `FdFile::UniformFixedRepositoryImpl<T>`

Repository for fixed-length records.

```cpp
template <typename T>
class UniformFixedRepositoryImpl : public RecordRepository<T> {
public:
    UniformFixedRepositoryImpl(const std::string& path, std::error_code& ec);
    
    bool save(const T& record, std::error_code& ec) override;
    bool saveAll(const std::vector<const T*>& records, std::error_code& ec) override;
    std::vector<std::unique_ptr<T>> findAll(std::error_code& ec) override;
    std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) override;
    bool deleteById(const std::string& id, std::error_code& ec) override;
    bool deleteAll(std::error_code& ec) override;
    size_t count(std::error_code& ec) override;
    bool existsById(const std::string& id, std::error_code& ec) override;
};
```

#### Constructor

```cpp
UniformFixedRepositoryImpl(const std::string& path, std::error_code& ec);
```

| Parameter | Description |
|-----------|-------------|
| `path` | File path for storage |
| `ec` | Error code (set on failure) |

### `FdFile::VariableFileRepositoryImpl`

Repository for variable-length records.

```cpp
class VariableFileRepositoryImpl : public RecordRepository<VariableRecordBase> {
public:
    VariableFileRepositoryImpl(
        const std::string& path,
        std::vector<std::unique_ptr<VariableRecordBase>> prototypes,
        std::error_code& ec);
    
    // ... same interface as UniformFixedRepositoryImpl
};
```

#### Constructor

```cpp
VariableFileRepositoryImpl(
    const std::string& path,
    std::vector<std::unique_ptr<VariableRecordBase>> prototypes,
    std::error_code& ec);
```

| Parameter | Description |
|-----------|-------------|
| `path` | File path for storage |
| `prototypes` | Prototype instances for each record type |
| `ec` | Error code (set on failure) |

---

## Macros

### `FD_STR(member)`

Declares a string field (char array).

```cpp
char name[20];
auto fields() const {
    return std::make_tuple(FD_STR(name));  // 20 bytes
}
```

### `FD_NUM(member)`

Declares a numeric field (int64_t).

```cpp
int64_t age;
auto fields() const {
    return std::make_tuple(FD_NUM(age));  // 20 bytes
}
```

### `FD_RECORD_IMPL(ClassName, TypeName, TypeLen, IdLen)`

Generates common record methods.

```cpp
FD_RECORD_IMPL(User, "User", 10, 10)
```

| Parameter | Description |
|-----------|-------------|
| `ClassName` | Class name |
| `TypeName` | Type string (stored in file) |
| `TypeLen` | Type field length |
| `IdLen` | ID field length |

Generated methods:
- Default constructor
- `typeName()`
- `getId()`
- `setId()`
- `__getFieldValue()`
- `__setFieldValue()`

---

## Utility Classes

### `FdFile::detail::UniqueFd`

RAII wrapper for POSIX file descriptors.

```cpp
class UniqueFd {
public:
    UniqueFd() noexcept;
    explicit UniqueFd(int fd) noexcept;
    
    int get() const noexcept;
    bool valid() const noexcept;
    explicit operator bool() const noexcept;
    int release() noexcept;
    void reset(int newFd = -1) noexcept;
};
```

### `FdFile::detail::MmapGuard`

RAII wrapper for mmap.

```cpp
class MmapGuard {
public:
    MmapGuard();
    MmapGuard(void* ptr, size_t size);
    
    void* get() const noexcept;
    char* data() const noexcept;
    size_t size() const noexcept;
    bool valid() const noexcept;
    
    void reset() noexcept;
    void reset(void* ptr, size_t size) noexcept;
    bool sync(bool async = false) noexcept;
};
```

### `FdFile::detail::FileLockGuard`

RAII guard for fcntl file locking.

```cpp
class FileLockGuard {
public:
    enum class Mode { Shared, Exclusive };
    
    FileLockGuard();
    FileLockGuard(int fd, Mode mode, std::error_code& ec);
    
    bool lock(int fd, Mode mode, std::error_code& ec);
    void unlockIgnore() noexcept;
    bool locked() const noexcept;
};
```

---

## Version Constants

```cpp
namespace FdFile {
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;
    constexpr const char* VERSION_STRING = "1.0.0";
}
```

---

## Error Handling

All operations use `std::error_code`:

```cpp
std::error_code ec;
repo.save(record, ec);

if (ec) {
    // ec.value() - error code
    // ec.message() - human-readable message
    // ec.category() - error category
}
```

Common error codes:
- `std::errc::invalid_argument` - Invalid parameter
- `std::errc::bad_file_descriptor` - Invalid file
- `std::errc::result_out_of_range` - Value out of range
- System errno values for I/O errors
