#pragma once
/// @file B.hpp
/// @brief Variable Length Record B

#include "../../fdFileLib/record/VariableRecordBase.hpp"
#include "../../fdFileLib/util/textFormatUtil.hpp"

class B : public FdFile::VariableRecordBase {
  public:
    std::string name;
    long userId = 0;
    std::string pw;

    B() = default;
    B(std::string name, long id, std::string pw)
        : name(std::move(name)), userId(id), pw(std::move(pw)) {}

    std::string id() const override { return std::to_string(userId); }
    const char* typeName() const override { return "B"; }

    std::unique_ptr<FdFile::RecordBase> clone() const override {
        return std::make_unique<B>(*this);
    }

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
        auto n = kv.find("name");
        auto i = kv.find("id");
        auto p = kv.find("pw");
        if (n == kv.end() || i == kv.end() || p == kv.end())
            return false;

        name = n->second.second;
        pw = p->second.second;
        long tmp = 0;
        if (!FdFile::util::parseLongStrict(i->second.second, tmp, ec))
            return false;
        userId = tmp;
        return true;
    }
};
