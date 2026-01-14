#pragma once
/// @file VariableRecordBase.hpp
/// @brief 가변 길이 레코드 베이스 (구 TextRecordBase)

#include "RecordBase.hpp"
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace FdFile {

/// @brief 가변 길이 텍스트 레코드 (JSON 스타일 Key-Value)
class VariableRecordBase : public RecordBase {
  public:
    virtual ~VariableRecordBase() = default;

    /// @brief 키-값 쌍으로 직렬화
    virtual void
    toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const = 0;

    /// @brief 키-값 쌍에서 역직렬화
    virtual bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                        std::error_code& ec) = 0;
};

} // namespace FdFile
