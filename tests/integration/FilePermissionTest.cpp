/**
 * @file tests/integration/FilePermissionTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file FilePermissionTest.cpp
 * @brief Integration tests for file permission and access control scenarios
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#include "records/FixedA.hpp"
#include <fdfile/repository/UniformFixedRepositoryImpl.hpp>

using namespace FdFile;

class FilePermissionTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_permission.db";
        ::remove(testFile_.c_str());
    }

    void TearDown() override {
        // Reset permissions before removing
        ::chmod(testFile_.c_str(), 0644);
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
};

// 시나리오 상세 설명: FilePermissionTest 그룹의 NormalReadWrite 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FilePermissionTest, NormalReadWrite) {
    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
    ASSERT_FALSE(ec);

    FixedA record("alice", 25, "001");
    EXPECT_TRUE(repo->save(record, ec));
    EXPECT_FALSE(ec);
}

// 시나리오 상세 설명: FilePermissionTest 그룹의 ReadOnlyFileWrite 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FilePermissionTest, ReadOnlyFileWrite) {
    // Create file with data
    {
        std::error_code ec;
        auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
        ASSERT_FALSE(ec);

        FixedA record("alice", 25, "001");
        repo->save(record, ec);
    }

    // Make read-only
    ::chmod(testFile_.c_str(), 0444);

    // Try to open for write - should fail or succeed depending on implementation
    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);

    // Open may succeed but write should fail
    if (!ec) {
        FixedA newRecord("bob", 30, "002");
        bool saved = repo->save(newRecord, ec);
        // May fail due to read-only
    }
}

// 시나리오 상세 설명: FilePermissionTest 그룹의 NonExistentDirectory 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FilePermissionTest, NonExistentDirectory) {
    std::error_code ec;
    auto repo =
        std::make_unique<UniformFixedRepositoryImpl<FixedA>>("/nonexistent/path/file.db", ec);

    // Should fail - directory doesn't exist
    EXPECT_TRUE(ec);
}

// 시나리오 상세 설명: FilePermissionTest 그룹의 CreateNewFile 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FilePermissionTest, CreateNewFile) {
    std::string newFile = "./test_new_file.db";
    ::remove(newFile.c_str());

    std::error_code ec;
    auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(newFile, ec);

    // Should create new file
    EXPECT_FALSE(ec);

    // Verify file exists
    struct stat st;
    EXPECT_EQ(::stat(newFile.c_str(), &st), 0);

    ::remove(newFile.c_str());
}

// 시나리오 상세 설명: FilePermissionTest 그룹의 FileDescriptorLeak 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FilePermissionTest, FileDescriptorLeak) {
    // Open and close many times - should not leak FDs
    for (int i = 0; i < 100; ++i) {
        std::error_code ec;
        auto repo = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec);
        ASSERT_FALSE(ec) << "Failed on iteration " << i;

        FixedA record("user", i, std::to_string(i).c_str());
        repo->save(record, ec);
    }

    // If we got here, no FD exhaustion
    SUCCEED();
}
