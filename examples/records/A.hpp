#pragma once
/// @file A.hpp
/// @brief 예제 레코드 타입 A

#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "../../fdFileLib/TextRecordBase.hpp"
#include "../../fdFileLib/textFormatUtil.hpp"

/// @brief 예제 레코드 타입 A (name, id 필드)
class A : public FdFile::TextRecordBase {
  public:
    std::string name;
    long userId = 0;

    /// @brief 기본 생성자
    A() = default;

    /// @brief 값 초기화 생성자
    A(std::string name, long userId) : name(std::move(name)), userId(userId) {}

    std::string id() const override { return std::to_string(userId); }

    const char* typeName() const override { return "A"; }

    std::unique_ptr<TextRecordBase> clone() const override { return std::make_unique<A>(*this); }

    void
    toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"name", {true, name}});
        out.push_back({"id", {false, std::to_string(userId)}});
    }

    bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                std::error_code& ec) override {
        ec.clear();

        for (const auto& e : kv) {
            if (e.first != "name" && e.first != "id") {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }
        }

        auto n = kv.find("name");
        auto i = kv.find("id");
        if (n == kv.end() || i == kv.end()) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }
        if (!n->second.first || i->second.first) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        name = n->second.second;

        long tmp = 0;
        if (!FdFile::util::parseLongStrict(i->second.second, tmp, ec))
            return false;
        userId = tmp;
        return true;
    }
};
