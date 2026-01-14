#pragma once
/// @file TextRecordBase.hpp
/// @brief 텍스트 파일에 저장 가능한 레코드의 추상 베이스 클래스

#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace FdFile {

/// @brief 텍스트 파일에 저장 가능한 레코드의 추상 베이스 클래스
///
/// 이 클래스를 상속받아 커스텀 레코드 타입을 정의한다.
/// 각 레코드는 키-값 쌍으로 직렬화/역직렬화된다.
///
/// @code
/// class User : public FdFile::TextRecordBase {
/// public:
///     std::string name;
///     long id = 0;
///
///     std::string id() const override { return std::to_string(id); }
///     const char* typeName() const override { return "User"; }
///     std::unique_ptr<TextRecordBase> clone() const override {
///         return std::make_unique<User>(*this);
///     }
///     void toKv(...) const override { /* 직렬화 */ }
///     bool fromKv(...) override { /* 역직렬화 */ }
/// };
/// @endcode
class TextRecordBase {
  public:
    virtual ~TextRecordBase() = default;

    /// @brief 레코드 고유 식별자 반환
    /// @return ID 문자열 (각 레코드 타입에서 정의)
    /// @note Repository의 findById(), deleteById() 등에서 사용
    virtual std::string id() const = 0;

    /// @brief 레코드 타입 이름을 반환
    /// @return 타입 식별 문자열 (예: "User", "Order")
    /// @note 이 값이 파일에 저장되는 타입 식별자로 사용된다
    virtual const char* typeName() const = 0;

    /// @brief 객체를 복제하여 새 인스턴스 생성
    /// @return 복제된 객체의 unique_ptr
    /// @note 읽기 시 프로토타입에서 복제하여 사용
    virtual std::unique_ptr<TextRecordBase> clone() const = 0;

    /// @brief 레코드를 키-값 쌍으로 직렬화
    /// @param[out] out 출력 벡터. 각 요소는 {키, {문자열여부, 값}} 형태
    ///   - 문자열여부가 true면 값이 따옴표로 감싸져 출력됨
    ///   - 벡터 순서대로 파일에 출력됨
    /// @note 호출 전 out.clear()를 호출해야 함
    virtual void
    toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const = 0;

    /// @brief 키-값 쌍에서 레코드를 역직렬화
    /// @param kv 입력 맵. 각 요소는 {키, {문자열여부, 값}} 형태
    /// @param ec 에러 코드 (파싱 실패 시 설정)
    /// @return 성공 시 true
    virtual bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                        std::error_code& ec) = 0;
};

} // namespace FdFile