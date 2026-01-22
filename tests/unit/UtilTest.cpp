/**
 * @file UtilTest.cpp
 * @brief Unit tests for utility functions (textFormatUtil)
 */

#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <fdfile/util/textFormatUtil.hpp>

using namespace FdFile::util;

// =============================================================================
// parseLongStrict Tests
// =============================================================================

TEST(ParseLongStrictTest, ValidPositiveNumber) {
    long result = 0;
    std::error_code ec;

    EXPECT_TRUE(parseLongStrict("12345", result, ec));
    EXPECT_EQ(result, 12345);
    EXPECT_FALSE(ec);
}

TEST(ParseLongStrictTest, ValidNegativeNumber) {
    long result = 0;
    std::error_code ec;

    EXPECT_TRUE(parseLongStrict("-12345", result, ec));
    EXPECT_EQ(result, -12345);
    EXPECT_FALSE(ec);
}

TEST(ParseLongStrictTest, Zero) {
    long result = 0;
    std::error_code ec;

    EXPECT_TRUE(parseLongStrict("0", result, ec));
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(ec);
}

TEST(ParseLongStrictTest, InvalidString) {
    long result = 0;
    std::error_code ec;

    EXPECT_FALSE(parseLongStrict("abc", result, ec));
    EXPECT_TRUE(ec);
}

TEST(ParseLongStrictTest, MixedContent) {
    long result = 0;
    std::error_code ec;

    EXPECT_FALSE(parseLongStrict("123abc", result, ec));
    EXPECT_TRUE(ec);
}

// =============================================================================
// parseLine Tests
// =============================================================================

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

TEST(ParseLineTest, EmptyObject) {
    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    std::error_code ec;

    std::string line = "TypeB { }";

    ASSERT_TRUE(parseLine(line, type, kv, ec));
    EXPECT_EQ(type, "TypeB");
    EXPECT_EQ(kv.size(), 0);
}

TEST(ParseLineTest, EscapedQuotes) {
    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    std::error_code ec;

    std::string line = R"(Type { "text": "hello \"world\"" })";

    ASSERT_TRUE(parseLine(line, type, kv, ec));
    EXPECT_EQ(kv["text"].second, R"(hello "world")");
}

TEST(ParseLineTest, NegativeNumber) {
    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    std::error_code ec;

    std::string line = R"(Type { "value": -999 })";

    ASSERT_TRUE(parseLine(line, type, kv, ec));
    EXPECT_EQ(kv["value"].second, "-999");
}

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

TEST(FormatLineTest, EmptyFields) {
    std::vector<std::pair<std::string, std::pair<bool, std::string>>> fields;

    std::string result = formatLine("Empty", fields);

    EXPECT_EQ(result, "Empty {  }\n");
}

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

TEST(EscapeStringTest, NoEscapeNeeded) {
    EXPECT_EQ(escapeString("hello"), "hello");
}

TEST(EscapeStringTest, EscapeQuotes) {
    EXPECT_EQ(escapeString("say \"hello\""), "say \\\"hello\\\"");
}

TEST(EscapeStringTest, EscapeBackslash) {
    EXPECT_EQ(escapeString("path\\to\\file"), "path\\\\to\\\\file");
}

TEST(EscapeStringTest, EscapeNewlineAndTab) {
    EXPECT_EQ(escapeString("line1\nline2\tindent"), "line1\\nline2\\tindent");
}
