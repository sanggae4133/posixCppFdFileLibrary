#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "TextRecordBase.hpp"
#include "detail/UniqueFd.hpp"

#include <fcntl.h>
#include <sys/types.h>

namespace FdFile {

class FdTextFile {
public:
    struct TypeSpec {
        std::string typeName;
        std::function<std::unique_ptr<TextRecordBase>()> factory;
    };

    FdTextFile(std::string path, std::vector<TypeSpec> types, std::error_code& ec,
               int flags = O_CREAT | O_RDWR, mode_t mode = 0644);

    ~FdTextFile() = default;

    FdTextFile(const FdTextFile&) = delete;
    FdTextFile& operator=(const FdTextFile&) = delete;

    FdTextFile(FdTextFile&&) noexcept = default;
    FdTextFile& operator=(FdTextFile&&) noexcept = default;

    bool ok() const { return fd_.valid(); }

    bool append(const TextRecordBase& rec, bool doFsync, std::error_code& ec);

    // offset에서 레코드 1개 읽기. 변경된 값이 있으면 "그대로" 읽힘.
    // 포맷 깨짐/중간 잘림이면 에러.
    bool readAt(off_t offset, std::unique_ptr<TextRecordBase>& out, off_t& nextOffset,
                std::error_code& ec);

    bool sync(std::error_code& ec);

private:
    // 생성 시점에 기존 파일 전체 스캔
    bool validateExisting(std::error_code& ec);

    bool writeAll(const void* buf, size_t n, std::error_code& ec);
    void resetReadBuffer() noexcept;

    static void stripCommentOutsideQuotes(std::string& s);
    static bool isBlankAfterTrimLeft(std::string_view s);

    bool readLineRelaxed(std::string& out, off_t& nextOff, std::error_code& ec);
    bool readNextRecordLine(std::string& out, off_t& nextOff, std::error_code& ec);

private:
    std::string path_;
    detail::UniqueFd fd_;
    std::vector<TypeSpec> types_;
    std::unordered_map<std::string, std::function<std::unique_ptr<TextRecordBase>()>> typeMap_;

    // buffered read state
    char readBuf_[4096];
    size_t readBufLen_ = 0;
    size_t readBufPos_ = 0;
    off_t bufFilePos_ = 0;       // 파일에서 "버퍼 시작 + readBufPos_"에 해당하는 위치
    bool hasBufFilePos_ = false; // 초기화 여부
};

} // namespace FdFile