# FdFileLib 문서

FdFileLib 문서에 오신 것을 환영합니다! 이 라이브러리는 POSIX API를 사용하여 고성능 파일 기반 레코드 저장소를 제공합니다.

## 목차

- [시작하기](getting-started.md) - 설치 및 기본 사용법
- [고정 길이 레코드](fixed-records.md) - 고성능 바이너리 레코드
- [가변 길이 레코드](variable-records.md) - 유연한 JSON 스타일 레코드
- [리포지토리 패턴](repository-pattern.md) - CRUD 작업
- [API 레퍼런스](api-reference.md) - 전체 API 문서
- [아키텍처](architecture.md) - 내부 설계 및 패턴
- [에러 처리](error-handling.md) - 에러 처리 패턴

## 빠른 링크

| 주제 | 설명 |
|------|------|
| [시작하기](getting-started.md) | 설치, CMake 설정, 첫 번째 예제 |
| [고정 길이 레코드](fixed-records.md) | CRTP 기반 제로 vtable 바이너리 레코드 |
| [가변 길이 레코드](variable-records.md) | 가상 다형성 JSON 스타일 레코드 |
| [리포지토리 패턴](repository-pattern.md) | CRUD 작업 및 캐싱 |
| [API 레퍼런스](api-reference.md) | 상세 클래스 및 함수 문서 |

## 기능 개요

### 레코드 타입

| 타입 | 형식 | 조회 속도 | 사용 사례 |
|------|------|----------|----------|
| 고정 길이 | 바이너리 | O(1) | 고성능, 구조화된 데이터 |
| 가변 길이 | JSON 스타일 | O(N) | 유연성, 스키마 변경 |

### 주요 기능

- **RAII 리소스 관리** - 파일 디스크립터, mmap, 락의 자동 정리
- **O(1) ID 조회** - 고정 레코드용 내부 해시맵 캐시
- **외부 수정 감지** - 파일 변경 시 자동 리로드
- **파일 잠금** - fcntl 기반 동시 접근 제어
- **제로 카피 mmap** - 성능을 위한 직접 메모리 매핑

## 버전

현재 버전: **1.0.0**

```cpp
#include <fdfile/fdfile.hpp>

std::cout << FdFile::VERSION_STRING; // "1.0.0"
```
