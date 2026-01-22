# FdFileLib

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

POSIX API (`open`, `read`, `write`, `fsync`, `flock`)와 `mmap`을 사용한 고성능 C++17 파일 리포지토리 라이브러리입니다. **고정 길이 (바이너리)**와 **가변 길이 (JSON 스타일)** 레코드 형식을 모두 지원합니다.

[English](README.md) | [문서](docs/ko/README.md) | [English Documentation](docs/README.md)

## 특징

| 특징 | 설명 |
|------|------|
| **고성능** | O(1) 랜덤 액세스를 위한 직접 POSIX I/O와 `mmap` |
| **O(1) ID 조회** | 빠른 `findById()`를 위한 내부 해시맵 캐시 |
| **제로 Vtable 오버헤드** | 고정 레코드를 위한 CRTP 패턴 |
| **C++17 튜플 기반** | `std::tuple`과 fold expressions를 사용한 모던 필드 메타데이터 |
| **RAII 안전성** | FD, mmap, 파일 잠금의 자동 정리 |
| **외부 수정 감지** | 외부 프로세스가 파일을 수정하면 자동 리로드 |
| **파일 잠금** | 동시성을 위한 `fcntl` 기반 공유/배타 잠금 |
| **Upsert 의미론** | `save()`가 자동으로 삽입 또는 업데이트 |

## 요구 사항

| 요구 사항 | 버전 |
|----------|------|
| C++ 표준 | C++17 |
| 컴파일러 | GCC 7+, Clang 5+, Apple Clang 10+ |
| CMake | 3.16+ |
| 플랫폼 | POSIX (Linux/macOS) |
| Google Test | (선택, 테스트용) |

## 빠른 시작

### 설치

**CMake FetchContent 사용 (권장):**

```cmake
include(FetchContent)
FetchContent_Declare(
    fdfilelib
    GIT_REPOSITORY https://github.com/your-repo/fdfilelib.git
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(fdfilelib)

target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

**서브디렉토리로 사용:**

```cmake
add_subdirectory(external/fdfilelib)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

**find_package 사용 (설치 후):**

```cmake
find_package(fdfile REQUIRED)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

### 단일 헤더 include

```cpp
#include <fdfile/fdfile.hpp>
```

### 레코드 정의

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

### 리포지토리 사용

```cpp
int main() {
    std::error_code ec;
    FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);

    // 생성
    User alice("Alice", 30, "001");
    repo.save(alice, ec);

    // 읽기 (O(1))
    auto found = repo.findById("001", ec);

    // 수정
    alice.age = 31;
    repo.save(alice, ec);

    // 삭제
    repo.deleteById("001", ec);
}
```

## 소스에서 빌드

```bash
git clone <repo-url>
cd posixCppFdFileLibrary
mkdir build && cd build
cmake ..
cmake --build . -j

# 테스트 실행
ctest --output-on-failure

# 설치
cmake --install . --prefix /usr/local
```

### CMake 옵션

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `FDFILE_BUILD_EXAMPLES` | ON* | 예제 프로그램 빌드 |
| `FDFILE_BUILD_TESTS` | ON* | 유닛 테스트 빌드 |
| `FDFILE_INSTALL` | ON | 설치 타겟 생성 |

*\*서브디렉토리로 사용 시 OFF가 기본값*

## 프로젝트 구조

```
posixCppFdFileLibrary/
├── include/fdfile/          # 공개 헤더
│   ├── fdfile.hpp           # 단일 진입점
│   ├── record/              # 레코드 타입
│   ├── repository/          # 리포지토리 구현
│   └── util/                # 유틸리티 클래스
├── src/                     # 구현 파일
├── cmake/                   # CMake 패키지 설정
├── docs/                    # 문서 (영문/한글)
├── examples/                # 예제 코드
└── tests/                   # 유닛 테스트 (155개)
```

## API 레퍼런스

### 리포지토리 메서드

| 메서드 | 설명 | 복잡도 |
|--------|------|--------|
| `save(record, ec)` | Upsert | O(1) |
| `findById(id, ec)` | ID로 찾기 | **O(1)** |
| `findAll(ec)` | 전체 조회 | O(N) |
| `existsById(id, ec)` | 존재 확인 | **O(1)** |
| `deleteById(id, ec)` | 하나 삭제 | O(N) |
| `deleteAll(ec)` | 전체 삭제 | O(1) |
| `count(ec)` | 개수 | O(1) |

### 매크로

| 매크로 | 설명 |
|--------|------|
| `FD_STR(member)` | 문자열 필드 (char 배열) |
| `FD_NUM(member)` | 숫자 필드 (int64_t) |
| `FD_RECORD_IMPL(Class, Type, TypeLen, IdLen)` | 레코드 메서드 생성 |

## 문서

### 한국어 문서

- [시작하기](docs/ko/getting-started.md)
- [고정 길이 레코드](docs/ko/fixed-records.md)
- [가변 길이 레코드](docs/ko/variable-records.md)
- [리포지토리 패턴](docs/ko/repository-pattern.md)
- [API 레퍼런스](docs/ko/api-reference.md)
- [아키텍처](docs/ko/architecture.md)
- [에러 처리](docs/ko/error-handling.md)

### English Documentation

- [Getting Started](docs/getting-started.md)
- [Fixed Records](docs/fixed-records.md)
- [Variable Records](docs/variable-records.md)
- [Repository Pattern](docs/repository-pattern.md)
- [API Reference](docs/api-reference.md)

## 라이선스

MIT 라이선스 - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.
