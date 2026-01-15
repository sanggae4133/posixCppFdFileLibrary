#pragma once
/// @file FixedRecordBase.hpp
/// @brief 고정 길이 레코드 베이스 및 레이아웃 엔진 (CRTP Template)

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

namespace FdFile {

/// @brief 고정 길이 바이너리 레코드 (CRTP, Zero Vtable Overhead)
/// @details CRTP(Curiously Recurring Template Pattern)를 사용하여 컴파일 타임에 다형성을 구현한
/// 고정 길이 레코드 베이스 클래스입니다.
///          가상 함수 테이블(vtable) 오버헤드 없이 고성능의 직렬화/역직렬화를 지원합니다.
///          레코드의 레이아웃(필드 배치)은 생성자에서 `defineLayout()`을 통해 정의해야 합니다.
/// @tparam Derived 상속받는 구체 클래스 (CRTP 파생 클래스)
template <typename Derived> class FixedRecordBase {
    // Allow FieldMeta utilities to access protected members
    template <typename Base, typename Tuple>
    friend void defineFieldsFromTuple(Base* base, const Tuple& fields);

  public:
    /// @brief 소멸자
    /// @details CRTP 패턴이므로 가상 소멸자가 아니지만, 일반적으로 Derived 클래스 포인터로
    /// 관리되므로 안전합니다.
    ///          메모리 누수를 방지하기 위해 default 소멸자를 명시합니다.
    ~FixedRecordBase() = default;

  protected:
    /// @brief 기본 생성자 (CRTP 타입 검증 포함)
    /// @note static_assert: Derived가 FixedRecordBase<Derived>를 상속해야 함
    FixedRecordBase() {
        static_assert(std::is_base_of_v<FixedRecordBase<Derived>, Derived>,
                      "CRTP: Derived must inherit from FixedRecordBase<Derived>");
    }

    // =========================================================================
    // Public Interface (No Virtual)
    // =========================================================================
  public:
    /// @brief 전체 레코드 크기 반환
    /// @return 레코드의 총 바이트 수 (타입, ID, 필드 데이터 포함)
    size_t recordSize() const { return totalSize_; }

    /// @brief 객체를 바이너리 데이터로 직렬화
    /// @details 미리 정의된 레이아웃(layoutDefined_)에 따라 Type, ID, Field 데이터를 버퍼에
    /// 복사합니다.
    ///          파생 클래스의 `typeName()`, `id()`, `getFieldValue()` 메서드를 정적으로 호출합니다.
    /// @param[out] buf 직렬화된 데이터를 저장할 버퍼 포인터. 최소 `recordSize()` 만큼의 크기여야
    /// 합니다.
    /// @return 직렬화 성공 여부 (true: 성공, false: 레이아웃 미정의 또는 버퍼 크기 부족 등)
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

        // 3. ID 덮어쓰기 (Derived::getId() 호출)
        std::string idStr = static_cast<const Derived*>(this)->getId();
        copyLen = std::min(idStr.size(), idLen_);
        std::memcpy(buf + idOffset_, idStr.data(), copyLen);

        // 4. 필드 데이터 덮어쓰기
        for (size_t i = 0; i < fields_.size(); ++i) {
            const auto& f = fields_[i];
            char valBuf[1024];
            if (f.length >= 1024)
                return false;

            std::memset(valBuf, 0, f.length);
            // Derived::__getFieldValue 호출
            static_cast<const Derived*>(this)->__getFieldValue(i, valBuf);

            std::memcpy(buf + f.offset, valBuf, f.length);
        }
        return true;
    }

    /// @brief 바이너리 데이터를 객체로 역직렬화
    /// @details 버퍼에서 데이터를 읽어 Type, ID, Field 값을 파싱하고 객체에 설정합니다.
    ///          파생 클래스의 `setRecordId()`, `setFieldValue()` 메서드를 정적으로 호출합니다.
    /// @param[in] buf 직렬화된 데이터가 담긴 버퍼 포인터.
    /// @param[out] ec 에러 발생 시 설정될 에러 코드 (ex: `std::errc::invalid_argument`)
    /// @return 역직렬화 성공 여부 (true: 성공, false: 실패)
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
        // Derived::setId 호출
        static_cast<Derived*>(this)->setId(std::string(tmpBuf));

        // 2. 필드 읽기
        for (size_t i = 0; i < fields_.size(); ++i) {
            const auto& f = fields_[i];
            if (f.length >= 1024)
                return false;

            std::memcpy(tmpBuf, buf + f.offset, f.length);
            // Derived::__setFieldValue 호출
            static_cast<Derived*>(this)->__setFieldValue(i, tmpBuf);
        }
        return true;
    }

  protected:
    /// @brief 필드 정보 구조체
    struct FieldInfo {
        std::string key; ///< 필드 키 (이름)
        size_t offset;   ///< 레코드 내 오프셋
        size_t length;   ///< 필드 길이
        bool isString;   ///< 문자열 여부 (true: 문자열, false: 숫자)
    };

    // Layout Helpers

    /// @brief 레이아웃 정의 시작
    /// @details 기존 레이아웃 정보를 초기화합니다.
    void defineStart() {
        fields_.clear();
        layoutDefined_ = false;
        formatTemplate_.clear();
        totalSize_ = 0;
    }

    /// @brief 타입 필드 정의
    /// @param len 타입 필드의 길이
    void defineType(size_t len) {
        typeOffset_ = totalSize_;
        typeLen_ = len;
        totalSize_ += len;
        formatTemplate_.append(len, '\0');
    }

    /// @brief ID 필드 정의
    /// @details ID 필드 앞뒤로 JSON 스타일의 구분자(",id:\"", "\"{")를 템플릿에 추가합니다.
    /// @param len ID 필드의 길이
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

    /// @brief 일반 데이터 필드 정의
    /// @details 필드 키와 값을 구분하는 구분자를 템플릿에 추가하고 레코드 크기를 계산합니다.
    /// @param key 필드 키 (이름)
    /// @param valLen 필드 값의 길이
    /// @param isString 문자열 여부 (true일 경우 값 앞뒤에 따옴표 추가)
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

    /// @brief 레이아웃 정의 종료
    /// @details JSON 닫는 중괄호("}")를 추가하고 레이아웃 정의를 완료로 표시합니다.
    void defineEnd() {
        totalSize_ += 1;
        formatTemplate_ += "}";
        layoutDefined_ = true;
    }

  private:
    std::string formatTemplate_;    ///< 직렬화 기본 템플릿 (미리 계산된 구분자 포함)
    std::vector<FieldInfo> fields_; ///< 필드 정보 목록
    size_t typeOffset_ = 0;         ///< 타입 필드 오프셋
    size_t typeLen_ = 0;            ///< 타입 필드 길이
    size_t idOffset_ = 0;           ///< ID 필드 오프셋
    size_t idLen_ = 0;              ///< ID 필드 길이
    size_t totalSize_ = 0;          ///< 전체 레코드 크기
    bool layoutDefined_ = false;    ///< 레이아웃 정의 완료 여부
};

} // namespace FdFile
