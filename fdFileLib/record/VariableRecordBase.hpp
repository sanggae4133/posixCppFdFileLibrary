/**
 * @file fdFileLib/record/VariableRecordBase.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
#pragma once
/// @file VariableRecordBase.hpp
/// @brief 가변 길이 레코드 베이스 (구 TextRecordBase)

#include "RecordBase.hpp"

#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace FdFile {

/// @brief 가변 길이 텍스트 레코드 (JSON 스타일 Key-Value)
/// @brief 가변 길이 텍스트 레코드 베이스 클래스 (구 TextRecordBase)
/// @details JSON 스타일의 Key-Value 쌍으로 데이터를 직렬화/역직렬화하는 가상 인터페이스를
/// 제공합니다.
///          이 클래스를 상속받아 구체적인 가변 길이 레코드를 구현해야 합니다.
class VariableRecordBase : public RecordBase {
  public:
    /// @brief 가상 소멸자
    /// @details 다형성을 위해 가상 소멸자를 기본값으로 정의합니다.
    virtual ~VariableRecordBase() = default;

    /// @brief 객체의 데이터를 Key-Value 쌍으로 직렬화합니다.
    /// @details 파생 클래스에서 이 메서드를 구현하여 객체의 멤버 변수들을 Key-Value 형태로 변환해야
    /// 합니다.
    /// @param[out] out 직렬화된 데이터가 저장될 벡터.
    ///                 각 요소는 {Key, {IsString, Value}} 형태입니다.
    ///                 - Key: 필드 이름
    ///                 - IsString: 값이 문자열 타입이면 true, 숫자 등 다른 타입이면 false
    ///                 - Value: 문자열로 변환된 값
    virtual void
    toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const = 0;

    /// @brief Key-Value 쌍에서 데이터를 역직렬화하여 객체에 설정합니다.
    /// @details 파생 클래스에서 이 메서드를 구현하여 Key-Value 데이터로부터 객체의 멤버 변수를
    /// 복원해야 합니다.
    /// @param[in] kv Key-Value 데이터 맵. {Key, {IsString, Value}} 형태입니다.
    /// @param[out] ec 에러 발생 시 설정될 에러 코드
    /// @return 역직렬화 성공 여부 (true: 성공, false: 실패)
    virtual bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                        std::error_code& ec) = 0;

    /// @brief 객체 복제 (Deep Copy)
    /// @details 현재 객체의 복사본을 생성하여 반환합니다.
    /// @return 복제된 객체를 관리하는 std::unique_ptr
    virtual std::unique_ptr<RecordBase> clone() const = 0;
};

} // namespace FdFile
