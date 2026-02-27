/**
 * @file examples/records/B.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 API 사용 순서와 레코드 모델링 방식을 빠르게 이해할 수 있도록 제공되는 실행 예제 코드입니다.
 * - 예제는 학습 목적의 최소 흐름을 보여주므로 실제 서비스 코드에서는 에러 처리, 검증, 동시성 제어를 더 강화해야 합니다.
 * - 필드 길이/타입/ID 정책을 변경하면 파일 포맷 결과가 달라질 수 있으므로 출력 데이터와 호환성을 함께 확인해야 합니다.
 * - 예제에서 사용하는 값은 설명용 샘플이며, 운영 환경에서는 입력 정합성과 보안 제약을 별도로 검토해야 합니다.
 * - 신규 예제를 추가할 때는 '정의 -> 저장 -> 조회 -> 검증'의 전체 lifecycle이 끊기지 않도록 구성하는 것이 좋습니다.
 */
#pragma once
/// @file B.hpp
/// @brief Variable Length Record B

#include "record/VariableRecordBase.hpp"
#include "util/textFormatUtil.hpp"

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
