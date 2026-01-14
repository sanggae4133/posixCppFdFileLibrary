#include "FdTextFile.hpp"

#include "detail/FileLockGuard.hpp"
#include "textFormatUtil.hpp"

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace FdFile {

namespace fs = std::filesystem;

FdTextFile::FdTextFile(const std::string& path,
                       std::vector<std::unique_ptr<TextRecordBase>> prototypes, std::error_code& ec)
    : path_(path) {
    ec.clear();

    // 프로토타입 등록
    for (auto& proto : prototypes) {
        if (!proto) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        std::string tname = proto->typeName();
        if (tname.empty()) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        if (prototypes_.count(tname)) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        prototypes_[tname] = std::move(proto);
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

    int flags = O_CREAT | O_RDWR;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    fd_.reset(::open(path_.c_str(), flags, 0644));
    if (!fd_) {
        ec = std::error_code(errno, std::generic_category());
        return;
    }

    // 기존 파일 내용 검증
    if (!validateExisting(ec)) {
        fd_.reset();
        return;
    }
}

// =============================================================================
// Create / Update
// =============================================================================

bool FdTextFile::save(const TextRecordBase& record, std::error_code& ec) {
    ec.clear();
    if (!ok()) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

    // 타입 확인
    if (prototypes_.find(record.typeName()) == prototypes_.end()) {
        ec = std::make_error_code(std::errc::not_supported);
        return false;
    }

    // 이미 존재하는지 확인
    std::string recordId = record.id();
    if (existsById(recordId, ec)) {
        // Update: 전체 재작성
        auto all = findAll(ec);
        if (ec)
            return false;

        // 기존 레코드를 새 레코드로 교체
        for (auto& r : all) {
            if (r->id() == recordId) {
                r = record.clone();
                break;
            }
        }
        return rewriteAll(all, ec);
    }
    if (ec)
        return false;

    // Insert: append
    return appendRecord(record, ec);
}

bool FdTextFile::saveAll(const std::vector<const TextRecordBase*>& records, std::error_code& ec) {
    ec.clear();
    for (const auto* rec : records) {
        if (!rec) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }
        if (!save(*rec, ec))
            return false;
    }
    return true;
}

// =============================================================================
// Read
// =============================================================================

std::vector<std::unique_ptr<TextRecordBase>> FdTextFile::findAll(std::error_code& ec) {
    ec.clear();
    std::vector<std::unique_ptr<TextRecordBase>> result;

    if (!ok()) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return result;
    }

    detail::FileLockGuard lk(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
    if (ec)
        return result;

    resetReadBuffer();
    if (::lseek(fd_.get(), 0, SEEK_SET) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return result;
    }

    while (true) {
        std::string line;
        off_t nextOff = 0;
        if (!readNextRecordLine(line, nextOff, ec)) {
            if (ec == std::errc::result_out_of_range) {
                ec.clear(); // EOF
            }
            break;
        }

        auto rec = parseRecord(line, ec);
        if (ec)
            break;
        if (rec)
            result.push_back(std::move(rec));
    }

    return result;
}

std::unique_ptr<TextRecordBase> FdTextFile::findById(const std::string& id, std::error_code& ec) {
    ec.clear();

    auto all = findAll(ec);
    if (ec)
        return nullptr;

    for (auto& rec : all) {
        if (rec->id() == id) {
            return std::move(rec);
        }
    }

    return nullptr; // Not found (not an error)
}

// =============================================================================
// Delete
// =============================================================================

bool FdTextFile::deleteById(const std::string& id, std::error_code& ec) {
    ec.clear();

    auto all = findAll(ec);
    if (ec)
        return false;

    std::vector<std::unique_ptr<TextRecordBase>> remaining;
    for (auto& rec : all) {
        if (rec->id() != id) {
            remaining.push_back(std::move(rec));
        }
    }

    return rewriteAll(remaining, ec);
}

bool FdTextFile::deleteAll(std::error_code& ec) {
    ec.clear();
    std::vector<std::unique_ptr<TextRecordBase>> empty;
    return rewriteAll(empty, ec);
}

// =============================================================================
// Utility
// =============================================================================

size_t FdTextFile::count(std::error_code& ec) {
    auto all = findAll(ec);
    if (ec)
        return 0;
    return all.size();
}

bool FdTextFile::existsById(const std::string& id, std::error_code& ec) {
    auto found = findById(id, ec);
    if (ec)
        return false;
    return found != nullptr;
}

bool FdTextFile::sync(std::error_code& ec) {
    ec.clear();
    if (::fsync(fd_.get()) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    return true;
}

// =============================================================================
// Private methods
// =============================================================================

bool FdTextFile::appendRecord(const TextRecordBase& record, std::error_code& ec) {
    ec.clear();

    detail::FileLockGuard lk(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
    if (ec)
        return false;

    if (::lseek(fd_.get(), 0, SEEK_END) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }

    std::vector<std::pair<std::string, std::pair<bool, std::string>>> fields;
    record.toKv(fields);

    std::string line = util::formatLine(record.typeName(), fields);
    line.push_back('\n');

    if (!writeAll(line.data(), line.size(), ec))
        return false;

    return sync(ec);
}

bool FdTextFile::rewriteAll(const std::vector<std::unique_ptr<TextRecordBase>>& records,
                            std::error_code& ec) {
    ec.clear();

    detail::FileLockGuard lk(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
    if (ec)
        return false;

    // Truncate file
    if (::ftruncate(fd_.get(), 0) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    if (::lseek(fd_.get(), 0, SEEK_SET) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }

    // Write all records
    for (const auto& rec : records) {
        std::vector<std::pair<std::string, std::pair<bool, std::string>>> fields;
        rec->toKv(fields);

        std::string line = util::formatLine(rec->typeName(), fields);
        line.push_back('\n');

        if (!writeAll(line.data(), line.size(), ec))
            return false;
    }

    return sync(ec);
}

std::unique_ptr<TextRecordBase> FdTextFile::parseRecord(const std::string& line,
                                                        std::error_code& ec) {
    ec.clear();

    std::string type;
    std::unordered_map<std::string, std::pair<bool, std::string>> kv;
    if (!util::parseLine(line, type, kv, ec))
        return nullptr;

    auto it = prototypes_.find(type);
    if (it == prototypes_.end()) {
        ec = std::make_error_code(std::errc::not_supported);
        return nullptr;
    }

    auto obj = it->second->clone();
    if (!obj) {
        ec = std::make_error_code(std::errc::not_enough_memory);
        return nullptr;
    }

    if (!obj->fromKv(kv, ec))
        return nullptr;

    return obj;
}

bool FdTextFile::validateExisting(std::error_code& ec) {
    ec.clear();
    if (!ok()) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

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
                if (ec == std::errc::result_out_of_range) {
                    ec.clear();
                    okValidate = true;
                }
                break;
            }

            auto rec = parseRecord(line, ec);
            if (ec)
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
        if (readBufPos_ >= readBufLen_) {
            ssize_t r = ::read(fd_.get(), readBuf_, sizeof(readBuf_));
            if (r < 0) {
                if (errno == EINTR)
                    continue;
                ec = std::error_code(errno, std::generic_category());
                return false;
            }
            if (r == 0) {
                if (out.empty()) {
                    ec = std::make_error_code(std::errc::result_out_of_range);
                    return false;
                }
                return true;
            }
            readBufLen_ = static_cast<size_t>(r);
            readBufPos_ = 0;
        }

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
