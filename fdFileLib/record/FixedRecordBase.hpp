#pragma once
/// @file FixedRecordBase.hpp
/// @brief 고정 길이 레코드 베이스 및 레이아웃 엔진 (CRTP Template)

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace FdFile {

/// @brief 고정 길이 바이너리 레코드 (CRTP, Zero Vtable Overhead)
/// @tparam Derived 상속받는 구체 클래스 (CRTP)
template <typename Derived> class FixedRecordBase {
  public:
    // Virtual Destructor 제거 (CRTP는 정적 다형성이므로 불필요)
    // 단, 삭제는 Derived 포인터로 해야 함.
    ~FixedRecordBase() = default;

    // =========================================================================
    // Public Interface (No Virtual)
    // =========================================================================

    /// @brief 전체 레코드 크기 반환
    size_t recordSize() const { return totalSize_; }

    /// @brief 직렬화
    bool serialize(char* buf) const {
        if (!layoutDefined_)
            return false;

        // 1. 템플릿 복사
        std::memcpy(buf, formatTemplate_.data(), totalSize_);

        // 2. Type 덮어쓰기 (Derived::typeName() 호출)
        const char* tName = static_cast<const Derived*>(this)->typeName();
        std::string tNameStr(tName);
        size_t copyLen = std::min(tNameStr.size(), typeLen_);
        std::memcpy(buf + typeOffset_, tNameStr.data(), copyLen);

        // 3. ID 덮어쓰기 (Derived::id() 호출)
        std::string idStr = static_cast<const Derived*>(this)->id();
        copyLen = std::min(idStr.size(), idLen_);
        std::memcpy(buf + idOffset_, idStr.data(), copyLen);

        // 4. 필드 데이터 덮어쓰기
        for (size_t i = 0; i < fields_.size(); ++i) {
            const auto& f = fields_[i];
            char valBuf[1024];
            if (f.length >= 1024)
                return false;

            std::memset(valBuf, 0, f.length);
            // Derived::getFieldValue 호출
            static_cast<const Derived*>(this)->getFieldValue(i, valBuf);

            std::memcpy(buf + f.offset, valBuf, f.length);
        }
        return true;
    }

    /// @brief 역직렬화
    bool deserialize(const char* buf, std::error_code& ec) {
        if (!layoutDefined_) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        char tmpBuf[1024];

        // 1. ID 읽기
        if (idLen_ >= 1024)
            return false;
        std::memset(tmpBuf, 0, sizeof(tmpBuf));
        std::memcpy(tmpBuf, buf + idOffset_, idLen_);
        // Derived::setRecordId 호출
        static_cast<Derived*>(this)->setRecordId(std::string(tmpBuf));

        // 2. 필드 읽기
        for (size_t i = 0; i < fields_.size(); ++i) {
            const auto& f = fields_[i];
            if (f.length >= 1024)
                return false;

            std::memcpy(tmpBuf, buf + f.offset, f.length);
            // Derived::setFieldValue 호출
            static_cast<Derived*>(this)->setFieldValue(i, tmpBuf);
        }
        return true;
    }

  protected:
    struct FieldInfo {
        std::string key;
        size_t offset;
        size_t length;
        bool isString;
    };

    // Layout Helpers
    void defineStart() {
        fields_.clear();
        layoutDefined_ = false;
        formatTemplate_.clear();
        totalSize_ = 0;
    }

    void defineType(size_t len) {
        typeOffset_ = totalSize_;
        typeLen_ = len;
        totalSize_ += len;
        formatTemplate_.append(len, '\0');
    }

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

    void defineEnd() {
        totalSize_ += 1;
        formatTemplate_ += "}";
        layoutDefined_ = true;
    }

  private:
    std::string formatTemplate_;
    std::vector<FieldInfo> fields_;
    size_t typeOffset_ = 0;
    size_t typeLen_ = 0;
    size_t idOffset_ = 0;
    size_t idLen_ = 0;
    size_t totalSize_ = 0;
    bool layoutDefined_ = false;
};

} // namespace FdFile
