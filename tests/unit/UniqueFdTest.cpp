/**
 * @file tests/unit/UniqueFdTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file UniqueFdTest.cpp
 * @brief Unit tests for UniqueFd RAII wrapper
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <fdfile/util/UniqueFd.hpp>

using namespace FdFile::detail;

class UniqueFdTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_uniquefd.tmp";
        ::remove(testFile_.c_str());
    }

    void TearDown() override { ::remove(testFile_.c_str()); }

    std::string testFile_;
};

// 시나리오 상세 설명: UniqueFdTest 그룹의 DefaultConstructor 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(UniqueFdTest, DefaultConstructor) {
    UniqueFd fd;
    EXPECT_EQ(fd.get(), -1);
    EXPECT_FALSE(fd.valid());
    EXPECT_FALSE(static_cast<bool>(fd));
}

// 시나리오 상세 설명: UniqueFdTest 그룹의 ConstructWithValidFd 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(UniqueFdTest, ConstructWithValidFd) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    UniqueFd fd(rawFd);
    EXPECT_EQ(fd.get(), rawFd);
    EXPECT_TRUE(fd.valid());
    EXPECT_TRUE(static_cast<bool>(fd));
}

// 시나리오 상세 설명: UniqueFdTest 그룹의 DestructorClosesFd 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(UniqueFdTest, DestructorClosesFd) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    {
        UniqueFd fd(rawFd);
        EXPECT_TRUE(fd.valid());
    } // fd goes out of scope, should close

    // Verify fd is closed - write should fail
    EXPECT_EQ(::write(rawFd, "x", 1), -1);
}

// 시나리오 상세 설명: UniqueFdTest 그룹의 MoveConstructor 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(UniqueFdTest, MoveConstructor) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    UniqueFd fd1(rawFd);
    UniqueFd fd2(std::move(fd1));

    EXPECT_EQ(fd1.get(), -1);
    EXPECT_FALSE(fd1.valid());
    EXPECT_EQ(fd2.get(), rawFd);
    EXPECT_TRUE(fd2.valid());
}

// 시나리오 상세 설명: UniqueFdTest 그룹의 MoveAssignment 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(UniqueFdTest, MoveAssignment) {
    int rawFd1 = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd1, 0);

    UniqueFd fd1(rawFd1);
    UniqueFd fd2;

    fd2 = std::move(fd1);

    EXPECT_EQ(fd1.get(), -1);
    EXPECT_EQ(fd2.get(), rawFd1);
}

// 시나리오 상세 설명: UniqueFdTest 그룹의 Release 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(UniqueFdTest, Release) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    UniqueFd fd(rawFd);
    int released = fd.release();

    EXPECT_EQ(released, rawFd);
    EXPECT_EQ(fd.get(), -1);
    EXPECT_FALSE(fd.valid());

    // Must manually close since we released
    ::close(released);
}

// 시나리오 상세 설명: UniqueFdTest 그룹의 Reset 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(UniqueFdTest, Reset) {
    int rawFd1 = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd1, 0);

    UniqueFd fd(rawFd1);

    // Open another file
    std::string testFile2 = "./test_uniquefd2.tmp";
    int rawFd2 = ::open(testFile2.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd2, 0);

    fd.reset(rawFd2);

    EXPECT_EQ(fd.get(), rawFd2);
    // rawFd1 should be closed
    EXPECT_EQ(::write(rawFd1, "x", 1), -1);

    ::remove(testFile2.c_str());
}

// 시나리오 상세 설명: UniqueFdTest 그룹의 ResetToInvalid 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(UniqueFdTest, ResetToInvalid) {
    int rawFd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(rawFd, 0);

    UniqueFd fd(rawFd);
    fd.reset();

    EXPECT_EQ(fd.get(), -1);
    EXPECT_FALSE(fd.valid());
}
