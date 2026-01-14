#pragma once
/// @file B.hpp
/// @brief 예제 레코드 타입 B

#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "../../fdFileLib/TextRecordBase.hpp"
#include "../../fdFileLib/textFormatUtil.hpp"

/// @brief 예제 레코드 타입 B (name, id, pw 필드)
class B : public FdFile::TextRecordBase {
  public:
    std::string name;
    long userId = 0;
    std::string pw;

    /// @brief 기본 생성자
    B() = default;

    /// @brief 값 초기화 생성자
    B(std::string name, long userId, std::string pw)
        : name(std::move(name)), userId(userId), pw(std::move(pw)) {}

    std::string id() const override { return std::to_string(userId); }

    const char* typeName() const override { return "B"; }

    std::unique_ptr<TextRecordBase> clone() const override { return std::make_unique<B>(*this); }

    void
    toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"name", {true, name}});
        out.push_back({"id", {false, std::to_string(userId)}});
        out.push_back({"pw", {true, pw}});
    }

    bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                std::error_code& ec) override {
        ec.clear();

        for (const auto& e : kv) {
            if (e.first != "name" && e.first != "id" && e.first != "pw") {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }
        }

        auto n = kv.find("name");
        auto i = kv.find("id");
        auto p = kv.find("pw");
        if (n == kv.end() || i == kv.end() || p == kv.end()) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }
        if (!n->second.first || i->second.first || !p->second.first) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        name = n->second.second;
        pw = p->second.second;

        long tmp = 0;
        if (!FdFile::util::parseLongStrict(i->second.second, tmp, ec))
            return false;
        userId = tmp;
        return true;
    }
};
