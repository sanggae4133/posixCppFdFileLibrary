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
| **48 Unit Tests** | Comprehensive GoogleTest coverage |

## Project Structure

```
posixCppFdFileLibrary/
├── CMakeLists.txt                  # Root CMake (includes GoogleTest v1.14)
├── compile.sh                      # Build script
├── run.sh                          # Run example
│
├── fdFileLib/                      # Core Library
│   ├── CMakeLists.txt
│   ├── record/
│   │   ├── RecordBase.hpp              # Base interface
│   │   ├── VariableRecordBase.hpp      # Variable-length (Virtual)
│   │   ├── FixedRecordBase.hpp         # Fixed-length (CRTP Template)
│   │   └── FieldMeta.hpp               # C++17 tuple-based field metadata
│   ├── repository/
│   │   ├── RecordRepository.hpp            # Repository interface
│   │   ├── VariableFileRepositoryImpl.hpp  # Variable-length repo
│   │   ├── VariableFileRepositoryImpl.cpp
│   │   └── UniformFixedRepositoryImpl.hpp  # Fixed-length repo (Header-only)
│   └── util/
│       ├── UniqueFd.hpp            # RAII file descriptor
│       ├── MmapGuard.hpp           # RAII mmap wrapper
│       ├── FileLockGuard.hpp       # RAII fcntl file locking
│       └── textFormatUtil.hpp      # JSON-like parsing/formatting
│
├── examples/                       # Usage Examples
│   ├── main.cpp                    # Comprehensive demo
│   └── records/
│       ├── FixedA.hpp, FixedB.hpp  # Fixed record examples
│       └── A.hpp, B.hpp            # Variable record examples
│
└── tests/                          # GoogleTest Unit Tests (48 tests)
    ├── CMakeLists.txt
    ├── FixedRecordTest.cpp         # 18 tests (incl. external mod)
    ├── VariableRecordTest.cpp      # 11 tests
    └── UtilTest.cpp                # 17 tests
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
./fdfile_tests --gtest_filter="ExternalModificationTest.*"
```

### Run Example

```bash
./run.sh
# or
./build/fdfile_example
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
    int64_t age;      // 19 digits (zero-padded)

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

### External Modification Support

```cpp
// Process A saves a record
repo.save(alice, ec);

// Process B (or vim) modifies the file externally
// ...

// Process A reads - automatically detects change and reloads
auto found = repo.findById("001", ec);  // Cache is auto-rebuilt
```

If external modification corrupts the file (invalid format), an error is returned:
```cpp
auto found = repo.findById("001", ec);
if (ec) {
    // ec contains error code (e.g., invalid_argument for corrupt file)
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
| `FD_NUM(member)` | Numeric field (int64_t, 19 digits) |
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
#pragma once
#include "record/FieldMeta.hpp"
#include "record/FixedRecordBase.hpp"
#include <cstring>

class User : public FdFile::FixedRecordBase<User> {
public:
    char name[20];    // 20바이트 고정
    int64_t age;      // 19자리 (0으로 패딩)

private:
    auto fields() const {
        return std::make_tuple(
            FD_STR(name),  // 문자열 필드 매크로
            FD_NUM(age)    // 숫자 필드 매크로
        );
    }

    void initMembers() {
        std::memset(name, 0, sizeof(name));
        age = 0;
    }

    // 자동 생성: 생성자, getId(), setId(), typeName() 등
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

### 단계 2: 리포지토리 사용

```cpp
#include "repository/UniformFixedRepositoryImpl.hpp"

int main() {
    std::error_code ec;
    FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);
    if (ec) return 1;

    // 삽입 (INSERT)
    User alice("Alice", 30, "001");
    repo.save(alice, ec);

    // 수정 (UPDATE) - 같은 ID
    alice.age = 31;
    repo.save(alice, ec);  // 제자리 업데이트

    // ID로 조회 (O(1) 캐시 조회)
    auto found = repo.findById("001", ec);
    if (found) {
        std::cout << found->name << ", " << found->age << "\n";
    }

    // 기타 작업
    auto all = repo.findAll(ec);           // 전체 조회
    bool exists = repo.existsById("001");  // 존재 확인
    size_t cnt = repo.count(ec);           // 개수
    repo.deleteById("001", ec);            // 하나 삭제
    repo.deleteAll(ec);                    // 전체 삭제
}
```

### 외부 수정 지원

```cpp
// 프로세스 A가 레코드 저장
repo.save(alice, ec);

// 프로세스 B (또는 vim)가 파일 직접 수정
// ...

// 프로세스 A가 읽기 - 변경 자동 감지 후 리로드
auto found = repo.findById("001", ec);  // 캐시 자동 리빌드
```

외부 수정이 파일을 손상시킨 경우 (잘못된 형식), 에러가 반환됩니다:
```cpp
auto found = repo.findById("001", ec);
if (ec) {
    // ec에 에러 코드 포함 (예: invalid_argument - 손상된 파일)
}
```

---

## 2. 가변 길이 레코드

유연성을 위한 JSON 스타일 형식. 가상 다형성 사용.

```cpp
class Config : public FdFile::VariableRecordBase {
public:
    std::string key, value;
    
    std::string id() const override { return key; }
    const char* typeName() const override { return "Config"; }
    std::unique_ptr<RecordBase> clone() const override {
        return std::make_unique<Config>(*this);
    }
    // toKv()와 fromKv() 구현 필요
};

// 프로토타입 인스턴스 필요
std::vector<std::unique_ptr<VariableRecordBase>> protos;
protos.push_back(std::make_unique<Config>());
FdFile::VariableFileRepositoryImpl repo("config.txt", std::move(protos), ec);
```

---

## API 레퍼런스

### `UniformFixedRepositoryImpl<T>`

| 메서드 | 설명 | 복잡도 |
|--------|------|--------|
| `save(record, ec)` | Upsert | O(1) 캐시 + O(1) 쓰기 |
| `findById(id, ec)` | ID로 조회 | **O(1)** (캐시 히트) |
| `findAll(ec)` | 전체 조회 | O(N) |
| `existsById(id, ec)` | 존재 확인 | **O(1)** |
| `deleteById(id, ec)` | 하나 삭제 | O(N) shift |
| `deleteAll(ec)` | 전체 삭제 | O(1) |
| `count(ec)` | 개수 | O(1) |

### 매크로

| 매크로 | 설명 |
|--------|------|
| `FD_STR(member)` | 문자열 필드 (char 배열) |
| `FD_NUM(member)` | 숫자 필드 (int64_t, 19자리) |
| `FD_RECORD_IMPL(Class, Type, TypeLen, IdLen)` | 레코드 메서드 생성 |

---

## Testing (테스트)

### Unit Tests (단위 테스트)

| Test File | Tests | Description |
|-----------|-------|-------------|
| `FixedRecordTest.cpp` | 18 | CRUD + 외부 수정 감지 |
| `VariableRecordTest.cpp` | 11 | 가변 레코드 CRUD |
| `UtilTest.cpp` | 17 | 유틸리티 함수 |

```bash
# 전체 테스트 실행
cd build && ctest --output-on-failure

# 결과: 100% tests passed, 0 tests failed out of 48
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

- No virtual function table (vtable 없음)
- Compile-time polymorphism (컴파일 타임 다형성)

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

MIT License - See [LICENSE](LICENSE)
