# 고정 길이 레코드

고정 길이 레코드는 바이너리 저장과 메모리 매핑을 통해 최대 성능을 제공합니다.

## 개요

| 특징 | 설명 |
|------|------|
| 저장 형식 | 바이너리 (mmap) |
| 조회 속도 | O(1) ID 캐시 |
| Vtable 오버헤드 | 없음 (CRTP) |
| 사용 사례 | 고성능, 구조화된 데이터 |

## 레코드 정의

### 기본 구조

```cpp
#include <fdfile/fdfile.hpp>
#include <cstring>

class Product : public FdFile::FixedRecordBase<Product> {
public:
    // public 데이터 멤버
    char name[30];    // 고정 30바이트
    int64_t price;    // 고정 20바이트 (부호 + 19자리)
    int64_t stock;

private:
    // 필수: 필드 메타데이터 튜플
    auto fields() const {
        return std::make_tuple(
            FD_STR(name),    // 문자열 필드
            FD_NUM(price),   // 숫자 필드
            FD_NUM(stock)
        );
    }

    // 필수: 멤버 초기화
    void initMembers() {
        std::memset(name, 0, sizeof(name));
        price = 0;
        stock = 0;
    }

    // 필수: 공통 메서드 생성
    // 인자: 클래스명, 타입명, 타입필드길이, ID필드길이
    FD_RECORD_IMPL(Product, "Product", 10, 10)

public:
    // 커스텀 생성자
    Product(const char* n, int64_t p, int64_t s, const char* id) {
        initMembers();
        if (n) std::strncpy(name, n, sizeof(name));
        price = p;
        stock = s;
        setId(id);
        defineLayout();
    }
};
```

### 필드 타입

| 매크로 | 타입 | 길이 | 설명 |
|--------|------|------|------|
| `FD_STR(member)` | `char[N]` | N 바이트 | 고정 길이 문자열 |
| `FD_NUM(member)` | `int64_t` | 20 바이트 | 부호 있는 64비트 정수 |

## 리포지토리 사용

### 기본 CRUD

```cpp
std::error_code ec;
FdFile::UniformFixedRepositoryImpl<Product> repo("products.db", ec);

// 생성
Product laptop("Laptop", 1500000, 50, "P001");
repo.save(laptop, ec);

// ID로 읽기 (O(1))
auto found = repo.findById("P001", ec);

// 전체 읽기
auto all = repo.findAll(ec);

// 수정 (같은 ID)
laptop.stock = 45;
repo.save(laptop, ec);

// 삭제
repo.deleteById("P001", ec);

// 개수
size_t count = repo.count(ec);
```

### 성능 특성

| 작업 | 복잡도 | 비고 |
|------|--------|------|
| `save()` (삽입) | O(1) | 추가 + 캐시 갱신 |
| `save()` (수정) | O(1) | 제자리 쓰기 |
| `findById()` | O(1) | 캐시 조회 + mmap 읽기 |
| `findAll()` | O(N) | 순차 읽기 |
| `deleteById()` | O(N) | 데이터 이동 필요 |
| `count()` | O(1) | 크기 계산 |
| `existsById()` | O(1) | 캐시 조회 |

## ID 캐시

리포지토리는 O(1) 조회를 위한 내부 캐시를 유지합니다:

```
findById("P001")
    ↓
checkAndRefreshCache()
    ├── fstat() → mtime 변경?
    │       ├── 예 → remapFile() + rebuildCache()
    │       └── 아니오 → 기존 캐시 사용
    ↓
idCache_.find("P001") → O(1) 조회
    ↓
인덱스의 레코드 반환
```

### 외부 수정 감지

다른 프로세스가 파일을 수정할 때:

1. `fstat()`가 mtime/size 변경 감지
2. 캐시 무효화
3. 파일 다시 매핑
4. 캐시 재구축

## CRTP 패턴

FdFileLib는 제로 오버헤드 다형성을 위해 CRTP를 사용합니다:

```
FixedRecordBase<Derived>   ← 템플릿 베이스
        ↑
        │ static_cast<Derived*>(this)
        │
      Product              ← 구체 타입
```

장점:
- 가상 함수 테이블 오버헤드 없음
- 인라인 가능한 메서드 호출
- 컴파일 타임 타입 안전성

## 다음 단계

- [가변 길이 레코드](variable-records.md) - 유연한 스키마 데이터용
- [리포지토리 패턴](repository-pattern.md) - 고급 리포지토리 사용법
- [API 레퍼런스](api-reference.md) - 전체 클래스 문서
