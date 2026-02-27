/**
 * @file tests/unit/UtilTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file UtilTest.cpp
 * @brief Unit tests for utility functions (textFormatUtil)
 */

#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "util/textFormatUtil.hpp"

using namespace FdFile::util;

// =============================================================================
// parseLongStrict Tests
// =============================================================================

// 시나리오 상세 설명: ParseLongStrictTest 그룹의 ValidPositiveNumber 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLongStrictTest, ValidPositiveNumber) {
    long result = 0;
    std::error_code ec;

    EXPECT_TRUE(parseLongStrict("12345", result, ec));
    EXPECT_EQ(result, 12345);
    EXPECT_FALSE(ec);
}

// 시나리오 상세 설명: ParseLongStrictTest 그룹의 ValidNegativeNumber 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLongStrictTest, ValidNegativeNumber) {
    long result = 0;
    std::error_code ec;

    EXPECT_TRUE(parseLongStrict("-12345", result, ec));
    EXPECT_EQ(result, -12345);
    EXPECT_FALSE(ec);
}

// 시나리오 상세 설명: ParseLongStrictTest 그룹의 Zero 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLongStrictTest, Zero) {
    long result = 0;
    std::error_code ec;

    EXPECT_TRUE(parseLongStrict("0", result, ec));
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(ec);
}

// 시나리오 상세 설명: ParseLongStrictTest 그룹의 InvalidString 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLongStrictTest, InvalidString) {
    long result = 0;
    std::error_code ec;

    EXPECT_FALSE(parseLongStrict("abc", result, ec));
    EXPECT_TRUE(ec);
}

// 시나리오 상세 설명: ParseLongStrictTest 그룹의 MixedContent 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLongStrictTest, MixedContent) {
    long result = 0;
    std::error_code ec;

    EXPECT_FALSE(parseLongStrict("123abc", result, ec));
    EXPECT_TRUE(ec);
}

// =============================================================================
// parseLine Tests
// =============================================================================

// 시나리오 상세 설명: ParseLineTest 그룹의 ValidLineWithStringAndNumber 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLineTest, ValidLineWithStringAndNumber) {
    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    std::error_code ec;

    std::string line = R"(TypeA { "name": "alice", "id": 123 })";

    ASSERT_TRUE(parseLine(line, type, kv, ec));
    EXPECT_EQ(type, "TypeA");
    EXPECT_EQ(kv.size(), 2);

    EXPECT_TRUE(kv["name"].first); // isString
    EXPECT_EQ(kv["name"].second, "alice");

    EXPECT_FALSE(kv["id"].first); // not string
    EXPECT_EQ(kv["id"].second, "123");
}

// 시나리오 상세 설명: ParseLineTest 그룹의 EmptyObject 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLineTest, EmptyObject) {
    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    std::error_code ec;

    std::string line = "TypeB { }";

    ASSERT_TRUE(parseLine(line, type, kv, ec));
    EXPECT_EQ(type, "TypeB");
    EXPECT_EQ(kv.size(), 0);
}

// 시나리오 상세 설명: ParseLineTest 그룹의 EscapedQuotes 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLineTest, EscapedQuotes) {
    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    std::error_code ec;

    std::string line = R"(Type { "text": "hello \"world\"" })";

    ASSERT_TRUE(parseLine(line, type, kv, ec));
    EXPECT_EQ(kv["text"].second, R"(hello "world")");
}

// 시나리오 상세 설명: ParseLineTest 그룹의 NegativeNumber 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLineTest, NegativeNumber) {
    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    std::error_code ec;

    std::string line = R"(Type { "value": -999 })";

    ASSERT_TRUE(parseLine(line, type, kv, ec));
    EXPECT_EQ(kv["value"].second, "-999");
}

// 시나리오 상세 설명: ParseLineTest 그룹의 InvalidMissingBrace 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(ParseLineTest, InvalidMissingBrace) {
    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    std::error_code ec;

    std::string line = R"(TypeA "name": "alice" })";

    EXPECT_FALSE(parseLine(line, type, kv, ec));
}

// =============================================================================
// formatLine Tests
// =============================================================================

// 시나리오 상세 설명: FormatLineTest 그룹의 SimpleFields 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FormatLineTest, SimpleFields) {
    std::vector<std::pair<std::string, std::pair<bool, std::string>>> fields;
    fields.push_back({"name", {true, "alice"}});
    fields.push_back({"id", {false, "123"}});

    std::string result = formatLine("TypeA", fields);

    // Should contain proper structure
    EXPECT_NE(result.find("TypeA"), std::string::npos);
    EXPECT_NE(result.find("\"name\": \"alice\""), std::string::npos);
    EXPECT_NE(result.find("\"id\": 123"), std::string::npos);
    EXPECT_EQ(result.back(), '\n'); // Should end with newline
}

// 시나리오 상세 설명: FormatLineTest 그룹의 EmptyFields 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FormatLineTest, EmptyFields) {
    std::vector<std::pair<std::string, std::pair<bool, std::string>>> fields;

    std::string result = formatLine("Empty", fields);

    EXPECT_EQ(result, "Empty {  }\n");
}

// 시나리오 상세 설명: FormatLineTest 그룹의 SpecialCharacters 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FormatLineTest, SpecialCharacters) {
    std::vector<std::pair<std::string, std::pair<bool, std::string>>> fields;
    fields.push_back({"text", {true, "hello\nworld"}});

    std::string result = formatLine("Type", fields);

    // Newline should be escaped
    EXPECT_NE(result.find("\\n"), std::string::npos);
}

// =============================================================================
// escapeString Tests
// =============================================================================

// 시나리오 상세 설명: EscapeStringTest 그룹의 NoEscapeNeeded 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(EscapeStringTest, NoEscapeNeeded) {
    EXPECT_EQ(escapeString("hello"), "hello");
}

// 시나리오 상세 설명: EscapeStringTest 그룹의 EscapeQuotes 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(EscapeStringTest, EscapeQuotes) {
    EXPECT_EQ(escapeString("say \"hello\""), "say \\\"hello\\\"");
}

// 시나리오 상세 설명: EscapeStringTest 그룹의 EscapeBackslash 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(EscapeStringTest, EscapeBackslash) {
    EXPECT_EQ(escapeString("path\\to\\file"), "path\\\\to\\\\file");
}

// 시나리오 상세 설명: EscapeStringTest 그룹의 EscapeNewlineAndTab 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(EscapeStringTest, EscapeNewlineAndTab) {
    EXPECT_EQ(escapeString("line1\nline2\tindent"), "line1\\nline2\\tindent");
}
