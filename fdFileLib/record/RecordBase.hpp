#pragma once
/// @file RecordBase.hpp
/// @brief 레코드 최상위 인터페이스

#include <memory>
#include <string>

namespace FdFile {

/// @brief 파일에 저장되는 모든 레코드의 최상위 인터페이스
/// @details 본 라이브러리에서 관리하는 모든 레코드 타입(고정 길이, 가변 길이)이 이 인터페이스를
/// 상속받아야 합니다.
///          레코드의 공통적인 기능(ID 조회, 타입 이름 조회, 복제)을 정의합니다.
class RecordBase {
public:
    /// @brief 가상 소멸자
    /// @details 다형성을 지원하기 위해 가상 소멸자를 기본값으로 정의합니다.
    virtual ~RecordBase() = default;

    /// @brief 레코드 고유 식별자 반환
    /// @return 레코드의 ID 문자열. 모든 레코드는 고유한 ID를 가져야 합니다.
    virtual std::string id() const = 0;

    /// @brief 레코드 타입 이름 반환
    /// @return 레코드의 타입 이름 (C-style string). 파일 저장 시 메타데이터로 사용될 수 있습니다.
    virtual const char* typeName() const = 0;

    /// @brief 객체 복제 (Deep Copy)
    /// @details 현재 객체의 복사본을 생성하여 반환합니다.
    /// @return 복제된 객체를 관리하는 std::unique_ptr
    virtual std::unique_ptr<RecordBase> clone() const = 0;
};

} // namespace FdFile
