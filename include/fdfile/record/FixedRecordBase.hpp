#pragma once
/// @file FixedRecordBase.hpp
/// @brief Fixed-length record base class with layout engine (CRTP Template)

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

namespace FdFile {

/// @brief Fixed-length binary record base class (CRTP, Zero Vtable Overhead)
/// @details Uses CRTP (Curiously Recurring Template Pattern) for compile-time polymorphism.
///          Provides high-performance serialization/deserialization without vtable overhead.
///          Record layout must be defined in constructor via `defineLayout()`.
/// @tparam Derived Concrete class inheriting from this base (CRTP derived class)
template <typename Derived> class FixedRecordBase {
    // Allow FieldMeta utilities to access protected members
    template <typename Base, typename Tuple>
    friend void defineFieldsFromTuple(Base* base, const Tuple& fields);

  public:
    /// @brief Destructor
    /// @details Not virtual since CRTP pattern is used, but safe because objects
    ///          are typically managed through Derived class pointers.
    ~FixedRecordBase() = default;

  protected:
    /// @brief Default constructor (with CRTP type validation)
    /// @note static_assert: Derived must inherit from FixedRecordBase<Derived>
    FixedRecordBase() {
        static_assert(std::is_base_of_v<FixedRecordBase<Derived>, Derived>,
                      "CRTP: Derived must inherit from FixedRecordBase<Derived>");
    }

    // =========================================================================
    // Public Interface (No Virtual)
    // =========================================================================
  public:
    /// @brief Returns total record size
    /// @return Total bytes of the record (includes type, ID, and field data)
    size_t recordSize() const { return totalSize_; }

    /// @brief Serialize object to binary data
    /// @details Copies Type, ID, and Field data to buffer according to predefined layout.
    ///          Statically calls `typeName()`, `id()`, `getFieldValue()` from derived class.
    /// @param[out] buf Buffer to store serialized data. Must be at least `recordSize()` bytes.
    /// @return true on success, false if layout undefined or buffer too small
    bool serialize(char* buf) const {
        if (!layoutDefined_)
            return false;

        // 1. Copy template
        std::memcpy(buf, formatTemplate_.data(), totalSize_);

        // 2. Overwrite Type (calls Derived::typeName())
        const char* tName = static_cast<const Derived*>(this)->typeName();
        std::string tNameStr(tName);
        size_t copyLen = std::min(tNameStr.size(), typeLen_);
        std::memcpy(buf + typeOffset_, tNameStr.data(), copyLen);

        // 3. Overwrite ID (calls Derived::getId())
        std::string idStr = static_cast<const Derived*>(this)->getId();
        copyLen = std::min(idStr.size(), idLen_);
        std::memcpy(buf + idOffset_, idStr.data(), copyLen);

        // 4. Overwrite field data
        for (size_t i = 0; i < fields_.size(); ++i) {
            const auto& f = fields_[i];
            char valBuf[1024];
            if (f.length >= 1024)
                return false;

            std::memset(valBuf, 0, f.length);
            // Call Derived::__getFieldValue
            static_cast<const Derived*>(this)->__getFieldValue(i, valBuf);

            std::memcpy(buf + f.offset, valBuf, f.length);
        }
        return true;
    }

    /// @brief Deserialize binary data to object
    /// @details Reads data from buffer and parses Type, ID, Field values.
    ///          Statically calls `setRecordId()`, `setFieldValue()` from derived class.
    /// @param[in] buf Buffer containing serialized data.
    /// @param[out] ec Error code set on failure (e.g., `std::errc::invalid_argument`)
    /// @return true on success, false on failure
    bool deserialize(const char* buf, std::error_code& ec) {
        if (!layoutDefined_) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        char tmpBuf[1024];

        // 1. Read ID
        if (idLen_ >= 1024)
            return false;
        std::memset(tmpBuf, 0, sizeof(tmpBuf));
        std::memcpy(tmpBuf, buf + idOffset_, idLen_);
        // Call Derived::setId
        static_cast<Derived*>(this)->setId(std::string(tmpBuf));

        // 2. Read fields
        for (size_t i = 0; i < fields_.size(); ++i) {
            const auto& f = fields_[i];
            if (f.length >= 1024)
                return false;

            std::memcpy(tmpBuf, buf + f.offset, f.length);
            // Call Derived::__setFieldValue
            static_cast<Derived*>(this)->__setFieldValue(i, tmpBuf);
        }
        return true;
    }

  protected:
    /// @brief Field information structure
    struct FieldInfo {
        std::string key; ///< Field key (name)
        size_t offset;   ///< Offset within record
        size_t length;   ///< Field length
        bool isString;   ///< String flag (true: string, false: numeric)
    };

    // Layout Helpers

    /// @brief Start layout definition
    /// @details Clears existing layout information.
    void defineStart() {
        fields_.clear();
        layoutDefined_ = false;
        formatTemplate_.clear();
        totalSize_ = 0;
    }

    /// @brief Define type field
    /// @param len Type field length
    void defineType(size_t len) {
        typeOffset_ = totalSize_;
        typeLen_ = len;
        totalSize_ += len;
        formatTemplate_.append(len, '\0');
    }

    /// @brief Define ID field
    /// @details Adds JSON-style delimiters (",id:\"", "\"{") around ID field.
    /// @param len ID field length
    void defineId(size_t len) {
        std::string prefix = ",id:\"";
        totalSize_ += prefix.size();
        formatTemplate_ += prefix;
        idOffset_ = totalSize_;
        idLen_ = len;
        totalSize_ += len;
        formatTemplate_.append(len, '\0');
        std::string suffix = "\"{";
        totalSize_ += suffix.size();
        formatTemplate_ += suffix;
    }

    /// @brief Define a data field
    /// @details Adds delimiters for field key-value and calculates record size.
    /// @param key Field key (name)
    /// @param valLen Field value length
    /// @param isString String flag (adds quotes around value if true)
    void defineField(const char* key, size_t valLen, bool isString) {
        if (!fields_.empty()) {
            totalSize_ += 1;
            formatTemplate_ += ",";
        }
        std::string k = key;
        std::string prefix = k + ":";
        if (isString)
            prefix += "\"";

        totalSize_ += prefix.size();
        formatTemplate_ += prefix;

        FieldInfo info;
        info.key = k;
        info.offset = totalSize_;
        info.length = valLen;
        info.isString = isString;
        fields_.push_back(info);

        totalSize_ += valLen;
        formatTemplate_.append(valLen, '\0');

        if (isString) {
            totalSize_ += 1;
            formatTemplate_ += "\"";
        }
    }

    /// @brief End layout definition
    /// @details Adds closing brace ("}") and marks layout as defined.
    void defineEnd() {
        totalSize_ += 1;
        formatTemplate_ += "}";
        layoutDefined_ = true;
    }

  private:
    std::string formatTemplate_;    ///< Serialization template (pre-calculated delimiters)
    std::vector<FieldInfo> fields_; ///< Field information list
    size_t typeOffset_ = 0;         ///< Type field offset
    size_t typeLen_ = 0;            ///< Type field length
    size_t idOffset_ = 0;           ///< ID field offset
    size_t idLen_ = 0;              ///< ID field length
    size_t totalSize_ = 0;          ///< Total record size
    bool layoutDefined_ = false;    ///< Layout definition complete flag
};

} // namespace FdFile
