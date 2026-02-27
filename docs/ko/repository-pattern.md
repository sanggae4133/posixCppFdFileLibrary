# 리포지토리 패턴

FdFileLib는 데이터 접근 추상화를 위한 리포지토리 패턴을 구현합니다.

## 인터페이스

고정 및 가변 리포지토리 모두 `RecordRepository<T>`를 구현합니다:

```cpp
template <typename T>
class RecordRepository {
public:
    virtual bool save(const T& record, std::error_code& ec) = 0;
    virtual bool saveAll(const std::vector<const T*>& records, std::error_code& ec) = 0;
    virtual std::vector<std::unique_ptr<T>> findAll(std::error_code& ec) = 0;
    virtual std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) = 0;
    virtual bool deleteById(const std::string& id, std::error_code& ec) = 0;
    virtual bool deleteAll(std::error_code& ec) = 0;
    virtual size_t count(std::error_code& ec) = 0;
    virtual bool existsById(const std::string& id, std::error_code& ec) = 0;
};
```

## 작업

### 저장 (Upsert)

ID 기반으로 삽입 또는 수정:

```cpp
User alice("Alice", 30, "001");
repo.save(alice, ec);  // 삽입

alice.age = 31;
repo.save(alice, ec);  // 수정 (같은 ID)
```

### ID로 찾기

```cpp
auto found = repo.findById("001", ec);
if (found) {
    std::cout << found->name << std::endl;
}
```

### 전체 찾기

```cpp
auto all = repo.findAll(ec);
for (const auto& record : all) {
    std::cout << record->getId() << std::endl;
}
```

### ID 존재 확인

```cpp
if (repo.existsById("001", ec)) {
    std::cout << "사용자 존재함" << std::endl;
}
```

### ID로 삭제

```cpp
repo.deleteById("001", ec);
```

### 전체 삭제

```cpp
repo.deleteAll(ec);
```

### 개수

```cpp
size_t total = repo.count(ec);
```

### 일괄 저장

```cpp
User alice("Alice", 30, "001");
User bob("Bob", 25, "002");

std::vector<const User*> users = {&alice, &bob};
repo.saveAll(users, ec);
```

## 에러 처리

모든 작업은 `std::error_code`를 사용합니다:

```cpp
std::error_code ec;

repo.save(record, ec);
if (ec) {
    std::cerr << "저장 실패: " << ec.message() << std::endl;
    // 에러 처리
}
```

## 동시성

### 파일 잠금

작업은 fcntl 파일 잠금을 사용합니다:

| 작업 | 잠금 타입 |
|------|----------|
| `save()` | 배타 |
| `saveAll()` | 배타 |
| `findById()` | 공유 |
| `findAll()` | 공유 |
| `deleteById()` | 배타 |
| `deleteAll()` | 배타 |
| `count()` | 공유 |
| `existsById()` | 공유 |

### 다중 프로세스 안전성

```cpp
// 프로세스 A
FdFile::UniformFixedRepositoryImpl<User> repoA("users.db", ec);
repoA.save(alice, ec);  // 배타 잠금 획득

// 프로세스 B (동시)
FdFile::UniformFixedRepositoryImpl<User> repoB("users.db", ec);
repoB.save(bob, ec);    // 잠금 대기 후 쓰기
```

## 캐싱

### 고정 리포지토리 캐시

고정 리포지토리는 ID 캐시를 유지합니다:

```cpp
// 첫 호출: 캐시 구축
auto found1 = repo.findById("001", ec);  // O(N) 캐시 구축

// 이후 호출: 캐시 히트
auto found2 = repo.findById("002", ec);  // O(1) 조회
```

### 캐시 무효화

다음 경우 캐시가 자동으로 갱신됩니다:

1. 파일 mtime 변경 (외부 수정)
2. 파일 크기 변경
3. 삭제 작업 (인덱스 이동)

## 성능 비교

| 작업 | 고정 | 가변 |
|------|------|------|
| `save()` 삽입 | O(1) | O(1) 추가 |
| `save()` 수정 | O(1) | O(N) 재작성 |
| `findById()` | O(1) | O(N) |
| `findAll()` | O(N) | O(N) |
| `deleteById()` | O(N) 이동 | O(N) 재작성 |
| `count()` | O(1) | O(1) 캐시됨 |

## 모범 사례

### 1. 에러 처리

```cpp
if (!repo.save(record, ec)) {
    // 로그 및 에러 처리
    logger.error("저장 실패: {}", ec.message());
}
```

### 2. 일괄 작업

```cpp
// 좋음: 한 번 호출
std::vector<const User*> batch = {&u1, &u2, &u3};
repo.saveAll(batch, ec);

// 나쁨: 여러 번 호출
repo.save(u1, ec);
repo.save(u2, ec);
repo.save(u3, ec);
```

### 3. 삭제 전 존재 확인

```cpp
// 존재하는 경우에만 삭제
if (repo.existsById(id, ec)) {
    repo.deleteById(id, ec);
}
```

## 다음 단계

- [고정 길이 레코드](fixed-records.md) - 바이너리 레코드 상세
- [가변 길이 레코드](variable-records.md) - 텍스트 레코드 상세
- [API 레퍼런스](api-reference.md) - 전체 API 문서
