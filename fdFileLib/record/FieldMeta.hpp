/**
 * @file fdFileLib/record/FieldMeta.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
#pragma once
/// @file FieldMeta.hpp
/// @brief C++17 tuple 기반 필드 메타데이터 유틸리티 + 간단한 매크로

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <tuple>
#include <type_traits>

namespace FdFile {

/// @brief int64_t의 최대 자리수 (부호 포함 20자리: +/- + 19자리)
constexpr size_t INT64_FIELD_LEN = 20;

// =============================================================================
// FieldMeta Template
// =============================================================================

/// @brief 필드 메타데이터 구조체
template <size_t Len, bool IsStr> struct FieldMeta {
    const char* name;
    void* ptr;

    static constexpr size_t length = Len;
    static constexpr bool isString = IsStr;

    void get(char* buf) const {
        if constexpr (IsStr) {
            // 고정 길이 문자열 필드는 원본 바이트를 그대로 복사한다.
            // padding/zero-fill 정책은 상위 레코드 초기화 로직에서 책임진다.
            std::memcpy(buf, ptr, Len);
        } else {
            int64_t val = *static_cast<int64_t*>(ptr);
            // 숫자 필드는 부호 + 19자리 zero-padding 고정 포맷으로 직렬화한다.
            // 예) +0000000000000000025, -0000000000000000025
            char sign = (val >= 0) ? '+' : '-';
            // INT64_MIN 절대값 처리 시 overflow를 피하기 위해 unsigned 경유 계산을 사용한다.
            uint64_t absVal =
                (val >= 0) ? static_cast<uint64_t>(val) : static_cast<uint64_t>(-(val + 1)) + 1;
            std::snprintf(buf, Len + 1, "%c%019llu", sign, static_cast<unsigned long long>(absVal));
        }
    }

    /// @brief 버퍼에서 값을 읽어 멤버에 설정
    /// @throws std::runtime_error 유효하지 않은 부호 문자인 경우
    void set(const char* buf) {
        if constexpr (IsStr) {
            // 역직렬화 시 기존 버퍼를 먼저 0으로 지워 trailing garbage를 제거한다.
            std::memset(ptr, 0, Len);
            std::memcpy(ptr, buf, Len);
        } else {
            // 숫자 필드는 첫 글자 부호 검증을 강제해 손상된 레코드를 조기에 차단한다.
            char sign = buf[0];
            if (sign != '+' && sign != '-') {
                throw std::runtime_error("Invalid numeric field: sign must be '+' or '-'");
            }
            // 절대값 파트는 unsigned로 파싱하여 INT64_MIN 경계값을 안전하게 다룬다.
            uint64_t absVal = std::stoull(buf + 1);
            if (sign == '-') {
                // 음수 복원 시에도 overflow 없는 경로를 유지한다.
                *static_cast<int64_t*>(ptr) = -static_cast<int64_t>(absVal);
            } else {
                *static_cast<int64_t*>(ptr) = static_cast<int64_t>(absVal);
            }
        }
    }
};

// =============================================================================
// Helper Functions
// =============================================================================

/// @brief 문자 배열 필드용 FieldMeta 생성
/// @note static_assert: 템플릿 길이(Len)와 배열 크기(N)가 일치해야 함
template <size_t Len, size_t N> auto makeField(const char* name, char (&member)[N]) {
    static_assert(Len == N, "FD_STR: Template length must match array size");
    return FieldMeta<Len, true>{name, static_cast<void*>(member)};
}

/// @brief 숫자 필드용 FieldMeta 생성 (부호 포함 20자리)
/// @note static_assert: 멤버 타입이 int64_t여야 함
template <typename T>
auto makeNumField(const char* name, T& member) {
    static_assert(std::is_same_v<std::remove_cv_t<T>, int64_t>,
                  "FD_NUM: member must be int64_t");
    return FieldMeta<INT64_FIELD_LEN, false>{name, static_cast<void*>(&member)};
}

// =============================================================================
// Tuple Utility Functions
// =============================================================================

template <typename Base, typename Tuple>
void defineFieldsFromTuple(Base* base, const Tuple& fields) {
    // tuple에 선언된 필드를 순회하며 레이아웃 엔진(FixedRecordBase)에 필드 정의를 등록한다.
    // fold expression을 사용해 런타임 반복문 없이 컴파일타임에 호출 시퀀스를 생성한다.
    std::apply([base](const auto&... f) { (base->defineField(f.name, f.length, f.isString), ...); },
               fields);
}

template <typename Tuple> void getFieldByIndex(const Tuple& fields, size_t idx, char* buf) {
    // 인덱스 기반 접근을 유지해 serialize 루프에서 조건 분기 비용을 최소화한다.
    size_t i = 0;
    std::apply([&](const auto&... f) { ((i++ == idx ? (f.get(buf), void()) : void()), ...); },
               fields);
}

template <typename Tuple> void setFieldByIndex(const Tuple& fields, size_t idx, const char* buf) {
    // tuple 요소가 const로 전달되더라도 set 호출이 가능하도록 const_cast를 사용한다.
    // 이 경로는 내부 구현 전용이며, 외부 API로는 노출되지 않는다.
    size_t i = 0;
    std::apply(
        [&](const auto&... f) {
            ((i++ == idx
                  ? (const_cast<std::remove_const_t<std::remove_reference_t<decltype(f)>>&>(f).set(
                         buf),
                     void())
                  : void()),
             ...);
        },
        fields);
}

} // namespace FdFile

// =============================================================================
// Simple Macros for Record Definition
// =============================================================================

/// @brief 문자열 필드 선언 (길이 = 배열 크기)
#define FD_STR(member)                                                                             \
    FdFile::makeField<sizeof(member)>(#member, const_cast<char(&)[sizeof(member)]>(member))

/// @brief 숫자 필드 선언 (길이 = 20, 부호 포함 int64_t)
#define FD_NUM(member) FdFile::makeNumField(#member, const_cast<int64_t&>(member))

/// @brief 레코드 공통 메서드 구현 매크로
/// @param ClassName 클래스명
/// @param TypeNameStr 타입 이름 문자열
/// @param TypeLen 타입 필드 길이
/// @param IdLen ID 필드 길이
#define FD_RECORD_IMPL(ClassName, TypeNameStr, TypeLen, IdLen)                                     \
  public:                                                                                          \
    ClassName() {                                                                                  \
        /* 생성 즉시 멤버 초기화 + 레이아웃 정의를 완료해 serialize/deserialize를 즉시 가능 상태로 만든다. */ \
        this->initMembers();                                                                       \
        this->defineLayout();                                                                      \
    }                                                                                              \
    void __getFieldValue(size_t idx, char* buf) const {                                            \
        FdFile::getFieldByIndex(fields(), idx, buf);                                               \
    }                                                                                              \
    void __setFieldValue(size_t idx, const char* buf) {                                            \
        FdFile::setFieldByIndex(fields(), idx, buf);                                               \
    }                                                                                              \
    const char* typeName() const {                                                                 \
        return TypeNameStr;                                                                        \
    }                                                                                              \
    std::string getId() const {                                                                    \
        return id_;                                                                                \
    }                                                                                              \
    void setId(const std::string& id) {                                                            \
        id_ = id;                                                                                  \
    }                                                                                              \
                                                                                                   \
  private:                                                                                         \
    std::string id_;                                                                               \
    void defineLayout() {                                                                          \
        /* 레이아웃 정의 순서(타입 -> id -> 사용자 필드)는 파일 포맷 호환성의 핵심 계약이다. */             \
        this->defineStart();                                                                       \
        this->defineType(TypeLen);                                                                 \
        this->defineId(IdLen);                                                                     \
        FdFile::defineFieldsFromTuple(this, fields());                                             \
        this->defineEnd();                                                                         \
    }
