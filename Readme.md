# PosixCppFdFileLibrary (FdFileLib)

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

## Project Structure

```
posixCppFdFileLibrary/
├── CMakeLists.txt              # Root CMake (includes GoogleTest v1.14)
├── compile.sh                  # Build script
├── run.sh                      # Run example
│
├── fdFileLib/                  # Core Library
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
    ├── unit/                   # 단위 테스트 (개별 클래스)
    │   ├── UniqueFdTest.cpp        # 9 tests
    │   ├── MmapGuardTest.cpp       # 8 tests
    │   ├── FileLockGuardTest.cpp   # 9 tests
    │   ├── FieldMetaTest.cpp       # 19 tests
    │   └── UtilTest.cpp            # 17 tests
    ├── scenario/               # 시나리오 테스트 (기능 흐름)
    │   ├── FixedRecordScenarioTest.cpp   # 8 tests
    │   └── VariableRecordScenarioTest.cpp # 8 tests
    ├── integration/            # 통합 테스트 (동시성, 권한, 손상)
    │   ├── ConcurrencyTest.cpp     # 4 tests
    │   └── FilePermissionTest.cpp  # 5 tests
    ├── FixedRecordTest.cpp     # 38 tests (CRUD + corruption)
    └── VariableRecordTest.cpp  # 30 tests (CRUD + corruption)
```

---

## Getting Started

### Prerequisites
- **C++17** compiler (GCC 7+, Clang 5+, Apple Clang 10+)
- **CMake** 3.16+
- **POSIX** environment (Linux/macOS)

### Build

```bash
git clone <repo-url>
cd posixCppFdFileLibrary

mkdir build && cd build
cmake ..
cmake --build . -j

# Or use the script
./compile.sh
```

### Run Tests

```bash
cd build && ctest --output-on-failure

# Run specific suite
./fdfile_tests --gtest_filter="Concurrency*"
./fdfile_tests --gtest_filter="unit/*"
./fdfile_tests --gtest_filter="scenario/*"
```

---

# [English] Complete Usage Guide

## 1. Fixed-Length Records (Recommended)

Fixed-length records are stored in binary format with `mmap` for maximum performance.

### Key Features

| Feature | Implementation |
|---------|----------------|
| **O(1) Lookup** | Internal `unordered_map<id, index>` cache |
| **External Mod Detection** | `fstat()` checks mtime/size on every operation |
| **File Locking** | Shared lock (read), Exclusive lock (write) |
| **Auto Cache Rebuild** | On external file change, cache auto-refreshes |

### Step 1: Define Your Record

```cpp
#pragma once
#include "record/FieldMeta.hpp"
#include "record/FixedRecordBase.hpp"
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

### Step 2: Use Repository

```cpp
#include "repository/UniformFixedRepositoryImpl.hpp"

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

## 2. Variable-Length Records

JSON-like format for flexibility. Uses virtual polymorphism.

```cpp
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

# [Korean] 전체 사용 가이드

## 1. 고정 길이 레코드 (권장)

고정 길이 레코드는 `mmap`을 사용한 바이너리 형식으로 최대 성능을 제공합니다.

### 주요 기능

| 기능 | 구현 |
|------|------|
| **O(1) 조회** | 내부 `unordered_map<id, index>` 캐시 |
| **외부 수정 감지** | 모든 작업 전 `fstat()`으로 mtime/size 확인 |
| **파일 잠금** | 읽기(Shared lock), 쓰기(Exclusive lock) |
| **자동 캐시 리빌드** | 외부 파일 변경 시 캐시 자동 갱신 |

### 단계 1: 레코드 정의

```cpp
class User : public FdFile::FixedRecordBase<User> {
public:
    char name[20];    // 20바이트 고정
    int64_t age;      // 20자리 (+/- + 19자리)

private:
    auto fields() const {
        return std::make_tuple(FD_STR(name), FD_NUM(age));
    }

    void initMembers() {
        std::memset(name, 0, sizeof(name));
        age = 0;
    }

    FD_RECORD_IMPL(User, "User", 10, 10)
};
```

### 단계 2: 리포지토리 사용

```cpp
std::error_code ec;
FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);

// CRUD 작업
repo.save(User("Alice", 30, "001"), ec);  // 삽입/수정
auto found = repo.findById("001", ec);     // O(1) 조회
repo.deleteById("001", ec);                // 삭제
```

---

## Testing (테스트)

### Test Categories (테스트 분류)

| Category | Directory | Tests | Description |
|----------|-----------|-------|-------------|
| **Unit** | `tests/unit/` | 62 | 개별 클래스/함수 |
| **Scenario** | `tests/scenario/` | 16 | 기능 흐름 |
| **Integration** | `tests/integration/` | 9 | 동시성, 권한, 손상 |
| **Corruption** | `tests/*.cpp` | 62 | 파일 손상 시나리오 |

### Unit Tests

| Test File | Tests | Description |
|-----------|-------|-------------|
| `UniqueFdTest.cpp` | 9 | FD RAII wrapper |
| `MmapGuardTest.cpp` | 8 | mmap RAII wrapper |
| `FileLockGuardTest.cpp` | 9 | fcntl lock wrapper |
| `FieldMetaTest.cpp` | 19 | Field 직렬화/역직렬화 |
| `UtilTest.cpp` | 17 | 텍스트 포맷 유틸리티 |

### Run Tests

```bash
# 전체 테스트 실행
cd build && ctest --output-on-failure

# 특정 테스트만
./tests/fdfile_tests --gtest_filter="FieldMeta*"
./tests/fdfile_tests --gtest_filter="*Corruption*"

# 결과: 100% tests passed, 0 tests failed out of 149
```

---

## Architecture (아키텍처)

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

## License

MIT License
