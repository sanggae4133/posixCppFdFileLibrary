/**
 * @file fdFileLib/record/RecordBase.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
#pragma once
/// @file RecordBase.hpp
/// @brief 레코드 최상위 인터페이스

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
};

} // namespace FdFile
