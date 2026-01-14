#include "FdTextFile.hpp"

#include "textFormatUtil.hpp"
#include "detail/FileLockGuard.hpp"

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace FdFile {

namespace fs = std::filesystem;

FdTextFile::FdTextFile(std::string path, std::vector<TypeSpec> types, std::error_code& ec,
                       int flags, mode_t mode)
    : path_(std::move(path)), types_(std::move(types)) {
    ec.clear();

    // 타입 고정, 중복 방지
    for (auto& t : types_) {
        if (!t.factory || t.typeName.empty()) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        if (typeMap_.count(t.typeName)) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        typeMap_[t.typeName] = std::move(t.factory);
    }

    // 디렉토리 생성
    fs::path p(path_);
    fs::path dir = p.parent_path();
    if (!dir.empty()) {
        std::error_code fec;
        if (!fs::exists(dir, fec)) {
            if (fec) {
                ec = fec;
                return;
            }
            fs::create_directories(dir, fec);
            if (fec) {
                ec = fec;
                return;
            }
        } else if (!fs::is_directory(dir, fec)) {
            ec = std::make_error_code(std::errc::not_a_directory);
            return;
        }
    }

#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    fd_.reset(::open(path_.c_str(), flags, mode));
    if (!fd_) {
        ec = std::error_code(errno, std::generic_category());
        return;
    }

    // ✅ 기존 파일 내용이 등록된 타입/포맷과 맞는지 전체 검증
    // 하나라도 깨져 있으면 생성 실패로 처리
    if (!validateExisting(ec)) {
        fd_.reset();
        return;
    }
}

bool FdTextFile::append(const TextRecordBase& rec, bool doFsync, std::error_code& ec) {
    ec.clear();
    if (!ok()) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

    if (typeMap_.find(rec.typeName()) == typeMap_.end()) {
        ec = std::make_error_code(std::errc::not_supported);
        return false;
    }

    std::vector<std::pair<std::string, std::pair<bool, std::string>>> fields;
    rec.toKv(fields);

    std::string line = util::formatLine(rec.typeName(), fields);
    line.push_back('\n');

    // 우리끼리 tearing 방지(상대가 lock 무시하면 어쩔 수 없음. 대신 read에서 포맷 깨짐 감지)
    detail::FileLockGuard lk(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
    if (ec)
        return false;

    bool okWrite = false;
    do {
        if (::lseek(fd_.get(), 0, SEEK_END) < 0) {
            ec = std::error_code(errno, std::generic_category());
            break;
        }
        if (!writeAll(line.data(), line.size(), ec))
            break;
        if (doFsync && !sync(ec))
            break;
        okWrite = true;
    } while (0);
    return okWrite;
}

bool FdTextFile::readAt(off_t offset, std::unique_ptr<TextRecordBase>& out, off_t& nextOffset,
                        std::error_code& ec) {
    ec.clear();
    out.reset();
    nextOffset = offset;

    if (!ok()) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

    detail::FileLockGuard lk(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
    if (ec)
        return false;

    std::string line;
    bool okRead = false;
    do {
        // seek가 들어가면 버퍼 기반 리더 상태를 리셋해야 함
        resetReadBuffer();
        if (::lseek(fd_.get(), offset, SEEK_SET) < 0) {
            ec = std::error_code(errno, std::generic_category());
            break;
        }

        // ✅ 마지막 줄 개행 없어도 허용
        // ✅ 빈 라인/주석 라인(#...)은 스킵
        if (!readNextRecordLine(line, nextOffset, ec))
            break;

        okRead = true;
    } while (0);
    if (!okRead)
        return false;

    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    if (!util::parseLine(line, type, kv, ec))
        return false;

    auto it = typeMap_.find(type);
    if (it == typeMap_.end()) {
        ec = std::make_error_code(std::errc::not_supported);
        return false;
    }

    auto obj = it->second();
    if (!obj) {
        ec = std::make_error_code(std::errc::not_enough_memory);
        return false;
    }
    if (std::string(obj->typeName()) != type) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }

    if (!obj->fromKv(kv, ec))
        return false;

    out = std::move(obj);
    return true;
}

bool FdTextFile::sync(std::error_code& ec) {
    ec.clear();
    if (::fsync(fd_.get()) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    return true;
}

bool FdTextFile::validateExisting(std::error_code& ec) {
    ec.clear();
    if (!ok()) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

    // 빈 파일이면 OK
    struct stat st{};
    if (::fstat(fd_.get(), &st) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    if (st.st_size == 0)
        return true;

    detail::FileLockGuard lk(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
    if (ec)
        return false;

    bool okValidate = false;
    do {
        resetReadBuffer();
        if (::lseek(fd_.get(), 0, SEEK_SET) < 0) {
            ec = std::error_code(errno, std::generic_category());
            break;
        }

        while (true) {
            std::string line;
            off_t nextOff = 0;
            if (!readNextRecordLine(line, nextOff, ec)) {
                // EOF면 정상 종료
                if (ec == std::errc::result_out_of_range) {
                    ec.clear();
                    okValidate = true;
                }
                break;
            }

            std::string type;
            std::unordered_map<std::string, std::pair<bool, std::string>> kv;
            if (!util::parseLine(line, type, kv, ec))
                break;

            auto it = typeMap_.find(type);
            if (it == typeMap_.end()) {
                ec = std::make_error_code(std::errc::not_supported);
                break;
            }

            auto obj = it->second();
            if (!obj) {
                ec = std::make_error_code(std::errc::not_enough_memory);
                break;
            }
            if (std::string(obj->typeName()) != type) {
                ec = std::make_error_code(std::errc::invalid_argument);
                break;
            }

            if (!obj->fromKv(kv, ec))
                break;
        }
    } while (0);
    return okValidate;
}

bool FdTextFile::writeAll(const void* buf, size_t n, std::error_code& ec) {
    const char* p = static_cast<const char*>(buf);
    size_t left = n;
    while (left > 0) {
        ssize_t w = ::write(fd_.get(), p, left);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            ec = std::error_code(errno, std::generic_category());
            return false;
        }
        if (w == 0) {
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }
        p += w;
        left -= static_cast<size_t>(w);
    }
    return true;
}

void FdTextFile::resetReadBuffer() noexcept {
    readBufLen_ = 0;
    readBufPos_ = 0;
    bufFilePos_ = 0;
    hasBufFilePos_ = false;
}

void FdTextFile::stripCommentOutsideQuotes(std::string& s) {
    bool inStr = false;
    bool escape = false;
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (inStr) {
            if (escape) {
                escape = false;
                continue;
            }
            if (c == '\\') {
                escape = true;
                continue;
            }
            if (c == '"') {
                inStr = false;
                continue;
            }
            continue;
        } else {
            if (c == '"') {
                inStr = true;
                continue;
            }
            if (c == '#') {
                s.resize(i);
                break;
            }
        }
    }

    // trim right (space/tab/\r)
    while (!s.empty()) {
        char t = s.back();
        if (t == ' ' || t == '\t' || t == '\r')
            s.pop_back();
        else
            break;
    }
}

bool FdTextFile::isBlankAfterTrimLeft(std::string_view s) {
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r'))
        ++i;
    return i == s.size();
}

bool FdTextFile::readLineRelaxed(std::string& out, off_t& nextOff, std::error_code& ec) {
    out.clear();
    ec.clear();

    if (!hasBufFilePos_) {
        off_t cur = ::lseek(fd_.get(), 0, SEEK_CUR);
        if (cur < 0) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }
        bufFilePos_ = cur;
        hasBufFilePos_ = true;
    }
    nextOff = bufFilePos_;

    while (true) {
        // 버퍼에 데이터가 없으면 채우기
        if (readBufPos_ >= readBufLen_) {
            ssize_t r = ::read(fd_.get(), readBuf_, sizeof(readBuf_));
            if (r < 0) {
                if (errno == EINTR)
                    continue;
                ec = std::error_code(errno, std::generic_category());
                return false;
            }
            if (r == 0) { // EOF
                if (out.empty()) {
                    ec = std::make_error_code(std::errc::result_out_of_range);
                    return false;
                }
                return true; // 마지막 줄 개행 없음 OK
            }
            readBufLen_ = static_cast<size_t>(r);
            readBufPos_ = 0;
        }

        // '\n' 찾기
        const char* start = readBuf_ + readBufPos_;
        const char* end = readBuf_ + readBufLen_;
        const char* nl =
            static_cast<const char*>(std::memchr(start, '\n', static_cast<size_t>(end - start)));
        if (nl) {
            out.append(start, nl);
            size_t consumed = static_cast<size_t>((nl + 1) - start);
            readBufPos_ += consumed;
            bufFilePos_ += static_cast<off_t>(consumed);
            nextOff = bufFilePos_;
            return true;
        }

        // 버퍼 끝까지 모두 라인에 포함
        out.append(start, end);
        size_t consumed = static_cast<size_t>(end - start);
        readBufPos_ += consumed;
        bufFilePos_ += static_cast<off_t>(consumed);
        nextOff = bufFilePos_;
    }
}

bool FdTextFile::readNextRecordLine(std::string& out, off_t& nextOff, std::error_code& ec) {
    ec.clear();
    while (true) {
        if (!readLineRelaxed(out, nextOff, ec))
            return false;
        stripCommentOutsideQuotes(out);
        if (!isBlankAfterTrimLeft(out))
            return true;
        out.clear();
    }
}

} // namespace FdFile
