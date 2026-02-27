/**
 * @file tests/unit/FieldMetaTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file FieldMetaTest.cpp
 * @brief Unit tests for FieldMeta serialization/deserialization
 */

#include <cstring>
#include <gtest/gtest.h>

#include "record/FieldMeta.hpp"

using namespace FdFile;

// =============================================================================
// String Field Tests
// =============================================================================

// 시나리오 상세 설명: FieldMetaStringTest 그룹의 GetBasicString 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaStringTest, GetBasicString) {
    char name[10] = "hello";
    auto field = makeField<10>("name", name);

    char buf[11] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "hello");
}

// 시나리오 상세 설명: FieldMetaStringTest 그룹의 SetBasicString 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaStringTest, SetBasicString) {
    char name[10] = {0};
    auto field = makeField<10>("name", name);

    field.set("world");

    EXPECT_STREQ(name, "world");
}

// 시나리오 상세 설명: FieldMetaStringTest 그룹의 SetLongStringTruncated 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 GetPositiveNumber 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, GetPositiveNumber) {
    int64_t age = 25;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "+0000000000000000025");
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 GetNegativeNumber 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, GetNegativeNumber) {
    int64_t age = -12345;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "-0000000000000012345");
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 GetZero 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, GetZero) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "+0000000000000000000");
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 GetMaxInt64 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, GetMaxInt64) {
    int64_t age = INT64_MAX;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "+9223372036854775807");
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 GetMinInt64 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, GetMinInt64) {
    int64_t age = INT64_MIN;
    auto field = makeNumField("age", age);

    char buf[21] = {0};
    field.get(buf);

    EXPECT_STREQ(buf, "-9223372036854775808");
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 SetPositiveNumber 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, SetPositiveNumber) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    field.set("+0000000000000000042");

    EXPECT_EQ(age, 42);
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 SetNegativeNumber 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, SetNegativeNumber) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    field.set("-0000000000000000099");

    EXPECT_EQ(age, -99);
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 SetMinInt64 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, SetMinInt64) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    field.set("-9223372036854775808");

    EXPECT_EQ(age, INT64_MIN);
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 SetMaxInt64 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST(FieldMetaNumericTest, SetMaxInt64) {
    int64_t age = 0;
    auto field = makeNumField("age", age);

    field.set("+9223372036854775807");

    EXPECT_EQ(age, INT64_MAX);
}

// 시나리오 상세 설명: FieldMetaNumericTest 그룹의 InvalidSignThrows 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: FieldMetaRoundTripTest 그룹의 PositiveNumberRoundTrip 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: FieldMetaRoundTripTest 그룹의 NegativeNumberRoundTrip 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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

// 시나리오 상세 설명: FieldMetaRoundTripTest 그룹의 StringRoundTrip 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
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
