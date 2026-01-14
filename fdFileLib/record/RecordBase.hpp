#pragma once
/// @file RecordBase.hpp
/// @brief 레코드 최상위 인터페이스

#include <memory>
#include <string>

namespace FdFile {

/// @brief 파일에 저장되는 모든 레코드의 최상위 인터페이스
class RecordBase {
  public:
    virtual ~RecordBase() = default;

    /// @brief 레코드 고유 식별자 반환
    virtual std::string id() const = 0;

    /// @brief 레코드 타입 이름 반환
    virtual const char* typeName() const = 0;

    /// @brief 객체 복제
    virtual std::unique_ptr<RecordBase> clone() const = 0;
};

} // namespace FdFile
