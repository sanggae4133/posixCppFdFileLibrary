# Variable-Length Records

Variable-length records provide flexibility through JSON-like text storage.

## Overview

| Feature | Description |
|---------|-------------|
| Storage Format | Text (JSON-like) |
| Lookup Speed | O(N) |
| Polymorphism | Virtual (runtime) |
| Use Case | Flexible data, schema evolution |

## Defining a Record

### Basic Structure

```cpp
#include <fdfile/fdfile.hpp>

class Config : public FdFile::VariableRecordBase {
public:
    std::string key;
    std::string value;
    long version = 0;

    Config() = default;
    Config(std::string k, std::string v, long ver)
        : key(std::move(k)), value(std::move(v)), version(ver) {}

    // Required: Return unique ID
    std::string id() const override { return key; }

    // Required: Return type name
    const char* typeName() const override { return "Config"; }

    // Required: Clone for copying
    std::unique_ptr<FdFile::RecordBase> clone() const override {
        return std::make_unique<Config>(*this);
    }

    // Required: Serialize to key-value pairs
    void toKv(std::vector<std::pair<std::string, 
              std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"key", {true, key}});           // String
        out.push_back({"value", {true, value}});       // String
        out.push_back({"version", {false, std::to_string(version)}}); // Number
    }

    // Required: Deserialize from key-value pairs
    bool fromKv(const std::unordered_map<std::string, 
                std::pair<bool, std::string>>& kv,
                std::error_code& ec) override {
        ec.clear();
        
        auto k = kv.find("key");
        auto v = kv.find("value");
        auto ver = kv.find("version");
        
        if (k == kv.end() || v == kv.end() || ver == kv.end())
            return false;

        key = k->second.second;
        value = v->second.second;
        
        long tmp = 0;
        if (!FdFile::util::parseLongStrict(ver->second.second, tmp, ec))
            return false;
        version = tmp;
        
        return true;
    }
};
```

### File Format

Records are stored as one JSON-like line per record:

```
Config { "key": "database.host", "value": "localhost", "version": 1 }
Config { "key": "database.port", "value": "5432", "version": 1 }
Setting { "name": "theme", "enabled": 1 }
```

## Using the Repository

### Setup with Prototypes

Variable records require prototype instances for type reconstruction:

```cpp
std::error_code ec;

// Create prototypes for each record type
std::vector<std::unique_ptr<FdFile::VariableRecordBase>> prototypes;
prototypes.push_back(std::make_unique<Config>());
prototypes.push_back(std::make_unique<Setting>());

// Create repository
FdFile::VariableFileRepositoryImpl repo("config.txt", std::move(prototypes), ec);
```

### Basic CRUD

```cpp
// Create
Config dbHost("database.host", "localhost", 1);
repo.save(dbHost, ec);

// Read by ID
auto found = repo.findById("database.host", ec);
if (found) {
    auto* config = dynamic_cast<Config*>(found.get());
    if (config) {
        std::cout << config->value << std::endl;
    }
}

// Read all
auto all = repo.findAll(ec);

// Update (same ID)
dbHost.value = "192.168.1.100";
dbHost.version = 2;
repo.save(dbHost, ec);

// Delete
repo.deleteById("database.host", ec);
```

### Type-Specific Queries

```cpp
// Find all records of a specific type
auto configs = repo.findAllByType<Config>(ec);

// Find by ID with type check
auto found = repo.findByIdAndType<Config>("database.host", ec);
```

## Key-Value Serialization

### Format

```cpp
// toKv output: vector of {key, {isString, value}}
out.push_back({"name", {true, "Alice"}});   // String: "name": "Alice"
out.push_back({"age", {false, "30"}});      // Number: "age": 30
```

### String Escaping

Strings are automatically escaped:

| Character | Escaped |
|-----------|---------|
| `"` | `\"` |
| `\` | `\\` |
| Newline | `\n` |
| Tab | `\t` |

## Multiple Record Types

You can store different record types in the same file:

```cpp
class User : public FdFile::VariableRecordBase {
    // ... user fields
    const char* typeName() const override { return "User"; }
};

class Product : public FdFile::VariableRecordBase {
    // ... product fields
    const char* typeName() const override { return "Product"; }
};

// Setup
std::vector<std::unique_ptr<FdFile::VariableRecordBase>> prototypes;
prototypes.push_back(std::make_unique<User>());
prototypes.push_back(std::make_unique<Product>());

FdFile::VariableFileRepositoryImpl repo("data.txt", std::move(prototypes), ec);

// All types stored in same file
repo.save(User("alice", 1), ec);
repo.save(Product("laptop", 1500), ec);

// Type-specific queries
auto users = repo.findAllByType<User>(ec);
auto products = repo.findAllByType<Product>(ec);
```

## Comparison with Fixed Records

| Aspect | Fixed | Variable |
|--------|-------|----------|
| Storage | Binary | Text |
| Lookup | O(1) | O(N) |
| Flexibility | Low | High |
| Schema Changes | Requires migration | Easy |
| Human Readable | No | Yes |
| Polymorphism | CRTP (compile-time) | Virtual (runtime) |

### When to Use Variable Records

- Schema may evolve over time
- Human-readable storage preferred
- Multiple record types in one file
- Flexibility over performance

### When to Use Fixed Records

- High-performance requirements
- Stable schema
- Single record type per file
- Memory efficiency important

## Next Steps

- [Fixed Records](fixed-records.md) - For high-performance needs
- [Repository Pattern](repository-pattern.md) - Advanced operations
- [API Reference](api-reference.md) - Complete documentation
