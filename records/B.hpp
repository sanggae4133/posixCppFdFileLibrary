#pragma once

#include "../fdFileLib/TextRecordBase.hpp"
#include "../fdFileLib/textFormatUtil.hpp"

#include <vector>
#include <unordered_map>
#include <system_error>

class B : public FdFile::TextRecordBase {
public:
    std::string name;
    long id = 0;
    std::string pw;

    const char* typeName() const override { return "B"; }

    void toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"name", {true, name}});
        out.push_back({"id", {false, std::to_string(id)}});
        out.push_back({"pw", {true, pw}});
    }

    bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                 std::error_code& ec) override {
        ec.clear();

        // ✅ unknown key 에러
        for (const auto& e : kv) {
            if (e.first != "name" && e.first != "id" && e.first != "pw") {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }
        }

        auto n = kv.find("name");
        auto i = kv.find("id");
        auto p = kv.find("pw");
        if (n == kv.end() || i == kv.end() || p == kv.end()) { ec = std::make_error_code(std::errc::invalid_argument); return false; }
        if (!n->second.first || i->second.first || !p->second.first) { ec = std::make_error_code(std::errc::invalid_argument); return false; }

        name = n->second.second;
        pw = p->second.second;

        long tmp = 0;
        if (!FdFile::util::flatfmt::parseLongStrict(i->second.second, tmp, ec)) return false;
        id = tmp;
        return true;
    }
};