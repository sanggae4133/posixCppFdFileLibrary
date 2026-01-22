#pragma once
/// @file FieldMeta.hpp
/// @brief C++17 tuple-based field metadata utilities and macros

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <tuple>
#include <type_traits>

namespace FdFile {

/// @brief Maximum digits for int64_t (20 characters: sign + 19 digits)
constexpr size_t INT64_FIELD_LEN = 20;

// =============================================================================
// FieldMeta Template
// =============================================================================

/// @brief Field metadata structure
/// @tparam Len Field length in bytes
/// @tparam IsStr True if string field, false if numeric
template <size_t Len, bool IsStr> struct FieldMeta {
    const char* name;   ///< Field name
    void* ptr;          ///< Pointer to field data

    static constexpr size_t length = Len;       ///< Field length
    static constexpr bool isString = IsStr;     ///< String type flag

    /// @brief Serialize field value to buffer
    /// @param buf Output buffer (must be at least Len bytes)
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

    /// @brief Deserialize field value from buffer
    /// @param buf Input buffer
    /// @throws std::runtime_error If sign character is invalid
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

/// @brief Create FieldMeta for character array fields
/// @tparam Len Template length parameter
/// @tparam N Array size (must match Len)
/// @param name Field name
/// @param member Reference to char array member
/// @return FieldMeta instance
/// @note static_assert ensures template length matches array size
template <size_t Len, size_t N> auto makeField(const char* name, char (&member)[N]) {
    static_assert(Len == N, "FD_STR: Template length must match array size");
    return FieldMeta<Len, true>{name, static_cast<void*>(member)};
}

/// @brief Create FieldMeta for numeric fields (sign + 19 digits)
/// @tparam T Field type (must be int64_t)
/// @param name Field name
/// @param member Reference to int64_t member
/// @return FieldMeta instance
/// @note static_assert ensures member type is int64_t
template <typename T>
auto makeNumField(const char* name, T& member) {
    static_assert(std::is_same_v<std::remove_cv_t<T>, int64_t>,
                  "FD_NUM: member must be int64_t");
    return FieldMeta<INT64_FIELD_LEN, false>{name, static_cast<void*>(&member)};
}

// =============================================================================
// Tuple Utility Functions
// =============================================================================

/// @brief Define fields from tuple
template <typename Base, typename Tuple>
void defineFieldsFromTuple(Base* base, const Tuple& fields) {
    std::apply([base](const auto&... f) { (base->defineField(f.name, f.length, f.isString), ...); },
               fields);
}

/// @brief Get field value by index from tuple
template <typename Tuple> void getFieldByIndex(const Tuple& fields, size_t idx, char* buf) {
    size_t i = 0;
    std::apply([&](const auto&... f) { ((i++ == idx ? (f.get(buf), void()) : void()), ...); },
               fields);
}

/// @brief Set field value by index in tuple
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

/// @brief Declare a string field (length = array size)
/// @param member Char array member variable
#define FD_STR(member)                                                                             \
    FdFile::makeField<sizeof(member)>(#member, const_cast<char(&)[sizeof(member)]>(member))

/// @brief Declare a numeric field (length = 20, signed int64_t)
/// @param member int64_t member variable
#define FD_NUM(member) FdFile::makeNumField(#member, const_cast<int64_t&>(member))

/// @brief Macro to implement common record methods
/// @param ClassName Class name
/// @param TypeNameStr Type name string
/// @param TypeLen Type field length
/// @param IdLen ID field length
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
