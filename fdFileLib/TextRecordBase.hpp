#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <system_error>

namespace FdFile {

// Flat key-value 레코드 베이스
class TextRecordBase {
public:
    virtual ~TextRecordBase() = default;
    virtual const char* typeName() const = 0;

    // write용: 순서가 중요하면 vector 순서대로 출력됨
    // value: (is_string, raw_value_without_quotes_if_string)
    virtual void toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const = 0;

    // read용: key -> (is_string, raw_value)
    virtual bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                         std::error_code& ec) = 0;
};

} // namespace FdFile