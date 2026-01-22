# API 레퍼런스

FdFileLib의 전체 API 문서입니다.

## 핵심 컴포넌트

### `FdFile::FixedRecordBase<Derived>`

CRTP를 사용하는 고정 길이 레코드용 템플릿 베이스 클래스.

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

#### 메서드

| 메서드 | 설명 |
|--------|------|
| `recordSize()` | 전체 레코드 크기(바이트) 반환 |
| `serialize(buf)` | 레코드를 버퍼에 쓰기 |
| `deserialize(buf, ec)` | 버퍼에서 레코드 읽기 |

### `FdFile::VariableRecordBase`

가변 길이 레코드용 가상 베이스 클래스.

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

#### 가상 메서드

| 메서드 | 설명 |
|--------|------|
| `id()` | 레코드의 고유 식별자 반환 |
| `typeName()` | 레코드 타입 이름 반환 |
| `toKv(out)` | 키-값 쌍으로 직렬화 |
| `fromKv(kv, ec)` | 키-값 쌍에서 역직렬화 |
| `clone()` | 깊은 복사 생성 |

---

## 리포지토리

### `FdFile::UniformFixedRepositoryImpl<T>`

고정 길이 레코드용 리포지토리.

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

#### 생성자

```cpp
UniformFixedRepositoryImpl(const std::string& path, std::error_code& ec);
```

| 파라미터 | 설명 |
|----------|------|
| `path` | 저장소 파일 경로 |
| `ec` | 에러 코드 (실패 시 설정) |

### `FdFile::VariableFileRepositoryImpl`

가변 길이 레코드용 리포지토리.

```cpp
class VariableFileRepositoryImpl : public RecordRepository<VariableRecordBase> {
public:
    VariableFileRepositoryImpl(
        const std::string& path,
        std::vector<std::unique_ptr<VariableRecordBase>> prototypes,
        std::error_code& ec);
    
    // ... UniformFixedRepositoryImpl과 동일한 인터페이스
};
```

#### 생성자

```cpp
VariableFileRepositoryImpl(
    const std::string& path,
    std::vector<std::unique_ptr<VariableRecordBase>> prototypes,
    std::error_code& ec);
```

| 파라미터 | 설명 |
|----------|------|
| `path` | 저장소 파일 경로 |
| `prototypes` | 각 레코드 타입의 프로토타입 인스턴스 |
| `ec` | 에러 코드 (실패 시 설정) |

---

## 매크로

### `FD_STR(member)`

문자열 필드 선언 (char 배열).

```cpp
char name[20];
auto fields() const {
    return std::make_tuple(FD_STR(name));  // 20바이트
}
```

### `FD_NUM(member)`

숫자 필드 선언 (int64_t).

```cpp
int64_t age;
auto fields() const {
    return std::make_tuple(FD_NUM(age));  // 20바이트
}
```

### `FD_RECORD_IMPL(ClassName, TypeName, TypeLen, IdLen)`

공통 레코드 메서드 생성.

```cpp
FD_RECORD_IMPL(User, "User", 10, 10)
```

| 파라미터 | 설명 |
|----------|------|
| `ClassName` | 클래스 이름 |
| `TypeName` | 타입 문자열 (파일에 저장) |
| `TypeLen` | 타입 필드 길이 |
| `IdLen` | ID 필드 길이 |

생성되는 메서드:
- 기본 생성자
- `typeName()`
- `getId()`
- `setId()`
- `__getFieldValue()`
- `__setFieldValue()`

---

## 유틸리티 클래스

### `FdFile::detail::UniqueFd`

POSIX 파일 디스크립터용 RAII 래퍼.

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

mmap용 RAII 래퍼.

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

fcntl 파일 잠금용 RAII 가드.

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

## 버전 상수

```cpp
namespace FdFile {
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;
    constexpr const char* VERSION_STRING = "1.0.0";
}
```

---

## 에러 처리

모든 작업은 `std::error_code`를 사용합니다:

```cpp
std::error_code ec;
repo.save(record, ec);

if (ec) {
    // ec.value() - 에러 코드
    // ec.message() - 사람이 읽을 수 있는 메시지
    // ec.category() - 에러 카테고리
}
```

일반적인 에러 코드:
- `std::errc::invalid_argument` - 잘못된 파라미터
- `std::errc::bad_file_descriptor` - 잘못된 파일
- `std::errc::result_out_of_range` - 값 범위 초과
- I/O 에러에 대한 시스템 errno 값
