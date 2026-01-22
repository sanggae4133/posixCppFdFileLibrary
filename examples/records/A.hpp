#pragma once
/// @file A.hpp
/// @brief Variable Length Record A

#include <fdfile/record/VariableRecordBase.hpp>
#include <fdfile/util/textFormatUtil.hpp>

class A : public FdFile::VariableRecordBase {
  public:
    std::string name;
    long userId = 0;

    A() = default;
    A(std::string name, long id) : name(std::move(name)), userId(id) {}

    std::string id() const override { return std::to_string(userId); }
    const char* typeName() const override { return "A"; }

    std::unique_ptr<FdFile::RecordBase> clone() const override {
        return std::make_unique<A>(*this);
    }

    void
    toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const override {
        out.clear();
        out.push_back({"name", {true, name}});
        out.push_back({"id", {false, std::to_string(userId)}});
    }

    bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                std::error_code& ec) override {
        ec.clear();
        auto n = kv.find("name");
        auto i = kv.find("id");
        if (n == kv.end() || i == kv.end())
            return false;

        name = n->second.second;
        long tmp = 0;
        if (!FdFile::util::parseLongStrict(i->second.second, tmp, ec))
            return false;
        userId = tmp;
        return true;
    }
};
