# 가변 길이 레코드

가변 길이 레코드는 JSON 스타일의 텍스트 저장을 통해 유연성을 제공합니다.

## 개요

| 특징 | 설명 |
|------|------|
| 저장 형식 | 텍스트 (JSON 스타일) |
| 조회 속도 | O(N) |
| 다형성 | 가상 (런타임) |
| 사용 사례 | 유연한 데이터, 스키마 변경 |

## 레코드 정의

### 기본 구조

```cpp
#include <fdfile/fdfile.hpp>

class Config : public FdFile::VariableRecordBase {
public:
    std::string key;
    std::string value;
    long version = 0;

    Config() = default;
    Config(std::string k, std::string v, long ver)
        : key(std::move(k)), value(std::move(v)), version(ver) {}

    // 필수: 고유 ID 반환
    std::string id() const override { return key; }

    // 필수: 타입 이름 반환
    const char* typeName() const override { return "Config"; }

    // 필수: 복제용 clone
    std::unique_ptr<FdFile::RecordBase> clone() const override {
        return std::make_unique<Config>(*this);
    }

    // 필수: 키-값 쌍으로 직렬화
    void toKv(std::vector<std::pair<std::string, 
              std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"key", {true, key}});           // 문자열
        out.push_back({"value", {true, value}});       // 문자열
        out.push_back({"version", {false, std::to_string(version)}}); // 숫자
    }

    // 필수: 키-값 쌍에서 역직렬화
    bool fromKv(const std::unordered_map<std::string, 
                std::pair<bool, std::string>>& kv,
                std::error_code& ec) override {
        ec.clear();
        
        auto k = kv.find("key");
        auto v = kv.find("value");
        auto ver = kv.find("version");
        
        if (k == kv.end() || v == kv.end() || ver == kv.end())
            return false;

        key = k->second.second;
        value = v->second.second;
        
        long tmp = 0;
        if (!FdFile::util::parseLongStrict(ver->second.second, tmp, ec))
            return false;
        version = tmp;
        
        return true;
    }
};
```

### 파일 형식

레코드는 한 줄에 하나의 JSON 스타일로 저장됩니다:

```
Config { "key": "database.host", "value": "localhost", "version": 1 }
Config { "key": "database.port", "value": "5432", "version": 1 }
Setting { "name": "theme", "enabled": 1 }
```

## 리포지토리 사용

### 프로토타입 설정

가변 레코드는 타입 재구성을 위한 프로토타입 인스턴스가 필요합니다:

```cpp
std::error_code ec;

// 각 레코드 타입의 프로토타입 생성
std::vector<std::unique_ptr<FdFile::VariableRecordBase>> prototypes;
prototypes.push_back(std::make_unique<Config>());
prototypes.push_back(std::make_unique<Setting>());

// 리포지토리 생성
FdFile::VariableFileRepositoryImpl repo("config.txt", std::move(prototypes), ec);
```

### 기본 CRUD

```cpp
// 생성
Config dbHost("database.host", "localhost", 1);
repo.save(dbHost, ec);

// ID로 읽기
auto found = repo.findById("database.host", ec);
if (found) {
    auto* config = dynamic_cast<Config*>(found.get());
    if (config) {
        std::cout << config->value << std::endl;
    }
}

// 전체 읽기
auto all = repo.findAll(ec);

// 수정 (같은 ID)
dbHost.value = "192.168.1.100";
dbHost.version = 2;
repo.save(dbHost, ec);

// 삭제
repo.deleteById("database.host", ec);
```

### 타입별 쿼리

```cpp
// 특정 타입의 모든 레코드 찾기
auto configs = repo.findAllByType<Config>(ec);

// ID와 타입으로 찾기
auto found = repo.findByIdAndType<Config>("database.host", ec);
```

## 고정 레코드와 비교

| 측면 | 고정 | 가변 |
|------|------|------|
| 저장 | 바이너리 | 텍스트 |
| 조회 | O(1) | O(N) |
| 유연성 | 낮음 | 높음 |
| 스키마 변경 | 마이그레이션 필요 | 쉬움 |
| 사람이 읽기 | 불가 | 가능 |
| 다형성 | CRTP (컴파일타임) | 가상 (런타임) |

### 가변 레코드를 사용해야 할 때

- 스키마가 시간이 지나면서 변경될 수 있을 때
- 사람이 읽을 수 있는 저장이 선호될 때
- 하나의 파일에 여러 레코드 타입
- 성능보다 유연성이 중요할 때

### 고정 레코드를 사용해야 할 때

- 고성능이 필요할 때
- 안정적인 스키마
- 파일당 단일 레코드 타입
- 메모리 효율성이 중요할 때

## 다음 단계

- [고정 길이 레코드](fixed-records.md) - 고성능이 필요할 때
- [리포지토리 패턴](repository-pattern.md) - 고급 작업
- [API 레퍼런스](api-reference.md) - 전체 문서
