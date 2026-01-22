/**
 * @file FieldMetaTest.cpp
 * @brief Unit tests for FieldMeta serialization/deserialization
 */

#include <cstring>
#include <gtest/gtest.h>

#include <fdfile/record/FieldMeta.hpp>

using namespace FdFile;

// =============================================================================
// String Field Tests
// =============================================================================

TEST(FieldMetaStringTest, GetBasicString) {
    char name[10] = "hello";
    auto field = makeField<10>("name", name);

    char buf[11] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "hello");
}

TEST(FieldMetaStringTest, SetBasicString) {
    char name[10] = {0};
    auto field = makeField<10>("name", name);

    field.set("world");

    EXPECT_STREQ(name, "world");
}

TEST(FieldMetaStringTest, SetLongStringTruncated) {
    char name[5] = {0};
    auto field = makeField<5>("name", name);

    // Longer string should be truncated
    field.set("hello world");

    // Only first 5 chars should be copied
    EXPECT_EQ(std::string(name, 5), "hello");
}

// =============================================================================
// Numeric Field Tests
// =============================================================================

TEST(FieldMetaNumericTest, GetPositiveNumber) {
    int64_t age = 25;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "+0000000000000000025");
}

TEST(FieldMetaNumericTest, GetNegativeNumber) {
    int64_t age = -12345;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "-0000000000000012345");
}

TEST(FieldMetaNumericTest, GetZero) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "+0000000000000000000");
}

TEST(FieldMetaNumericTest, GetMaxInt64) {
    int64_t age = INT64_MAX;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "+9223372036854775807");
}

TEST(FieldMetaNumericTest, GetMinInt64) {
    int64_t age = INT64_MIN;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "-9223372036854775808");
}

TEST(FieldMetaNumericTest, SetPositiveNumber) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    field.set("+0000000000000000042");

    EXPECT_EQ(age, 42);
}

TEST(FieldMetaNumericTest, SetNegativeNumber) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    field.set("-0000000000000000099");

    EXPECT_EQ(age, -99);
}

TEST(FieldMetaNumericTest, SetMinInt64) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    field.set("-9223372036854775808");

    EXPECT_EQ(age, INT64_MIN);
}

TEST(FieldMetaNumericTest, SetMaxInt64) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    field.set("+9223372036854775807");

    EXPECT_EQ(age, INT64_MAX);
}

TEST(FieldMetaNumericTest, InvalidSignThrows) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    // Invalid sign character
    EXPECT_THROW(field.set("X0000000000000000025"), std::runtime_error);
    EXPECT_THROW(field.set("00000000000000000025"), std::runtime_error);
    EXPECT_THROW(field.set(" 0000000000000000025"), std::runtime_error);
}

// =============================================================================
// Round-trip Tests
// =============================================================================

TEST(FieldMetaRoundTripTest, PositiveNumberRoundTrip) {
    int64_t original = 123456789;
    int64_t restored = 0;

    auto fieldOut = makeNumField("num", original);
    auto fieldIn = makeNumField("num", restored);

    char buf[21] = {0};
    fieldOut.get(buf);
    fieldIn.set(buf);

    EXPECT_EQ(original, restored);
}

TEST(FieldMetaRoundTripTest, NegativeNumberRoundTrip) {
    int64_t original = -987654321;
    int64_t restored = 0;

    auto fieldOut = makeNumField("num", original);
    auto fieldIn = makeNumField("num", restored);

    char buf[21] = {0};
    fieldOut.get(buf);
    fieldIn.set(buf);

    EXPECT_EQ(original, restored);
}

TEST(FieldMetaRoundTripTest, StringRoundTrip) {
    char original[20] = "test_string";
    char restored[20] = {0};

    auto fieldOut = makeField<20>("str", original);
    auto fieldIn = makeField<20>("str", restored);

    char buf[21] = {0};
    fieldOut.get(buf);
    fieldIn.set(buf);

    EXPECT_STREQ(original, restored);
}
