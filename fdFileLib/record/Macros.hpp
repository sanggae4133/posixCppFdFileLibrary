#pragma once
/// @file Macros.hpp
/// @brief FixedRecordBase 구현을 돕는 매크로 (Variadic & Inline Definition)

#include "FixedRecordBase.hpp"
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

// =============================================================================
// Internal Helpers for Token Pasting
// =============================================================================
#define FD_CAT_IMP(a, b) a##b
#define FD_CAT(a, b) FD_CAT_IMP(a, b)

// =============================================================================
// Get Value Implementation
// =============================================================================
#define FD_GET_IMPL_true(buf, member, len) std::memcpy(buf, member, (len));

#define FD_GET_IMPL_false(buf, member, len)                                                        \
    std::snprintf(buf, (len) + 1, "%0*ld", (int)(len), static_cast<long>(member));

#define FD_X_GET_VAL(key, member, len, isStr)                                                      \
    case FIELD_IDX_##key: {                                                                        \
        FD_CAT(FD_GET_IMPL_, isStr)(buf, member, len) break;                                       \
    }

// =============================================================================
// Set Value Implementation
// =============================================================================
#define FD_SET_IMPL_true(buf, member, len)                                                         \
    std::memset(member, 0, sizeof(member));                                                        \
    std::memcpy(member, buf, (len));

#define FD_SET_IMPL_false(buf, member, len) member = std::stol(buf);

#define FD_X_SET_VAL(key, member, len, isStr)                                                      \
    case FIELD_IDX_##key: {                                                                        \
        FD_CAT(FD_SET_IMPL_, isStr)(buf, member, len) break;                                       \
    }

// =============================================================================
// Layout & Enum Helpers
// =============================================================================
#define FD_X_DEFINE_LAYOUT(key, member, len, isStr) this->defineField(#key, len, isStr);

#define FD_X_DEFINE_ENUM(key, member, len, isStr) FIELD_IDX_##key,

// =============================================================================
// FOR_EACH Implementation (Recursion for Variadic Args)
// =============================================================================

// FD_FIELD pack: (key, member, len, isStr)
// Wraps arguments in parentheses to be treated as a single argument by FOR_EACH,
// and then used as the argument list for the operation macros.
#define FD_FIELD(key, member, len, isStr) (key, member, len, isStr)

// Operation wrappers
// x is already (key, member, len, isStr), so we just append it to the macro name.
// e.g. FD_X_DEFINE_ENUM (key, member, len, isStr) -> expands correctly
#define FD_OP_ENUM(x) FD_X_DEFINE_ENUM x
#define FD_OP_LAYOUT(x) FD_X_DEFINE_LAYOUT x
#define FD_OP_GET(x) FD_X_GET_VAL x
#define FD_OP_SET(x) FD_X_SET_VAL x

// FOR_EACH Macros (Limit: 10 fields for simplicity, extend if needed)
#define FE_1(OP, x) OP(x)
#define FE_2(OP, x, ...) OP(x) FE_1(OP, __VA_ARGS__)
#define FE_3(OP, x, ...) OP(x) FE_2(OP, __VA_ARGS__)
#define FE_4(OP, x, ...) OP(x) FE_3(OP, __VA_ARGS__)
#define FE_5(OP, x, ...) OP(x) FE_4(OP, __VA_ARGS__)
#define FE_6(OP, x, ...) OP(x) FE_5(OP, __VA_ARGS__)
#define FE_7(OP, x, ...) OP(x) FE_6(OP, __VA_ARGS__)
#define FE_8(OP, x, ...) OP(x) FE_7(OP, __VA_ARGS__)
#define FE_9(OP, x, ...) OP(x) FE_8(OP, __VA_ARGS__)
#define FE_10(OP, x, ...) OP(x) FE_9(OP, __VA_ARGS__)

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME
#define FOR_EACH(OP, ...)                                                                          \
    GET_MACRO(__VA_ARGS__, FE_10, FE_9, FE_8, FE_7, FE_6, FE_5, FE_4, FE_3, FE_2,                  \
              FE_1)(OP, __VA_ARGS__)

// =============================================================================
// Main Macro
// =============================================================================

/// @brief 고정 길이 레코드 필수 메서드 및 생성자 자동 구현
///
/// @param ClassName 클래스 이름
/// @param TypeNameStr 레코드 타입 이름
/// @param TypeLen 타입 필드 길이
/// @param IdLen ID 필드 길이
/// @param ... FD_FIELD(key, member, len, isStr) 리스트
///
/// 이 매크로는 기본 생성자 ClassName() { defineLayout(); } 를 자동으로 생성함.
#define FD_FIXED_DEFS(ClassName, TypeNameStr, TypeLen, IdLen, ...)                                 \
  public:                                                                                          \
    /* 1. Default Constructor */                                                                   \
    ClassName() {                                                                                  \
        this->defineLayout();                                                                      \
    }                                                                                              \
                                                                                                   \
    /* 2. Enum for Fields */                                                                       \
    enum FieldIdx { FOR_EACH(FD_OP_ENUM, __VA_ARGS__) FIELD_COUNT };                               \
                                                                                                   \
    /* 3. Layout Method */                                                                         \
    void defineLayout() {                                                                          \
        this->defineStart();                                                                       \
        this->defineType(TypeLen);                                                                 \
        this->defineId(IdLen);                                                                     \
        FOR_EACH(FD_OP_LAYOUT, __VA_ARGS__)                                                        \
        this->defineEnd();                                                                         \
    }                                                                                              \
                                                                                                   \
    /* 4. Get Method */                                                                            \
    void getFieldValue(size_t fieldIdx, char* buf) const {                                         \
        switch (fieldIdx) {                                                                        \
            FOR_EACH(FD_OP_GET, __VA_ARGS__)                                                       \
        default:                                                                                   \
            break;                                                                                 \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    /* 5. Set Method */                                                                            \
    void setFieldValue(size_t fieldIdx, const char* buf) {                                         \
        switch (fieldIdx) {                                                                        \
            FOR_EACH(FD_OP_SET, __VA_ARGS__)                                                       \
        default:                                                                                   \
            break;                                                                                 \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    /* 6. Base Methods */                                                                          \
    const char* typeName() const {                                                                 \
        return TypeNameStr;                                                                        \
    }                                                                                              \
    std::string id() const {                                                                       \
        return id_;                                                                                \
    }                                                                                              \
    void setRecordId(const std::string& id) {                                                      \
        id_ = id;                                                                                  \
    }                                                                                              \
    std::unique_ptr<ClassName> clone() const {                                                     \
        return std::make_unique<ClassName>(*this);                                                 \
    }                                                                                              \
                                                                                                   \
  private:                                                                                         \
    std::string id_;                                                                               \
                                                                                                   \
  public:
