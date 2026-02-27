# 시작하기

이 가이드는 FdFileLib을 프로젝트에서 사용할 수 있도록 도와드립니다.

## 요구 사항

| 요구 사항 | 버전 |
|----------|------|
| C++ 표준 | C++17 |
| 컴파일러 | GCC 7+, Clang 5+, Apple Clang 10+ |
| CMake | 3.16+ |
| 플랫폼 | POSIX (Linux/macOS) |

## 설치

### 방법 1: CMake FetchContent (권장)

`CMakeLists.txt`에 추가:

```cmake
include(FetchContent)
FetchContent_Declare(
    fdfilelib
    GIT_REPOSITORY https://github.com/your-repo/fdfilelib.git
    GIT_TAG v1.0.0
)
# 서브디렉토리로 사용 시 예제와 테스트가 자동으로 비활성화됨
FetchContent_MakeAvailable(fdfilelib)

target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

### 방법 2: 서브디렉토리

라이브러리를 프로젝트에 복사:

```cmake
add_subdirectory(external/fdfilelib)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

### 방법 3: 시스템 설치

빌드 및 설치:

```bash
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

프로젝트에서 사용:

```cmake
find_package(fdfile REQUIRED)
target_link_libraries(your_app PRIVATE fdfile::fdfile)
```

## 첫 번째 예제

### 1. 레코드 정의

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

### 2. 리포지토리 사용

```cpp
#include <fdfile/fdfile.hpp>
#include <iostream>

int main() {
    std::error_code ec;
    
    // 리포지토리 생성
    FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);
    if (ec) {
        std::cerr << "열기 실패: " << ec.message() << std::endl;
        return 1;
    }
    
    // 생성
    User alice("Alice", 30, "001");
    repo.save(alice, ec);
    
    // 읽기
    auto found = repo.findById("001", ec);
    if (found) {
        std::cout << "찾음: " << found->name << ", " << found->age << std::endl;
    }
    
    // 수정
    alice.age = 31;
    repo.save(alice, ec);  // 같은 ID = 업데이트
    
    // 삭제
    repo.deleteById("001", ec);
    
    return 0;
}
```

### 3. 빌드

```bash
mkdir build && cd build
cmake ..
cmake --build .
./your_app
```

## 다음 단계

- [고정 길이 레코드](fixed-records.md) - 고성능 바이너리 레코드 알아보기
- [가변 길이 레코드](variable-records.md) - 유연한 JSON 스타일 레코드 알아보기
- [리포지토리 패턴](repository-pattern.md) - CRUD 작업 심화
- [API 레퍼런스](api-reference.md) - 전체 API 문서
