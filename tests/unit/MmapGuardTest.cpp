/**
 * @file tests/unit/MmapGuardTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file MmapGuardTest.cpp
 * @brief Unit tests for MmapGuard RAII wrapper
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util/MmapGuard.hpp"

using namespace FdFile::detail;

class MmapGuardTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_mmap.tmp";
        ::remove(testFile_.c_str());

        // Create file with some content
        int fd = ::open(testFile_.c_str(), O_CREAT | O_RDWR, 0644);
        ASSERT_GE(fd, 0);
        const char* data = "Hello, MmapGuard Test!";
        ::write(fd, data, strlen(data));
        ::close(fd);
    }

    void TearDown() override { ::remove(testFile_.c_str()); }

    std::string testFile_;
};

// 시나리오 상세 설명: MmapGuardTest 그룹의 DefaultConstructor 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(MmapGuardTest, DefaultConstructor) {
    MmapGuard mmap;
    EXPECT_EQ(mmap.get(), nullptr);
    EXPECT_EQ(mmap.size(), 0);
    EXPECT_FALSE(mmap.valid());
    EXPECT_FALSE(static_cast<bool>(mmap));
}

// 시나리오 상세 설명: MmapGuardTest 그룹의 ConstructWithValidMapping 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(MmapGuardTest, ConstructWithValidMapping) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT_NE(ptr, MAP_FAILED);

    MmapGuard mmap(ptr, st.st_size);
    EXPECT_EQ(mmap.get(), ptr);
    EXPECT_EQ(mmap.size(), static_cast<size_t>(st.st_size));
    EXPECT_TRUE(mmap.valid());

    ::close(fd);
}

// 시나리오 상세 설명: MmapGuardTest 그룹의 ConstructWithMapFailed 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(MmapGuardTest, ConstructWithMapFailed) {
    MmapGuard mmap(MAP_FAILED, 100);
    EXPECT_EQ(mmap.get(), nullptr);
    EXPECT_EQ(mmap.size(), 0);
    EXPECT_FALSE(mmap.valid());
}

// 시나리오 상세 설명: MmapGuardTest 그룹의 MoveConstructor 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(MmapGuardTest, MoveConstructor) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    MmapGuard mmap1(ptr, st.st_size);
    MmapGuard mmap2(std::move(mmap1));

    EXPECT_EQ(mmap1.get(), nullptr);
    EXPECT_EQ(mmap2.get(), ptr);

    ::close(fd);
}

// 시나리오 상세 설명: MmapGuardTest 그룹의 Reset 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(MmapGuardTest, Reset) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    MmapGuard mmap(ptr, st.st_size);
    EXPECT_TRUE(mmap.valid());

    mmap.reset();
    EXPECT_FALSE(mmap.valid());
    EXPECT_EQ(mmap.get(), nullptr);

    ::close(fd);
}

// 시나리오 상세 설명: MmapGuardTest 그룹의 DataAccess 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(MmapGuardTest, DataAccess) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    MmapGuard mmap(ptr, st.st_size);
    EXPECT_STREQ(mmap.data(), "Hello, MmapGuard Test!");

    ::close(fd);
}

// 시나리오 상세 설명: MmapGuardTest 그룹의 Sync 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(MmapGuardTest, Sync) {
    int fd = ::open(testFile_.c_str(), O_RDWR);
    ASSERT_GE(fd, 0);

    struct stat st;
    ::fstat(fd, &st);

    void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    MmapGuard mmap(ptr, st.st_size);

    // Modify content
    char* data = mmap.data();
    data[0] = 'X';

    // Sync should succeed
    EXPECT_TRUE(mmap.sync());
    EXPECT_TRUE(mmap.sync(true)); // async sync

    ::close(fd);
}

// 시나리오 상세 설명: MmapGuardTest 그룹의 SyncInvalidMapping 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(MmapGuardTest, SyncInvalidMapping) {
    MmapGuard mmap;
    EXPECT_FALSE(mmap.sync());
}
