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
            std::memcpy(buf, ptr, Len);
        } else {
            int64_t val = *static_cast<int64_t*>(ptr);
            // Format: +0000000000000000025 or -0000000000000000025 (sign + 19 digits = 20 chars)
            char sign = (val >= 0) ? '+' : '-';
            // Use unsigned to avoid overflow when val == INT64_MIN (-9223372036854775808)
            // -INT64_MIN would overflow, but casting to unsigned first avoids this
            uint64_t absVal =
                (val >= 0) ? static_cast<uint64_t>(val) : static_cast<uint64_t>(-(val + 1)) + 1;
            std::snprintf(buf, Len + 1, "%c%019llu", sign, static_cast<unsigned long long>(absVal));
        }
    }

    /// @brief 버퍼에서 값을 읽어 멤버에 설정
    /// @throws std::runtime_error 유효하지 않은 부호 문자인 경우
    void set(const char* buf) {
        if constexpr (IsStr) {
            std::memset(ptr, 0, Len);
            std::memcpy(ptr, buf, Len);
        } else {
            // Validate sign character
            char sign = buf[0];
            if (sign != '+' && sign != '-') {
                throw std::runtime_error("Invalid numeric field: sign must be '+' or '-'");
            }
            // Parse the numeric part as unsigned (to handle INT64_MIN's absolute value)
            uint64_t absVal = std::stoull(buf + 1);
            if (sign == '-') {
                // Handle INT64_MIN specially: -(uint64_t)9223372036854775808
                // can't be done directly, so we cast through unsigned
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
template <size_t Len, size_t N> auto makeField(const char* name, char (&member)[N]) {
    return FieldMeta<Len, true>{name, static_cast<void*>(member)};
}

/// @brief 숫자 필드용 FieldMeta 생성 (부호 포함 20자리)
inline auto makeNumField(const char* name, int64_t& member) {
    return FieldMeta<INT64_FIELD_LEN, false>{name, static_cast<void*>(&member)};
}

// =============================================================================
// Tuple Utility Functions
// =============================================================================

template <typename Base, typename Tuple>
void defineFieldsFromTuple(Base* base, const Tuple& fields) {
    std::apply([base](const auto&... f) { (base->defineField(f.name, f.length, f.isString), ...); },
               fields);
}

template <typename Tuple> void getFieldByIndex(const Tuple& fields, size_t idx, char* buf) {
    size_t i = 0;
    std::apply([&](const auto&... f) { ((i++ == idx ? (f.get(buf), void()) : void()), ...); },
               fields);
}

template <typename Tuple> void setFieldByIndex(const Tuple& fields, size_t idx, const char* buf) {
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
        this->defineStart();                                                                       \
        this->defineType(TypeLen);                                                                 \
        this->defineId(IdLen);                                                                     \
        FdFile::defineFieldsFromTuple(this, fields());                                             \
        this->defineEnd();                                                                         \
    }
