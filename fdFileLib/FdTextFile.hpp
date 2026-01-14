#pragma once
/// @file FdTextFile.hpp
/// @brief POSIX 파일 디스크립터 기반 텍스트 레코드 Repository 구현

#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "RecordRepository.hpp"
#include "TextRecordBase.hpp"
#include "detail/UniqueFd.hpp"

#include <fcntl.h>
#include <sys/types.h>

namespace FdFile {

/// @brief 파일 기반 레코드 Repository 구현
///
/// 텍스트 파일에 레코드를 저장하고 CRUD 작업을 수행한다.
/// RecordRepository 인터페이스를 구현한다.
///
/// @code
/// std::vector<std::unique_ptr<TextRecordBase>> protos;
/// protos.push_back(std::make_unique<User>());
/// protos.push_back(std::make_unique<Order>());
///
/// std::error_code ec;
/// FdFile::FdTextFile repo("data.txt", std::move(protos), ec);
///
/// User u; u.name = "john"; u.userId = 123;
/// repo.save(u, ec);
///
/// auto all = repo.findAll(ec);
/// auto found = repo.findById("123", ec);
/// repo.deleteById("123", ec);
/// @endcode
class FdTextFile : public RecordRepository {
  public:
    /// @brief Repository 생성
    /// @param path 파일 경로 (부모 디렉토리가 없으면 자동 생성)
    /// @param prototypes 지원할 레코드 타입의 프로토타입 (타입당 하나씩)
    /// @param ec 에러 코드
    /// @note 기존 파일이 있으면 내용 검증, 없으면 새로 생성
    FdTextFile(const std::string& path, std::vector<std::unique_ptr<TextRecordBase>> prototypes,
               std::error_code& ec);

    ~FdTextFile() override = default;

    // 복사 금지
    FdTextFile(const FdTextFile&) = delete;
    FdTextFile& operator=(const FdTextFile&) = delete;

    // 이동 허용
    FdTextFile(FdTextFile&&) noexcept = default;
    FdTextFile& operator=(FdTextFile&&) noexcept = default;

    /// @brief 파일이 정상적으로 열렸는지 확인
    bool ok() const { return fd_.valid(); }

    // =========================================================================
    // RecordRepository 구현
    // =========================================================================

    bool save(const TextRecordBase& record, std::error_code& ec) override;
    bool saveAll(const std::vector<const TextRecordBase*>& records, std::error_code& ec) override;

    std::vector<std::unique_ptr<TextRecordBase>> findAll(std::error_code& ec) override;
    std::unique_ptr<TextRecordBase> findById(const std::string& id, std::error_code& ec) override;

    bool deleteById(const std::string& id, std::error_code& ec) override;
    bool deleteAll(std::error_code& ec) override;

    size_t count(std::error_code& ec) override;
    bool existsById(const std::string& id, std::error_code& ec) override;

    /// @brief 파일 데이터를 디스크에 동기화
    bool sync(std::error_code& ec);

  private:
    bool appendRecord(const TextRecordBase& record, std::error_code& ec);
    bool rewriteAll(const std::vector<std::unique_ptr<TextRecordBase>>& records,
                    std::error_code& ec);
    bool validateExisting(std::error_code& ec);

    bool writeAll(const void* buf, size_t n, std::error_code& ec);
    void resetReadBuffer() noexcept;

    static void stripCommentOutsideQuotes(std::string& s);
    static bool isBlankAfterTrimLeft(std::string_view s);

    bool readLineRelaxed(std::string& out, off_t& nextOff, std::error_code& ec);
    bool readNextRecordLine(std::string& out, off_t& nextOff, std::error_code& ec);

    std::unique_ptr<TextRecordBase> parseRecord(const std::string& line, std::error_code& ec);

  private:
    std::string path_;
    detail::UniqueFd fd_;
    std::unordered_map<std::string, std::unique_ptr<TextRecordBase>> prototypes_;

    // Buffered read state
    char readBuf_[4096];
    size_t readBufLen_ = 0;
    size_t readBufPos_ = 0;
    off_t bufFilePos_ = 0;
    bool hasBufFilePos_ = false;
};

} // namespace FdFile