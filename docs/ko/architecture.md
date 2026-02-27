# 아키텍처

FdFileLib의 내부 설계 패턴과 아키텍처입니다.

## 디렉토리 구조

```
posixCppFdFileLibrary/
├── include/fdfile/          # 공개 헤더
│   ├── fdfile.hpp           # 단일 진입점
│   ├── record/              # 레코드 타입
│   │   ├── RecordBase.hpp
│   │   ├── FieldMeta.hpp
│   │   ├── FixedRecordBase.hpp
│   │   └── VariableRecordBase.hpp
│   ├── repository/          # 리포지토리 구현
│   │   ├── RecordRepository.hpp
│   │   ├── UniformFixedRepositoryImpl.hpp
│   │   └── VariableFileRepositoryImpl.hpp
│   └── util/                # 유틸리티 클래스
│       ├── UniqueFd.hpp
│       ├── MmapGuard.hpp
│       ├── FileLockGuard.hpp
│       └── textFormatUtil.hpp
├── src/                     # 구현 파일
│   └── VariableFileRepositoryImpl.cpp
├── cmake/                   # CMake 패키지 설정
├── docs/                    # 문서
├── examples/                # 예제 코드
└── tests/                   # 유닛 테스트
```

## 설계 패턴

### CRTP (Curiously Recurring Template Pattern)

제로 vtable 다형성을 위해 고정 길이 레코드에 사용됩니다:

```cpp
template <typename Derived>
class FixedRecordBase {
    bool serialize(char* buf) const {
        // 가상 디스패치 없이 파생 클래스 메서드 호출
        static_cast<const Derived*>(this)->__getFieldValue(i, buf);
    }
};

class User : public FixedRecordBase<User> {
    // vtable이 생성되지 않음
};
```

장점:
- 가상 함수 오버헤드 없음
- 인라인 가능한 메서드 호출
- 컴파일 타임 타입 안전성

### 리포지토리 패턴

데이터 영속성을 추상화합니다:

```cpp
template <typename T>
class RecordRepository {
    virtual bool save(const T& record, std::error_code& ec) = 0;
    virtual std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) = 0;
    // ...
};
```

### RAII (Resource Acquisition Is Initialization)

모든 리소스가 자동으로 관리됩니다:

```cpp
// 파일 디스크립터
detail::UniqueFd fd(::open(path, O_RDWR));
// fd가 스코프를 벗어나면 자동으로 닫힘

// 메모리 매핑
detail::MmapGuard mmap(ptr, size);
// 자동으로 언맵됨

// 파일 잠금
detail::FileLockGuard lock(fd, Mode::Exclusive, ec);
// 자동으로 잠금 해제
```

## 컴포넌트 다이어그램

```
┌─────────────────────────────────────────────────────────┐
│                     애플리케이션                          │
├─────────────────────────────────────────────────────────┤
│  ┌─────────────────────┐  ┌─────────────────────────┐   │
│  │  사용자 정의         │  │  사용자 정의             │   │
│  │  FixedRecord        │  │  VariableRecord         │   │
│  └──────────┬──────────┘  └───────────┬─────────────┘   │
│             │                         │                  │
│  ┌──────────▼──────────┐  ┌───────────▼─────────────┐   │
│  │  FixedRecordBase    │  │  VariableRecordBase     │   │
│  │  (CRTP 템플릿)       │  │  (가상 인터페이스)       │   │
│  └──────────┬──────────┘  └───────────┬─────────────┘   │
│             │                         │                  │
│  ┌──────────▼──────────┐  ┌───────────▼─────────────┐   │
│  │  UniformFixed       │  │  VariableFile           │   │
│  │  RepositoryImpl     │  │  RepositoryImpl         │   │
│  └──────────┬──────────┘  └───────────┬─────────────┘   │
│             │                         │                  │
│  ┌──────────▼─────────────────────────▼─────────────┐   │
│  │              유틸리티 클래스                       │   │
│  │  UniqueFd │ MmapGuard │ FileLockGuard            │   │
│  └──────────────────────────────────────────────────┘   │
│                          │                               │
├──────────────────────────▼──────────────────────────────┤
│                    POSIX API                            │
│  open() │ read() │ write() │ mmap() │ fcntl() │ close() │
└─────────────────────────────────────────────────────────┘
```

## 데이터 흐름

### 고정 레코드 쓰기

```
User::serialize()
    │
    ▼
FixedRecordBase::serialize()
    │
    ├── 포맷 템플릿 복사
    ├── 타입 이름 쓰기
    ├── ID 쓰기
    └── 필드 쓰기 (CRTP 통해)
    │
    ▼
UniformFixedRepositoryImpl::save()
    │
    ├── 배타 잠금 획득
    ├── 외부 수정 확인
    ├── 캐시에서 ID 찾기
    │   ├── 찾음 → 제자리 업데이트
    │   └── 못 찾음 → 추가
    ├── 캐시 갱신
    └── 잠금 해제
```

### 고정 레코드 읽기

```
UniformFixedRepositoryImpl::findById()
    │
    ├── 공유 잠금 획득
    ├── 외부 수정 확인
    │   └── 변경됨 → 캐시 재구축
    ├── 캐시에서 조회 (O(1))
    ├── mmap에서 읽기
    ├── 역직렬화
    └── 잠금 해제
    │
    ▼
User::deserialize()
    │
    ▼
std::unique_ptr<User> 반환
```

## 캐싱 전략

### ID 캐시 (고정 레코드)

```cpp
std::unordered_map<std::string, size_t> idCache_;
// ID → 레코드 인덱스 매핑

// 캐시 무효화 트리거:
// 1. fstat()가 mtime 변경 감지
// 2. fstat()가 size 변경 감지
// 3. deleteById()가 인덱스 이동
```

### 레코드 캐시 (가변 레코드)

```cpp
std::vector<std::unique_ptr<VariableRecordBase>> cache_;
bool cacheValid_ = false;

// 캐시 무효화 트리거:
// 1. stat()가 mtime 변경 감지
// 2. stat()가 size 변경 감지
// 3. 모든 쓰기 작업
```

## 파일 잠금

POSIX fcntl 권고 잠금 사용:

```cpp
struct flock fl;
fl.l_type = F_RDLCK;   // 공유 (읽기)
fl.l_type = F_WRLCK;   // 배타 (쓰기)
fl.l_type = F_UNLCK;   // 잠금 해제
fl.l_whence = SEEK_SET;
fl.l_start = 0;
fl.l_len = 0;          // 전체 파일

fcntl(fd, F_SETLKW, &fl);  // 블로킹 대기
```

## 메모리 매핑

고정 레코드는 제로 카피 접근을 위해 mmap 사용:

```cpp
void* ptr = mmap(nullptr, size, 
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 fd, 0);

// 파일 내용에 직접 메모리 접근
char* record = static_cast<char*>(ptr) + index * recordSize;

// 변경사항을 디스크에 동기화
msync(ptr, size, MS_SYNC);

// 정리
munmap(ptr, size);
```

## 스레드 안전성

- 파일 잠금이 다중 프로세스 안전성 제공
- 프로세스 내에서는 단일 스레드 (내부 뮤텍스 없음)
- 멀티스레드 접근 시 외부 동기화 사용

## 확장 포인트

### 커스텀 레코드 타입

다음 중 하나를 구현:
- `FixedRecordBase<T>` 바이너리 레코드용
- `VariableRecordBase` 텍스트 레코드용

### 커스텀 리포지토리

`RecordRepository<T>` 인터페이스 구현:
- 다른 저장소 백엔드 (SQLite, 네트워크 등)
- 커스텀 캐싱 전략
- 다른 직렬화 형식
