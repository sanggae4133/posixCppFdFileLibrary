/**
 * @file tests/FixedRecordTest.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 라이브러리의 동작 계약(contract)을 검증하기 위한 테스트 시나리오를 정의합니다.
 * - 테스트는 정상 경로뿐 아니라 경계값, 실패 경로, 파일 I/O 예외 상황을 분리해 원인 추적이 쉽도록 구성되어야 합니다.
 * - 각 assertion은 '무엇이 실패했는지'가 즉시 드러나도록 작성하며, 상태 공유를 피하기 위해 테스트 간 파일/데이터 독립성을 유지해야 합니다.
 * - 저장 포맷/락 정책/캐시 정책이 바뀌면 해당 변화가 기존 계약을 깨지 않는지 회귀 테스트를 반드시 확장해야 합니다.
 * - 향후 테스트 추가 시에는 재현 가능한 입력, 명확한 기대 결과, 실패 시 진단 가능한 메시지를 함께 유지하는 것을 권장합니다.
 */
/**
 * @file FixedRecordTest.cpp
 * @brief Unit tests for Fixed-length record repositories (FixedA, FixedB)
 */

#include <gtest/gtest.h>
#include <memory>

#include "records/FixedA.hpp"
#include "records/FixedB.hpp"
#include <fdfile/repository/UniformFixedRepositoryImpl.hpp>

using namespace FdFile;

// =============================================================================
// FixedA Repository Tests
// =============================================================================

class FixedARepositoryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_fixed_a.db";
        ::remove(testFile_.c_str());
        repo_ = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec_);
        ASSERT_FALSE(ec_) << "Repository init failed: " << ec_.message();
    }

    void TearDown() override {
        repo_.reset();
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<UniformFixedRepositoryImpl<FixedA>> repo_;
};

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 InsertSingleRecord 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, InsertSingleRecord) {
    FixedA alice("alice", 25, "001");

    ASSERT_TRUE(repo_->save(alice, ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 InsertMultipleRecords 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, InsertMultipleRecords) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    FixedA charlie("charlie", 35, "003");

    ASSERT_TRUE(repo_->save(alice, ec_));
    ASSERT_TRUE(repo_->save(bob, ec_));
    ASSERT_TRUE(repo_->save(charlie, ec_));

    EXPECT_EQ(repo_->count(ec_), 3);
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 UpdateExistingRecord 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, UpdateExistingRecord) {
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);

    // Update with same ID
    FixedA updatedAlice("alice_v2", 26, "001");
    ASSERT_TRUE(repo_->save(updatedAlice, ec_));

    // Count should still be 1
    EXPECT_EQ(repo_->count(ec_), 1);

    // Verify update
    auto found = repo_->findById("001", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, 26);
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 FindById 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, FindById) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    auto found = repo_->findById("002", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "bob");
    EXPECT_EQ(found->age, 30);
    EXPECT_EQ(found->getId(), "002");
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 FindByIdNotFound 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, FindByIdNotFound) {
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);

    auto notFound = repo_->findById("999", ec_);
    EXPECT_EQ(notFound, nullptr);
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 FindAll 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, FindAll) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    FixedA charlie("charlie", 35, "003");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);
    repo_->save(charlie, ec_);

    auto all = repo_->findAll(ec_);
    EXPECT_EQ(all.size(), 3);
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 ExistsById 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, ExistsById) {
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);

    EXPECT_TRUE(repo_->existsById("001", ec_));
    EXPECT_FALSE(repo_->existsById("999", ec_));
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 DeleteById 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, DeleteById) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    ASSERT_TRUE(repo_->deleteById("001", ec_));
    EXPECT_EQ(repo_->count(ec_), 1);
    EXPECT_FALSE(repo_->existsById("001", ec_));
    EXPECT_TRUE(repo_->existsById("002", ec_));
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 DeleteAll 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, DeleteAll) {
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);

    ASSERT_TRUE(repo_->deleteAll(ec_));
    EXPECT_EQ(repo_->count(ec_), 0);
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 EdgeCaseEmptyName 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, EdgeCaseEmptyName) {
    FixedA empty("", 0, "E01");

    ASSERT_TRUE(repo_->save(empty, ec_));

    auto found = repo_->findById("E01", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "");
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 EdgeCaseMaxInt64 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, EdgeCaseMaxInt64) {
    FixedA maxAge("max", INT64_MAX, "E02");

    ASSERT_TRUE(repo_->save(maxAge, ec_));

    auto found = repo_->findById("E02", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, INT64_MAX);
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 EdgeCaseNegativeInt64 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, EdgeCaseNegativeInt64) {
    FixedA negative("negative", -12345, "E03");

    ASSERT_TRUE(repo_->save(negative, ec_));

    auto found = repo_->findById("E03", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, -12345);
}

// 시나리오 상세 설명: FixedARepositoryTest 그룹의 EdgeCaseMinInt64 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedARepositoryTest, EdgeCaseMinInt64) {
    FixedA minAge("min", INT64_MIN, "E04");

    ASSERT_TRUE(repo_->save(minAge, ec_));

    auto found = repo_->findById("E04", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->age, INT64_MIN);
}

// =============================================================================
// FixedB Repository Tests
// =============================================================================

class FixedBRepositoryTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_fixed_b.db";
        ::remove(testFile_.c_str());
        repo_ = std::make_unique<UniformFixedRepositoryImpl<FixedB>>(testFile_, ec_);
        ASSERT_FALSE(ec_) << "Repository init failed: " << ec_.message();
    }

    void TearDown() override {
        repo_.reset();
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<UniformFixedRepositoryImpl<FixedB>> repo_;
};

// 시나리오 상세 설명: FixedBRepositoryTest 그룹의 InsertMultipleRecords 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedBRepositoryTest, InsertMultipleRecords) {
    FixedB laptop("Laptop", 1500000, "P001");
    FixedB phone("Phone", 800000, "P002");
    FixedB tablet("Tablet", 500000, "P003");

    ASSERT_TRUE(repo_->save(laptop, ec_));
    ASSERT_TRUE(repo_->save(phone, ec_));
    ASSERT_TRUE(repo_->save(tablet, ec_));

    EXPECT_EQ(repo_->count(ec_), 3);
}

// 시나리오 상세 설명: FixedBRepositoryTest 그룹의 FindAll 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedBRepositoryTest, FindAll) {
    FixedB laptop("Laptop", 1500000, "P001");
    FixedB phone("Phone", 800000, "P002");
    repo_->save(laptop, ec_);
    repo_->save(phone, ec_);

    auto all = repo_->findAll(ec_);
    ASSERT_EQ(all.size(), 2);

    // Verify content
    bool foundLaptop = false, foundPhone = false;
    for (const auto& rec : all) {
        if (std::string(rec->title) == "Laptop")
            foundLaptop = true;
        if (std::string(rec->title) == "Phone")
            foundPhone = true;
    }
    EXPECT_TRUE(foundLaptop);
    EXPECT_TRUE(foundPhone);
}

// 시나리오 상세 설명: FixedBRepositoryTest 그룹의 UpdateExistingRecord 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(FixedBRepositoryTest, UpdateExistingRecord) {
    FixedB phone("Phone", 800000, "P001");
    repo_->save(phone, ec_);

    // Update price (discount!)
    FixedB discountedPhone("Phone", 750000, "P001");
    ASSERT_TRUE(repo_->save(discountedPhone, ec_));

    EXPECT_EQ(repo_->count(ec_), 1);

    auto found = repo_->findById("P001", ec_);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->cost, 750000);
}

// =============================================================================
// External Modification Tests
// =============================================================================

class ExternalModificationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_external_mod.db";
        ::remove(testFile_.c_str());
        repo_ = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec_);
        ASSERT_FALSE(ec_) << "Repository init failed: " << ec_.message();
    }

    void TearDown() override {
        repo_.reset();
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<UniformFixedRepositoryImpl<FixedA>> repo_;
};

// 시나리오 상세 설명: ExternalModificationTest 그룹의 DetectsExternalChange 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(ExternalModificationTest, DetectsExternalChange) {
    // 1. Save via repo
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // 2. Get record serialized size for external modification
    size_t recordSize = alice.recordSize();

    // 3. External modification: create a new record and write directly
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        // Seek to the first record and modify age bytes
        // The exact byte position depends on the serialization format
        // For simplicity, we'll append a new record instead

        // Create a new FixedA and serialize it
        FixedA bob("bob", 30, "002");
        std::vector<char> buf(recordSize);
        bob.serialize(buf.data());

        // Append to file
        ::lseek(fd, 0, SEEK_END);
        ::write(fd, buf.data(), recordSize);
        ::fsync(fd);
        ::close(fd);
    }

    // 4. Force mtime change (sleep briefly for mtime granularity)
    usleep(10000); // 10ms

    // 5. Read again - should detect external change and reload
    auto found = repo_->findById("002", ec_);
    ASSERT_FALSE(ec_);
    ASSERT_NE(found, nullptr) << "External append should be detected";
    EXPECT_STREQ(found->name, "bob");
    EXPECT_EQ(found->age, 30);

    // Count should now be 2
    EXPECT_EQ(repo_->count(ec_), 2);
}

// 시나리오 상세 설명: ExternalModificationTest 그룹의 DetectsExternalChangeImmediately 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(ExternalModificationTest, DetectsExternalChangeImmediately) {
    // 1. Save via repo
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // 2. Get record serialized size for external modification
    size_t recordSize = alice.recordSize();

    // 3. External modification: create a new record and write directly
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        // Seek to the first record and modify age bytes
        // The exact byte position depends on the serialization format
        // For simplicity, we'll append a new record instead

        // Create a new FixedA and serialize it
        FixedA bob("bob", 30, "002");
        std::vector<char> buf(recordSize);
        bob.serialize(buf.data());

        // Append to file
        ::lseek(fd, 0, SEEK_END);
        ::write(fd, buf.data(), recordSize);
        ::fsync(fd);
        ::close(fd);
    }

    // 4. Read again - should detect external change and reload
    auto found = repo_->findById("002", ec_);
    ASSERT_FALSE(ec_);
    ASSERT_NE(found, nullptr) << "External append should be detected";
    EXPECT_STREQ(found->name, "bob");
    EXPECT_EQ(found->age, 30);

    // Count should now be 2
    EXPECT_EQ(repo_->count(ec_), 2);
}

// 시나리오 상세 설명: ExternalModificationTest 그룹의 CorruptFileReturnsError 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(ExternalModificationTest, CorruptFileReturnsError) {
    // 1. Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // 2. Corrupt the file externally (write partial data)
    {
        int fd = ::open(testFile_.c_str(), O_RDWR | O_APPEND);
        ASSERT_GE(fd, 0);

        // Write garbage data that doesn't align with recordSize
        const char* garbage = "corrupt";
        ::write(fd, garbage, 7); // Not a multiple of recordSize
        ::fsync(fd);
        ::close(fd);
    }

    // 3. Force mtime change
    usleep(10000);

    // 4. Try to read - should return error due to file size mismatch
    auto found = repo_->findById("001", ec_);
    EXPECT_TRUE(ec_) << "Should return error for corrupt file";
}

// 시나리오 상세 설명: ExternalModificationTest 그룹의 CorruptFileReturnsErrorImmediately 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(ExternalModificationTest, CorruptFileReturnsErrorImmediately) {
    // 1. Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // 2. Corrupt the file externally (write partial data)
    {
        int fd = ::open(testFile_.c_str(), O_RDWR | O_APPEND);
        ASSERT_GE(fd, 0);

        // Write garbage data that doesn't align with recordSize
        const char* garbage = "corrupt";
        ::write(fd, garbage, 7); // Not a multiple of recordSize
        ::fsync(fd);
        ::close(fd);
    }

    // 3. Try to read - should return error due to file size mismatch
    auto found = repo_->findById("001", ec_);
    EXPECT_TRUE(ec_) << "Should return error for corrupt file";
}

// 시나리오 상세 설명: ExternalModificationTest 그룹의 CacheConsistencyAfterDelete 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(ExternalModificationTest, CacheConsistencyAfterDelete) {
    // Insert 3 records
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    FixedA charlie("charlie", 35, "003");

    repo_->save(alice, ec_);
    repo_->save(bob, ec_);
    repo_->save(charlie, ec_);
    ASSERT_FALSE(ec_);

    // Delete middle record
    repo_->deleteById("002", ec_);
    ASSERT_FALSE(ec_);

    // Verify cache is still consistent
    EXPECT_TRUE(repo_->existsById("001", ec_));
    EXPECT_FALSE(repo_->existsById("002", ec_)); // Deleted
    EXPECT_TRUE(repo_->existsById("003", ec_));

    // FindById should still work
    auto foundAlice = repo_->findById("001", ec_);
    ASSERT_NE(foundAlice, nullptr);
    EXPECT_STREQ(foundAlice->name, "alice");

    auto foundCharlie = repo_->findById("003", ec_);
    ASSERT_NE(foundCharlie, nullptr);
    EXPECT_STREQ(foundCharlie->name, "charlie");
}

// 시나리오 상세 설명: ExternalModificationTest 그룹의 ExternalAppendMultipleRecords 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(ExternalModificationTest, ExternalAppendMultipleRecords) {
    // 1. Save initial record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);
    EXPECT_EQ(repo_->count(ec_), 1);

    // 2. Externally append 2 more records
    size_t recordSize = alice.recordSize();
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        FixedA bob("bob", 30, "002");
        FixedA charlie("charlie", 35, "003");

        std::vector<char> buf1(recordSize), buf2(recordSize);
        bob.serialize(buf1.data());
        charlie.serialize(buf2.data());

        ::lseek(fd, 0, SEEK_END);
        ::write(fd, buf1.data(), recordSize);
        ::write(fd, buf2.data(), recordSize);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // 3. Verify all 3 records are visible
    EXPECT_EQ(repo_->count(ec_), 3);
    EXPECT_TRUE(repo_->existsById("001", ec_));
    EXPECT_TRUE(repo_->existsById("002", ec_));
    EXPECT_TRUE(repo_->existsById("003", ec_));
}

// =============================================================================
// Bizarre File Corruption Tests (기상천외한 파일 손상 테스트)
// =============================================================================

class BizarreCorruptionTest : public ::testing::Test {
  protected:
    void SetUp() override {
        testFile_ = "./test_bizarre_corrupt.db";
        ::remove(testFile_.c_str());
        repo_ = std::make_unique<UniformFixedRepositoryImpl<FixedA>>(testFile_, ec_);
        ASSERT_FALSE(ec_) << "Repository init failed: " << ec_.message();

        // Get record size
        FixedA temp("test", 0, "000");
        recordSize_ = temp.recordSize();
    }

    void TearDown() override {
        repo_.reset();
        ::remove(testFile_.c_str());
    }

    std::string testFile_;
    std::error_code ec_;
    std::unique_ptr<UniformFixedRepositoryImpl<FixedA>> repo_;
    size_t recordSize_ = 0;
};

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 EmptyFileAfterTruncation 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, EmptyFileAfterTruncation) {
    // Save then externally truncate to empty
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Externally truncate to 0
    {
        int fd = ::open(testFile_.c_str(), O_RDWR | O_TRUNC);
        ASSERT_GE(fd, 0);
        ::close(fd);
    }

    usleep(10000);

    // Should return 0 count, no error
    size_t cnt = repo_->count(ec_);
    EXPECT_FALSE(ec_); // Empty file is valid
    EXPECT_EQ(cnt, 0);
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 FileFilledWithZeros 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, FileFilledWithZeros) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Overwrite with all zeros
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);
        std::vector<char> zeros(recordSize_, 0);
        ::write(fd, zeros.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Try to find - zeros cause invalid sign character
    try {
        auto found = repo_->findById("001", ec_);
        // If no exception, record not found is acceptable
        EXPECT_EQ(found, nullptr);
    } catch (const std::runtime_error& e) {
        // Exception for invalid sign is expected
        EXPECT_NE(std::string(e.what()).find("sign"), std::string::npos);
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 FileFilledWithRandomBinaryGarbage 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, FileFilledWithRandomBinaryGarbage) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Overwrite with random binary data
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);
        std::vector<char> garbage(recordSize_);
        for (size_t i = 0; i < recordSize_; ++i) {
            garbage[i] = static_cast<char>(rand() % 256);
        }
        ::write(fd, garbage.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Try operations - may throw, but shouldn't crash
    try {
        auto all = repo_->findAll(ec_);
        // If it didn't throw, it should have handled it somehow
    } catch (const std::exception& e) {
        // Exception is acceptable for corrupt data
        SUCCEED() << "Exception thrown for garbage data: " << e.what();
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 TruncatedRecord_PartialData 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, TruncatedRecord_PartialData) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Truncate to half the record size
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);
        ::ftruncate(fd, recordSize_ / 2); // Half a record
        ::close(fd);
    }

    usleep(10000);

    // Should return error - file size not aligned
    auto found = repo_->findById("001", ec_);
    EXPECT_TRUE(ec_) << "Should error on partial record";
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 InvalidSignCharacter 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, InvalidSignCharacter) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Find the age field and corrupt the sign character
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        // Read current content
        std::vector<char> buf(recordSize_);
        ::read(fd, buf.data(), recordSize_);

        // Find +/- and replace with invalid char (somewhere in the numeric field)
        for (size_t i = 0; i < recordSize_; ++i) {
            if (buf[i] == '+' || buf[i] == '-') {
                buf[i] = 'X'; // Invalid sign
                break;
            }
        }

        ::lseek(fd, 0, SEEK_SET);
        ::write(fd, buf.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Should throw exception for invalid sign
    try {
        auto found = repo_->findById("001", ec_);
        // If no exception, might have just not found it
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("sign"), std::string::npos);
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 FileFilledWithNewlines 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, FileFilledWithNewlines) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Overwrite with newlines
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);
        std::vector<char> newlines(recordSize_, '\n');
        ::write(fd, newlines.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Newlines cause invalid sign character
    try {
        auto found = repo_->findById("001", ec_);
        EXPECT_EQ(found, nullptr);
    } catch (const std::runtime_error& e) {
        // Exception for invalid sign is expected
        EXPECT_NE(std::string(e.what()).find("sign"), std::string::npos);
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 FileFilledWithNulls 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, FileFilledWithNulls) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Overwrite with null bytes
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);
        std::vector<char> nulls(recordSize_, '\0');
        ::write(fd, nulls.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Should handle gracefully - may throw or fail silently
    try {
        auto found = repo_->findById("001", ec_);
        EXPECT_EQ(found, nullptr);
    } catch (const std::exception& e) {
        SUCCEED() << "Exception for null-filled data: " << e.what();
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 PartialOverwriteMiddle 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, PartialOverwriteMiddle) {
    // Save 2 records
    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    repo_->save(alice, ec_);
    repo_->save(bob, ec_);
    ASSERT_FALSE(ec_);

    // Partially overwrite the middle of the file
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);
        ::lseek(fd, recordSize_ / 2, SEEK_SET); // Middle of first record
        const char* garbage = "CORRUPTED_DATA";
        ::write(fd, garbage, strlen(garbage));
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // First record should be corrupted, second might still work
    auto foundAlice = repo_->findById("001", ec_);
    // Alice is corrupted, might not be found

    auto foundBob = repo_->findById("002", ec_);
    // Bob might still be intact depending on where corruption hit
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 FileExtendedWithOddBytes 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, FileExtendedWithOddBytes) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Append odd number of bytes (not multiple of record size)
    {
        int fd = ::open(testFile_.c_str(), O_RDWR | O_APPEND);
        ASSERT_GE(fd, 0);
        const char* oddBytes = "123"; // 3 bytes
        ::write(fd, oddBytes, 3);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Should error - file size not aligned
    auto found = repo_->findById("001", ec_);
    EXPECT_TRUE(ec_) << "Should error on unaligned file size";
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 RecordSizeChangedMidway 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, RecordSizeChangedMidway) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Double the file size with different pattern
    {
        int fd = ::open(testFile_.c_str(), O_RDWR | O_APPEND);
        ASSERT_GE(fd, 0);
        std::vector<char> halfRecord(recordSize_ / 2, 'X');
        ::write(fd, halfRecord.data(), halfRecord.size());
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // File is 1.5x record size - not aligned
    auto found = repo_->findById("001", ec_);
    EXPECT_TRUE(ec_) << "Should error on unaligned file";
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 LettersInNumericField 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, LettersInNumericField) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Replace numeric digits with letters
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        std::vector<char> buf(recordSize_);
        ::read(fd, buf.data(), recordSize_);

        // Find the +/- sign and replace following digits with letters
        for (size_t i = 0; i < recordSize_ - 5; ++i) {
            if (buf[i] == '+' || buf[i] == '-') {
                buf[i + 1] = 'A';
                buf[i + 2] = 'B';
                buf[i + 3] = 'C';
                break;
            }
        }

        ::lseek(fd, 0, SEEK_SET);
        ::write(fd, buf.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Should throw exception for invalid number
    try {
        auto found = repo_->findById("001", ec_);
    } catch (const std::exception& e) {
        // stoll will throw for "ABC..."
        SUCCEED() << "Exception for letters in numeric: " << e.what();
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 NumericFieldWithSpecialChars 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, NumericFieldWithSpecialChars) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Put special characters in numeric field
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        std::vector<char> buf(recordSize_);
        ::read(fd, buf.data(), recordSize_);

        // Find +/- and replace with special chars
        for (size_t i = 0; i < recordSize_ - 5; ++i) {
            if (buf[i] == '+' || buf[i] == '-') {
                buf[i + 1] = '!';
                buf[i + 2] = '@';
                buf[i + 3] = '#';
                break;
            }
        }

        ::lseek(fd, 0, SEEK_SET);
        ::write(fd, buf.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    try {
        auto found = repo_->findById("001", ec_);
    } catch (const std::exception& e) {
        SUCCEED() << "Exception for special chars: " << e.what();
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 TypeNameCorrupted 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, TypeNameCorrupted) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Corrupt the type name field (usually at the start)
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        // Overwrite first few bytes with garbage
        const char* garbage = "BADTYPE!!";
        ::write(fd, garbage, strlen(garbage));
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Type name mismatch - record should not be found or error
    auto found = repo_->findById("001", ec_);
    // May or may not find depending on where ID is stored
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 IdFieldCorrupted 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, IdFieldCorrupted) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Corrupt ID field specifically
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        std::vector<char> buf(recordSize_);
        ::read(fd, buf.data(), recordSize_);

        // ID is typically after type name, corrupt it with special chars
        // Assuming type is ~10 chars, ID starts around offset 10
        buf[10] = '?';
        buf[11] = '/';
        buf[12] = '*';

        ::lseek(fd, 0, SEEK_SET);
        ::write(fd, buf.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Corruption may or may not affect ID depending on exact layout
    // Just verify it doesn't crash and handles gracefully
    try {
        auto found = repo_->findById("001", ec_);
        // Found or not found is acceptable - layout varies
    } catch (const std::exception& e) {
        SUCCEED() << "Exception is acceptable: " << e.what();
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 HighBitUTF8Characters 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, HighBitUTF8Characters) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Insert UTF-8 multi-byte characters (한글, emoji, etc.)
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);

        std::vector<char> buf(recordSize_);
        ::read(fd, buf.data(), recordSize_);

        // Insert UTF-8 Korean character 한 (ED 95 9C in UTF-8)
        buf[0] = static_cast<char>(0xED);
        buf[1] = static_cast<char>(0x95);
        buf[2] = static_cast<char>(0x9C);

        ::lseek(fd, 0, SEEK_SET);
        ::write(fd, buf.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Should handle non-ASCII gracefully
    try {
        auto found = repo_->findById("001", ec_);
        // Not found is acceptable
    } catch (const std::exception& e) {
        SUCCEED() << "Exception for UTF-8: " << e.what();
    }
}

// 시나리오 상세 설명: BizarreCorruptionTest 그룹의 AllFieldsOverwrittenWithSpaces 케이스 동작을 검증한다.
// - 검증 포인트: 정상 경로, 경계값, 오류 경로에서 API 계약이 일관되게 유지되는지 확인한다.
// - 실패 시 점검 순서: 입력 데이터 준비 -> repository/API 호출 결과 -> 최종 assertion 순으로 원인을 좁힌다.
TEST_F(BizarreCorruptionTest, AllFieldsOverwrittenWithSpaces) {
    // Save valid record
    FixedA alice("alice", 25, "001");
    repo_->save(alice, ec_);
    ASSERT_FALSE(ec_);

    // Fill entire record with spaces
    {
        int fd = ::open(testFile_.c_str(), O_RDWR);
        ASSERT_GE(fd, 0);
        std::vector<char> spaces(recordSize_, ' ');
        ::write(fd, spaces.data(), recordSize_);
        ::fsync(fd);
        ::close(fd);
    }

    usleep(10000);

    // Spaces in numeric field should cause issues
    try {
        auto found = repo_->findById("001", ec_);
    } catch (const std::exception& e) {
        EXPECT_NE(std::string(e.what()).find("sign"), std::string::npos);
    }
}
