# POSIX C++ File Descriptor File Library

A C++ library implementing a **JPA-style Repository pattern** for storing and reading records in text files using POSIX file descriptors.

## Features

- **JPA-style Repository API**: `save()`, `findAll()`, `findById()`, `deleteById()` 등 친숙한 CRUD 인터페이스
- **POSIX File Descriptor I/O**: 저수준 파일 시스템 API를 직접 사용
- **File Locking**: `fcntl`을 사용한 공유/배타 잠금으로 동시성 제어
- **Multi-type Records**: 하나의 파일에 여러 타입의 레코드 저장
- **JSON-like Format**: 사람이 읽고 편집할 수 있는 텍스트 형식
- **Type-safe Queries**: `findByIdAndType<T>()`, `findAllByType<T>()` 지원

## File Structure

```
posixCppFdFileLibrary/
├── fdFileLib/                    # Core library
│   ├── RecordRepository.hpp      # Repository 인터페이스 (JPA-style)
│   ├── FdTextFile.hpp/cpp        # Repository 구현체
│   ├── TextRecordBase.hpp        # 레코드 베이스 클래스
│   ├── textFormatUtil.hpp        # 텍스트 포맷 유틸리티
│   └── detail/                   # 내부 구현
│       ├── FileLockGuard.hpp
│       └── UniqueFd.hpp
├── examples/
│   ├── main.cpp                  # CLI 예제 프로그램
│   └── records/
│       ├── A.hpp                 # 예제 레코드 타입 A
│       └── B.hpp                 # 예제 레코드 타입 B
├── build.sh                      # 빌드 스크립트
├── run.sh                        # 실행 스크립트
└── CMakeLists.txt
```

## Quick Start

### 1. Define Record Class

```cpp
class User : public FdFile::TextRecordBase {
public:
    std::string name;
    long userId = 0;

    User() = default;
    User(std::string name, long id) : name(std::move(name)), userId(id) {}

    // Required overrides
    std::string id() const override { return std::to_string(userId); }
    const char* typeName() const override { return "User"; }
    std::unique_ptr<TextRecordBase> clone() const override {
        return std::make_unique<User>(*this);
    }

    void toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"name", {true, name}});
        out.push_back({"id", {false, std::to_string(userId)}});
    }

    bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                std::error_code& ec) override {
        // Parse from key-value map
        name = kv.at("name").second;
        userId = std::stol(kv.at("id").second);
        return true;
    }
};
```

### 2. Create Repository

```cpp
std::error_code ec;

// Register prototypes
std::vector<std::unique_ptr<FdFile::TextRecordBase>> protos;
protos.push_back(std::make_unique<User>());
protos.push_back(std::make_unique<Order>());

// Create repository
FdFile::FdTextFile repo("data.txt", std::move(protos), ec);
```

### 3. CRUD Operations

```cpp
// Create / Update
User user("john", 123);
repo.save(user, ec);

// Read all
auto all = repo.findAll(ec);

// Read by ID
auto found = repo.findById("123", ec);

// Read by ID with type check
auto user = repo.findByIdAndType<User>("123", ec);

// Read all by type
auto allUsers = repo.findAllByType<User>(ec);

// Delete
repo.deleteById("123", ec);

// Utilities
size_t count = repo.count(ec);
bool exists = repo.existsById("123", ec);
```

## Repository API

| Method | Description |
|--------|-------------|
| `save(record, ec)` | 저장 (id 존재 시 업데이트) |
| `saveAll(records, ec)` | 여러 레코드 일괄 저장 |
| `findAll(ec)` | 모든 레코드 조회 |
| `findById(id, ec)` | ID로 조회 |
| `findByIdAndType<T>(id, ec)` | ID + 타입으로 조회 |
| `findAllByType<T>(ec)` | 특정 타입만 조회 |
| `deleteById(id, ec)` | ID로 삭제 |
| `deleteAll(ec)` | 전체 삭제 |
| `count(ec)` | 레코드 수 |
| `existsById(id, ec)` | 존재 여부 확인 |

## File Format

```
TypeName { "key1": "value1", "key2": 123 }
```

Example:
```
User { "name": "john", "id": 123 }
Order { "item": "book", "price": 1500 }
```

- 빈 줄과 `#` 주석은 무시됨
- 레코드 뒤에 인라인 주석 가능

## Build & Run

```bash
# Build
./build.sh
# or
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j

# Run CLI example
./run.sh
# or
./build/fdfile_example
```

## Requirements

- C++17 or higher
- POSIX-compatible system (Linux, macOS)
- CMake 3.16+

## License

See LICENSE file.

---

# POSIX C++ File Descriptor File Library (한국어)

POSIX 파일 디스크립터를 사용하여 텍스트 파일에 레코드를 저장하는 **JPA 스타일 Repository 패턴** C++ 라이브러리입니다.

## 주요 기능

- **JPA 스타일 Repository API**: `save()`, `findAll()`, `findById()`, `deleteById()` 등
- **POSIX 파일 디스크립터 I/O**: 저수준 API 직접 사용
- **파일 잠금**: `fcntl` 기반 공유/배타 잠금
- **다중 타입 레코드**: 한 파일에 여러 타입 저장
- **타입 안전 조회**: `findByIdAndType<T>()`, `findAllByType<T>()`

## 사용 예시

```cpp
// Repository 생성
std::vector<std::unique_ptr<FdFile::TextRecordBase>> protos;
protos.push_back(std::make_unique<A>());
protos.push_back(std::make_unique<B>());

FdFile::FdTextFile repo("data.txt", std::move(protos), ec);

// 저장
A a("john", 123);
repo.save(a, ec);

// 조회
auto found = repo.findByIdAndType<A>("123", ec);
if (found) {
    std::cout << found->name << "\n";
}

// 삭제
repo.deleteById("123", ec);
```

## 빌드

```bash
./build.sh
./run.sh
```

## 라이선스

LICENSE 파일 참조.
