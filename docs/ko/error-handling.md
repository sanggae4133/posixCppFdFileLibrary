# 에러 처리

FdFileLib는 모든 에러 처리에 `std::error_code`를 사용합니다.

## 기본 패턴

```cpp
std::error_code ec;

repo.save(record, ec);
if (ec) {
    std::cerr << "에러: " << ec.message() << std::endl;
    // 에러 처리
}
```

## 에러 카테고리

### 표준 에러 코드

| 코드 | 의미 |
|------|------|
| `std::errc::invalid_argument` | 잘못된 파라미터 |
| `std::errc::bad_file_descriptor` | 잘못된 파일 디스크립터 |
| `std::errc::result_out_of_range` | 값 범위 초과 |

### 시스템 에러 코드

I/O 작업은 시스템 errno 값을 반환할 수 있습니다:

| 코드 | 의미 |
|------|------|
| `ENOENT` | 파일 없음 |
| `EACCES` | 권한 거부 |
| `ENOSPC` | 디스크 공간 부족 |
| `EIO` | I/O 에러 |
| `ENOMEM` | 메모리 부족 |

## 일반적인 시나리오

### 파일 열기 실패

```cpp
std::error_code ec;
FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);

if (ec) {
    if (ec == std::errc::no_such_file_or_directory) {
        // 파일이 자동 생성되므로 이 경우는 발생하지 않아야 함
    } else if (ec.value() == EACCES) {
        std::cerr << "권한 거부" << std::endl;
    } else {
        std::cerr << "열기 실패: " << ec.message() << std::endl;
    }
}
```

### 레코드 크기 불일치

```cpp
// 파일에 다른 크기의 레코드가 있음
std::error_code ec;
repo.save(record, ec);

if (ec == std::errc::invalid_argument) {
    std::cerr << "레코드 크기 불일치 - 파일이 손상되었을 수 있음" << std::endl;
}
```

### 손상된 데이터

```cpp
auto record = repo.findById("001", ec);

if (ec) {
    std::cerr << "읽기 실패: " << ec.message() << std::endl;
    // 손상된 파일일 수 있음
}

if (!record) {
    // 레코드를 찾지 못함 (에러 아님)
}
```

## 예외 없는 설계

FdFileLib는 리포지토리 작업에서 예외를 던지지 않습니다:

```cpp
// 항상 반환, 예외 없음
bool success = repo.save(record, ec);

// ec로 에러 확인
if (!success || ec) {
    // 에러 처리
}
```

### 내부 예외

`FieldMeta::set()`은 잘못된 숫자 형식에 대해 `std::runtime_error`를 던질 수 있습니다:

```cpp
try {
    record.deserialize(buf, ec);
} catch (const std::runtime_error& e) {
    // 잘못된 숫자 필드 형식
    std::cerr << "파싱 에러: " << e.what() << std::endl;
}
```

## 에러 확인 패턴

### 단순 확인

```cpp
if (ec) {
    // 에러 발생
}
```

### 상세 확인

```cpp
if (ec) {
    std::cerr << "에러 코드: " << ec.value() << std::endl;
    std::cerr << "메시지: " << ec.message() << std::endl;
    std::cerr << "카테고리: " << ec.category().name() << std::endl;
}
```

### 반환값 확인

```cpp
if (!repo.save(record, ec) || ec) {
    // 저장 실패
}

auto found = repo.findById("001", ec);
if (!found) {
    if (ec) {
        // 읽기 에러
    } else {
        // 레코드를 찾지 못함 (에러 아님)
    }
}
```

## 에러 확인 매크로

예제와 테스트에 유용한 패턴:

```cpp
#define CHECK_EC(ec, context)                                      \
    if (ec) {                                                      \
        std::cerr << context << ": " << ec.message() << std::endl; \
        return false;                                              \
    }

bool doSomething() {
    std::error_code ec;
    
    repo.save(record, ec);
    CHECK_EC(ec, "레코드 저장");
    
    auto found = repo.findById("001", ec);
    CHECK_EC(ec, "레코드 찾기");
    
    return true;
}
```

## 모범 사례

### 1. 작업 후 항상 확인

```cpp
repo.save(record, ec);
if (ec) { /* 처리 */ }

auto found = repo.findById(id, ec);
if (ec) { /* 처리 */ }
```

### 2. 재사용 전 에러 코드 초기화

```cpp
std::error_code ec;

repo.save(r1, ec);
if (ec) { /* 처리 */ }

ec.clear();  // 선택사항 - 작업이 내부적으로 ec를 초기화함

repo.save(r2, ec);
if (ec) { /* 처리 */ }
```

### 3. 의미있는 컨텍스트 로깅

```cpp
if (ec) {
    logger.error("사용자 {}를 {}에 저장 실패: {}", 
                 user.getId(), 
                 filePath, 
                 ec.message());
}
```

### 4. 처리 vs 전파

```cpp
// 로컬에서 처리
bool saveUser(const User& user) {
    std::error_code ec;
    repo.save(user, ec);
    if (ec) {
        logger.warn("저장 실패, 재시도 예정");
        return false;
    }
    return true;
}

// 호출자에게 전파
bool saveUser(const User& user, std::error_code& ec) {
    return repo.save(user, ec);
}
```
