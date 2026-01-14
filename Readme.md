# POSIX C++ File Descriptor File Library

A C++ library for storing and reading various types of records in text files using POSIX file descriptors.

## Features

- **POSIX File Descriptor-based I/O**: Direct use of low-level file system APIs for efficient file operations
- **File Locking Support**: Concurrent access control using shared/exclusive locks via `fcntl`
- **Multi-type Record Support**: Store multiple types of records in a single file
- **JSON-like Text Format**: Human-readable and editable text format
- **Record-based Read/Write**: Read specific records by offset
- **Error Handling**: Explicit error handling using `std::error_code`

## File Structure

```
posixCppFdFileLibrary/
├── fdFileLib/
│   ├── FdTextFile.hpp      # Main file class
│   ├── TextRecordBase.hpp  # Record base class
│   └── textFormatUtil.hpp  # Text format utilities
├── records/
│   ├── A.hpp               # Example record type A
│   └── B.hpp               # Example record type B
├── test/
│   └── mixed.txt           # Test data file
├── main.cpp                # Usage example
└── Readme.md
```

## Usage

### 1. Define Record Class

Define a record class by inheriting from `TextRecordBase`:

```cpp
class A : public FdFile::TextRecordBase {
public:
    std::string name;
    long id = 0;

    const char* typeName() const override { return "A"; }

    void toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"name", {true, name}});  // true = string type
        out.push_back({"id", {false, std::to_string(id)}});  // false = numeric type
    }

    bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                 std::error_code& ec) override {
        // Implement parsing logic
        // ...
    }
};
```

### 2. Open File

Open a file by registering type factory functions:

```cpp
std::error_code ec;
FdFile::FdTextFile f(
    "./test/mixed.txt",
    {
        {"A", [](){ return std::make_unique<A>(); }},
        {"B", [](){ return std::make_unique<B>(); }},
    },
    ec
);
if (ec) {
    std::cerr << "open: " << ec.message() << "\n";
    return 1;
}
```

### 3. Write Records

Append records to the end of the file using the `append` method:

```cpp
A a;
a.name = "john";
a.id = 123;

if (!f.append(a, true, ec)) {  // true = perform fsync
    std::cerr << "append: " << ec.message() << "\n";
    return 1;
}
```

### 4. Read Records

Read records from a specific offset using the `readAt` method:

```cpp
off_t offset = 0;
std::unique_ptr<FdFile::TextRecordBase> rec;
off_t nextOffset = 0;

if (!f.readAt(offset, rec, nextOffset, ec)) {
    std::cerr << "read error: " << ec.message() << "\n";
    return 1;
}

// Downcast based on type
if (std::string(rec->typeName()) == "A") {
    auto* a = static_cast<A*>(rec.get());
    std::cout << "A name=" << a->name << " id=" << a->id << "\n";
}
```

## File Format

Each record is stored on a single line following this format:

```
TypeName { "key1": "value1", "key2": 123 }
```

Example:
```
A { "name": "john", "id": 123 }
B { "name": "albert", "id": 1234, "pw": "1234" }
```

### Ignored Lines

- Blank lines are ignored.
- Comment lines are ignored: lines whose first non-space character is `#` are treated as comments.

## Key Features

### File Locking

- **Write**: Uses exclusive lock
- **Read**: Uses shared lock
- Ensures data consistency when multiple processes access the file simultaneously

### Flexible Reading

- Can read normally even if the last line doesn't end with a newline character
- Parsing succeeds if the record is complete
- Returns an error if the format is corrupted or truncated

### Error Handling

All I/O operations return errors via `std::error_code`:

```cpp
std::error_code ec;
if (!f.append(record, true, ec)) {
    if (ec == std::errc::not_supported) {
        // Unsupported type
    } else if (ec == std::errc::io_error) {
        // I/O error
    }
}
```

## Build and Run

```bash
# CMake (recommended)
cmake -S . -B build
cmake --build build -j
./build/fdfile_example

# Or compile directly
g++ -std=c++17 -o main main.cpp -lstdc++fs

# Run
./main
```

## Requirements

- C++17 or higher
- POSIX-compatible system (Linux, macOS, etc.)
- Read/write permissions on the file system

## License

See LICENSE file.

---

# POSIX C++ File Descriptor File Library

POSIX 파일 디스크립터를 사용하여 텍스트 파일에 다양한 타입의 레코드를 저장하고 읽는 C++ 라이브러리입니다.

## 주요 기능

- **POSIX 파일 디스크립터 기반 I/O**: 저수준 파일 시스템 API를 직접 사용하여 효율적인 파일 작업
- **파일 잠금 지원**: `fcntl`을 사용한 공유/배타 잠금으로 동시성 제어
- **다중 타입 레코드 지원**: 하나의 파일에 여러 타입의 레코드를 저장 가능
- **JSON-like 텍스트 포맷**: 사람이 읽고 편집할 수 있는 텍스트 형식
- **레코드 기반 읽기/쓰기**: 오프셋 기반으로 특정 레코드만 읽기 가능
- **에러 처리**: `std::error_code`를 사용한 명시적 에러 처리

## 파일 구조

```
posixCppFdFileLibrary/
├── fdFileLib/
│   ├── FdTextFile.hpp      # 메인 파일 클래스
│   ├── TextRecordBase.hpp  # 레코드 베이스 클래스
│   └── textFormatUtil.hpp  # 텍스트 포맷 유틸리티
├── records/
│   ├── A.hpp               # 예제 레코드 타입 A
│   └── B.hpp               # 예제 레코드 타입 B
├── test/
│   └── mixed.txt           # 테스트 데이터 파일
├── main.cpp                # 사용 예제
└── Readme.md
```

## 사용 방법

### 1. 레코드 클래스 정의

`TextRecordBase`를 상속받아 레코드 클래스를 정의합니다:

```cpp
class A : public FdFile::TextRecordBase {
public:
    std::string name;
    long id = 0;

    const char* typeName() const override { return "A"; }

    void toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"name", {true, name}});  // true = 문자열 타입
        out.push_back({"id", {false, std::to_string(id)}});  // false = 숫자 타입
    }

    bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                 std::error_code& ec) override {
        // 파싱 로직 구현
        // ...
    }
};
```

### 2. 파일 열기

타입 팩토리 함수를 등록하여 파일을 엽니다:

```cpp
std::error_code ec;
FdFile::FdTextFile f(
    "./test/mixed.txt",
    {
        {"A", [](){ return std::make_unique<A>(); }},
        {"B", [](){ return std::make_unique<B>(); }},
    },
    ec
);
if (ec) {
    std::cerr << "open: " << ec.message() << "\n";
    return 1;
}
```

### 3. 레코드 쓰기

`append` 메서드로 레코드를 파일 끝에 추가합니다:

```cpp
A a;
a.name = "john";
a.id = 123;

if (!f.append(a, true, ec)) {  // true = fsync 수행
    std::cerr << "append: " << ec.message() << "\n";
    return 1;
}
```

### 4. 레코드 읽기

`readAt` 메서드로 특정 오프셋에서 레코드를 읽습니다:

```cpp
off_t offset = 0;
std::unique_ptr<FdFile::TextRecordBase> rec;
off_t nextOffset = 0;

if (!f.readAt(offset, rec, nextOffset, ec)) {
    std::cerr << "read error: " << ec.message() << "\n";
    return 1;
}

// 타입에 따라 다운캐스팅
if (std::string(rec->typeName()) == "A") {
    auto* a = static_cast<A*>(rec.get());
    std::cout << "A name=" << a->name << " id=" << a->id << "\n";
}
```

## 파일 포맷

각 레코드는 한 줄로 저장되며, 다음과 같은 형식을 따릅니다:

```
TypeName { "key1": "value1", "key2": 123 }
```

예시:
```
A { "name": "john", "id": 123 }
B { "name": "albert", "id": 1234, "pw": "1234" }
```

### 무시되는 라인

- 빈 줄은 무시됩니다.
- 주석 줄은 무시됩니다: 공백을 제외한 첫 문자가 `#`이면 주석으로 취급합니다.

## 주요 특징

### 파일 잠금

- **쓰기**: 배타 잠금 (exclusive lock) 사용
- **읽기**: 공유 잠금 (shared lock) 사용
- 여러 프로세스가 동시에 파일에 접근할 때 데이터 일관성 보장

### 유연한 읽기

- 마지막 줄이 개행 문자로 끝나지 않아도 정상적으로 읽을 수 있음
- 레코드가 완전하면 파싱 성공
- 포맷이 깨지거나 중간에 잘리면 에러 반환

### 에러 처리

모든 I/O 작업은 `std::error_code`를 통해 에러를 반환합니다:

```cpp
std::error_code ec;
if (!f.append(record, true, ec)) {
    if (ec == std::errc::not_supported) {
        // 지원하지 않는 타입
    } else if (ec == std::errc::io_error) {
        // I/O 에러
    }
}
```

## 빌드 및 실행

```bash
# CMake (권장)
cmake -S . -B build
cmake --build build -j
./build/fdfile_example

# 또는 직접 컴파일
g++ -std=c++17 -o main main.cpp -lstdc++fs

# 실행
./main
```

## 요구사항

- C++17 이상
- POSIX 호환 시스템 (Linux, macOS 등)
- 파일 시스템에 대한 읽기/쓰기 권한

## 라이선스

LICENSE 파일을 참조하세요.
