# Getting Started

This guide will help you get FdFileLib up and running in your project.

## Requirements

| Requirement | Version |
|-------------|---------|
| C++ Standard | C++17 |
| Compiler | GCC 7+, Clang 5+, Apple Clang 10+ |
| CMake | 3.16+ |
| Platform | POSIX (Linux/macOS) |

## Installation

### Method 1: CMake FetchContent (Recommended)

Add to your `CMakeLists.txt`:

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

### Method 2: Subdirectory

Clone or copy the library to your project:

```cmake
add_subdirectory(external/fdfilelib)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

### Method 3: System Installation

Build and install:

```bash
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

Then use in your project:

```cmake
find_package(fdfile REQUIRED)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

## First Example

### 1. Define a Record

```cpp
#include <fdfile/fdfile.hpp>
#include <cstring>

class User : public FdFile::FixedRecordBase<User> {
public:
    char name[20];
    int64_t age;

private:
    auto fields() const {
        return std::make_tuple(FD_STR(name), FD_NUM(age));
    }

    void initMembers() {
        std::memset(name, 0, sizeof(name));
        age = 0;
    }

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

### 2. Use the Repository

```cpp
#include <fdfile/fdfile.hpp>
#include <iostream>

int main() {
    std::error_code ec;
    
    // Create repository
    FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);
    if (ec) {
        std::cerr << "Failed to open: " << ec.message() << std::endl;
        return 1;
    }
    
    // Create
    User alice("Alice", 30, "001");
    repo.save(alice, ec);
    
    // Read
    auto found = repo.findById("001", ec);
    if (found) {
        std::cout << "Found: " << found->name << ", " << found->age << std::endl;
    }
    
    // Update
    alice.age = 31;
    repo.save(alice, ec);  // Same ID = update
    
    // Delete
    repo.deleteById("001", ec);
    
    return 0;
}
```

### 3. Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
./your_app
```

## Next Steps

- [Fixed Records](fixed-records.md) - Learn about high-performance binary records
- [Variable Records](variable-records.md) - Learn about flexible JSON-like records
- [Repository Pattern](repository-pattern.md) - Deep dive into CRUD operations
- [API Reference](api-reference.md) - Complete API documentation
