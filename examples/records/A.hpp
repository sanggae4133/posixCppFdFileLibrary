#pragma once

#include <limits>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "../../fdFileLib/TextRecordBase.hpp"
#include "../../fdFileLib/textFormatUtil.hpp"

class A : public FdFile::TextRecordBase {
public:
    std::string name;
    long id = 0;

    const char* typeName() const override { return "A"; }

    void toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"name", {true, name}});
        out.push_back({"id", {false, std::to_string(id)}});
    }

    bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                std::error_code& ec) override {
        ec.clear();

        // unknown key -> error
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
        if (!FdFile::util::parseLongStrict(i->second.second, tmp, ec)) return false;
        id = tmp;
        return true;
    }
};

