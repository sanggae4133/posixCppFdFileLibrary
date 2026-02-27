/**
 * @file tests/unit/FileLockGuardTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file FileLockGuardTest.cpp
 * @brief Unit tests for FileLockGuard RAII wrapper
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <fdfile/util/FileLockGuard.hpp>

using namespace FdFile::detail;

class FileLockGuardTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_filelock.tmp";
        ::remove(testFile_.c_str());

        fd_ = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
        ASSERT_GE(fd_, 0);
    }

    void TearDown() override {
        if (fd_ >= 0)
            ::close(fd_);
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    int fd_ = -1;
};

// 시나리오 상세 설명: FileLockGuardTest 그룹의 DefaultConstructor 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, DefaultConstructor) {
    FileLockGuard lock;
    EXPECT_FALSE(lock.locked());
}

// 시나리오 상세 설명: FileLockGuardTest 그룹의 SharedLock 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, SharedLock) {
    std::error_code ec;
    FileLockGuard lock(fd_, FileLockGuard::Mode::Shared, ec);

    EXPECT_FALSE(ec);
    EXPECT_TRUE(lock.locked());
}

// 시나리오 상세 설명: FileLockGuardTest 그룹의 ExclusiveLock 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, ExclusiveLock) {
    std::error_code ec;
    FileLockGuard lock(fd_, FileLockGuard::Mode::Exclusive, ec);

    EXPECT_FALSE(ec);
    EXPECT_TRUE(lock.locked());
}

// 시나리오 상세 설명: FileLockGuardTest 그룹의 InvalidFdLock 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, InvalidFdLock) {
    std::error_code ec;
    FileLockGuard lock(-1, FileLockGuard::Mode::Shared, ec);

    EXPECT_TRUE(ec);
    EXPECT_FALSE(lock.locked());
}

// 시나리오 상세 설명: FileLockGuardTest 그룹의 UnlockOnDestruction 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, UnlockOnDestruction) {
    {
        std::error_code ec;
        FileLockGuard lock(fd_, FileLockGuard::Mode::Exclusive, ec);
        EXPECT_TRUE(lock.locked());
    } // lock goes out of scope

    // Should be able to lock again
    std::error_code ec;
    FileLockGuard lock2(fd_, FileLockGuard::Mode::Exclusive, ec);
    EXPECT_FALSE(ec);
    EXPECT_TRUE(lock2.locked());
}

// 시나리오 상세 설명: FileLockGuardTest 그룹의 MoveConstructor 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, MoveConstructor) {
    std::error_code ec;
    FileLockGuard lock1(fd_, FileLockGuard::Mode::Exclusive, ec);
    ASSERT_TRUE(lock1.locked());

    FileLockGuard lock2(std::move(lock1));

    EXPECT_FALSE(lock1.locked());
    EXPECT_TRUE(lock2.locked());
}

// 시나리오 상세 설명: FileLockGuardTest 그룹의 MoveAssignment 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, MoveAssignment) {
    std::error_code ec;
    FileLockGuard lock1(fd_, FileLockGuard::Mode::Exclusive, ec);
    ASSERT_TRUE(lock1.locked());

    FileLockGuard lock2;
    lock2 = std::move(lock1);

    EXPECT_FALSE(lock1.locked());
    EXPECT_TRUE(lock2.locked());
}

// 시나리오 상세 설명: FileLockGuardTest 그룹의 ManualLock 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, ManualLock) {
    FileLockGuard lock;
    EXPECT_FALSE(lock.locked());

    std::error_code ec;
    bool success = lock.lock(fd_, FileLockGuard::Mode::Shared, ec);

    EXPECT_TRUE(success);
    EXPECT_FALSE(ec);
    EXPECT_TRUE(lock.locked());
}

// 시나리오 상세 설명: FileLockGuardTest 그룹의 UnlockIgnore 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FileLockGuardTest, UnlockIgnore) {
    std::error_code ec;
    FileLockGuard lock(fd_, FileLockGuard::Mode::Exclusive, ec);
    ASSERT_TRUE(lock.locked());

    lock.unlockIgnore();
    EXPECT_FALSE(lock.locked());
}
