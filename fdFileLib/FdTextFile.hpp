#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <system_error>
#include <filesystem>
#include <string_view>
#include <cstring>

#include "TextRecordBase.hpp"
#include "textFormatUtil.hpp"
#include "UniqueFd.hpp"
#include "FileLockGuard.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

namespace FdFile {

namespace fs = std::filesystem;

class FdTextFile {
public:
    struct TypeSpec {
        std::string typeName;
        std::function<std::unique_ptr<TextRecordBase>()> factory;
    };

    FdTextFile(std::string path,
               std::vector<TypeSpec> types,
               std::error_code& ec,
               int flags = O_CREAT | O_RDWR,
               mode_t mode = 0644)
        : path_(std::move(path)), types_(std::move(types)) {
        ec.clear();

        // 타입 고정, 중복 방지
        for (auto& t : types_) {
            if (!t.factory || t.typeName.empty()) { ec = std::make_error_code(std::errc::invalid_argument); return; }
            if (typeMap_.count(t.typeName)) { ec = std::make_error_code(std::errc::invalid_argument); return; }
            typeMap_[t.typeName] = std::move(t.factory);
        }

        // 디렉토리 생성
        fs::path p(path_);
        fs::path dir = p.parent_path();
        if (!dir.empty()) {
            std::error_code fec;
            if (!fs::exists(dir, fec)) {
                if (fec) { ec = fec; return; }
                fs::create_directories(dir, fec);
                if (fec) { ec = fec; return; }
            } else if (!fs::is_directory(dir, fec)) {
                ec = std::make_error_code(std::errc::not_a_directory);
                return;
            }
        }

#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
        fd_.reset(::open(path_.c_str(), flags, mode));
        if (!fd_) { ec = std::error_code(errno, std::generic_category()); return; }

        // ✅ 기존 파일 내용이 등록된 타입/포맷과 맞는지 전체 검증
        // 하나라도 깨져 있으면 생성 실패로 처리
        if (!validateExisting(ec)) {
            fd_.reset();
            return;
        }
    }

    ~FdTextFile() = default;

    FdTextFile(const FdTextFile&) = delete;
    FdTextFile& operator=(const FdTextFile&) = delete;

    FdTextFile(FdTextFile&&) noexcept = default;
    FdTextFile& operator=(FdTextFile&&) noexcept = default;

    bool ok() const { return fd_.valid(); }

    bool append(const TextRecordBase& rec, bool doFsync, std::error_code& ec) {
        ec.clear();
        if (!ok()) { ec = std::make_error_code(std::errc::bad_file_descriptor); return false; }

        if (typeMap_.find(rec.typeName()) == typeMap_.end()) {
            ec = std::make_error_code(std::errc::not_supported);
            return false;
        }

        std::vector<std::pair<std::string, std::pair<bool, std::string>>> fields;
        rec.toKv(fields);

        std::string line = util::formatLine(rec.typeName(), fields);
        line.push_back('\n');

        // 우리끼리 tearing 방지(상대가 lock 무시하면 어쩔 수 없음. 대신 read에서 포맷 깨짐 감지)
        FileLockGuard lk(fd_.get(), FileLockGuard::Mode::Exclusive, ec);
        if (ec) return false;

        bool okWrite = false;
        do {
            if (::lseek(fd_.get(), 0, SEEK_END) < 0) { ec = std::error_code(errno, std::generic_category()); break; }
            if (!writeAll(line.data(), line.size(), ec)) break;
            if (doFsync && !sync(ec)) break;
            okWrite = true;
        } while (0);
        return okWrite;
    }

    // offset에서 레코드 1개 읽기. 변경된 값이 있으면 "그대로" 읽힘.
    // 포맷 깨짐/중간 잘림이면 에러.
    bool readAt(off_t offset, std::unique_ptr<TextRecordBase>& out, off_t& nextOffset, std::error_code& ec) {
        ec.clear();
        out.reset();
        nextOffset = offset;

        if (!ok()) { ec = std::make_error_code(std::errc::bad_file_descriptor); return false; }

        FileLockGuard lk(fd_.get(), FileLockGuard::Mode::Shared, ec);
        if (ec) return false;

        std::string line;
        bool okRead = false;
        do {
            // seek가 들어가면 버퍼 기반 리더 상태를 리셋해야 함
            resetReadBuffer();
            if (::lseek(fd_.get(), offset, SEEK_SET) < 0) { ec = std::error_code(errno, std::generic_category()); break; }

            // ✅ 마지막 줄 개행 없어도 허용
            // ✅ 빈 라인/주석 라인(#...)은 스킵
            if (!readNextRecordLine(line, nextOffset, ec)) break;

            okRead = true;
        } while (0);
        if (!okRead) return false;

        std::string type;
        std::unordered_map<std::string, std::pair<bool, std::string>> kv;
        if (!util::parseLine(line, type, kv, ec)) return false;

        auto it = typeMap_.find(type);
        if (it == typeMap_.end()) { ec = std::make_error_code(std::errc::not_supported); return false; }

        auto obj = it->second();
        if (!obj) { ec = std::make_error_code(std::errc::not_enough_memory); return false; }
        if (std::string(obj->typeName()) != type) { ec = std::make_error_code(std::errc::invalid_argument); return false; }

        if (!obj->fromKv(kv, ec)) return false;

        out = std::move(obj);
        return true;
    }

    bool sync(std::error_code& ec) {
        ec.clear();
        if (::fsync(fd_.get()) < 0) { ec = std::error_code(errno, std::generic_category()); return false; }
        return true;
    }

private:
    // 생성 시점에 기존 파일 전체 스캔:
    // - 각 줄이 parseLine 가능해야 함
    // - type이 등록된 타입이어야 함
    // - factory로 만든 객체의 typeName이 일치해야 함
    // - fromKv가 성공해야 함
    // - 마지막 줄은 개행 없이 끝나도 OK(하지만 잘린/깨진 레코드는 에러)
    bool validateExisting(std::error_code& ec) {
        ec.clear();
        if (!ok()) { ec = std::make_error_code(std::errc::bad_file_descriptor); return false; }

        // 빈 파일이면 OK
        struct stat st{};
        if (::fstat(fd_.get(), &st) < 0) { ec = std::error_code(errno, std::generic_category()); return false; }
        if (st.st_size == 0) return true;

        FileLockGuard lk(fd_.get(), FileLockGuard::Mode::Shared, ec);
        if (ec) return false;

        bool okValidate = false;
        do {
            resetReadBuffer();
            if (::lseek(fd_.get(), 0, SEEK_SET) < 0) { ec = std::error_code(errno, std::generic_category()); break; }

            while (true) {
                std::string line;
                off_t nextOff = 0;
                if (!readNextRecordLine(line, nextOff, ec)) {
                    // EOF면 정상 종료
                    if (ec == std::errc::result_out_of_range) { ec.clear(); okValidate = true; }
                    break;
                }

                std::string type;
                std::unordered_map<std::string, std::pair<bool, std::string>> kv;
                if (!util::parseLine(line, type, kv, ec)) break;

                auto it = typeMap_.find(type);
                if (it == typeMap_.end()) { ec = std::make_error_code(std::errc::not_supported); break; }

                auto obj = it->second();
                if (!obj) { ec = std::make_error_code(std::errc::not_enough_memory); break; }
                if (std::string(obj->typeName()) != type) { ec = std::make_error_code(std::errc::invalid_argument); break; }

                if (!obj->fromKv(kv, ec)) break;
            }
        } while (0);
        return okValidate;
    }

    bool writeAll(const void* buf, size_t n, std::error_code& ec) {
        const char* p = (const char*)buf;
        size_t left = n;
        while (left > 0) {
            ssize_t w = ::write(fd_.get(), p, left);
            if (w < 0) {
                if (errno == EINTR) continue;
                ec = std::error_code(errno, std::generic_category());
                return false;
            }
            if (w == 0) { ec = std::make_error_code(std::errc::io_error); return false; }
            p += w;
            left -= (size_t)w;
        }
        return true;
    }

    void resetReadBuffer() noexcept {
        readBufLen_ = 0;
        readBufPos_ = 0;
        bufFilePos_ = 0;
        hasBufFilePos_ = false;
    }

    // 따옴표(") 밖에서 등장하는 # 부터 라인 끝까지는 주석으로 제거한다.
    // - \" 처럼 escape된 따옴표는 문자열 종료로 보지 않음
    // - 문자열 내부의 #는 주석 시작이 아님
    static void stripCommentOutsideQuotes(std::string& s) {
        bool inStr = false;
        bool escape = false;
        for (size_t i = 0; i < s.size(); ++i) {
            const char c = s[i];
            if (inStr) {
                if (escape) {
                    escape = false;
                    continue;
                }
                if (c == '\\') { escape = true; continue; }
                if (c == '"') { inStr = false; continue; }
                continue;
            } else {
                if (c == '"') { inStr = true; continue; }
                if (c == '#') { s.resize(i); break; }
            }
        }

        // trim right (space/tab/\r)
        while (!s.empty()) {
            char t = s.back();
            if (t == ' ' || t == '\t' || t == '\r') s.pop_back();
            else break;
        }
    }

    static bool isBlankAfterTrimLeft(std::string_view s) {
        size_t i = 0;
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r')) ++i;
        return i == s.size();
    }

    // 버퍼 기반: 4KB 단위로 read + '\n' 스캔
    // - EOF에서 out이 비어있지 않으면 마지막 레코드로 인정 (개행 없어도 OK)
    // - 빈 줄/주석 줄은 상위 readNextRecordLine에서 스킵 가능
    bool readLineRelaxed(std::string& out, off_t& nextOff, std::error_code& ec) {
        out.clear();
        ec.clear();

        if (!hasBufFilePos_) {
            off_t cur = ::lseek(fd_.get(), 0, SEEK_CUR);
            if (cur < 0) { ec = std::error_code(errno, std::generic_category()); return false; }
            bufFilePos_ = cur;
            hasBufFilePos_ = true;
        }
        nextOff = bufFilePos_;

        while (true) {
            // 버퍼에 데이터가 없으면 채우기
            if (readBufPos_ >= readBufLen_) {
                ssize_t r = ::read(fd_.get(), readBuf_, sizeof(readBuf_));
                if (r < 0) {
                    if (errno == EINTR) continue;
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
                readBufLen_ = (size_t)r;
                readBufPos_ = 0;
            }

            // '\n' 찾기
            const char* start = readBuf_ + readBufPos_;
            const char* end = readBuf_ + readBufLen_;
            const char* nl = (const char*)memchr(start, '\n', (size_t)(end - start));
            if (nl) {
                out.append(start, nl);
                size_t consumed = (size_t)((nl + 1) - start);
                readBufPos_ += consumed;
                bufFilePos_ += (off_t)consumed;
                nextOff = bufFilePos_;
                return true;
            }

            // 버퍼 끝까지 모두 라인에 포함
            out.append(start, end);
            size_t consumed = (size_t)(end - start);
            readBufPos_ += consumed;
            bufFilePos_ += (off_t)consumed;
            nextOff = bufFilePos_;
        }
    }

    // ✅ 빈 라인/주석(#...) 라인을 무시하고 “다음 레코드 라인”을 읽는다.
    // - EOF면 result_out_of_range
    bool readNextRecordLine(std::string& out, off_t& nextOff, std::error_code& ec) {
        ec.clear();
        while (true) {
            if (!readLineRelaxed(out, nextOff, ec)) return false;
            stripCommentOutsideQuotes(out);
            if (!isBlankAfterTrimLeft(out)) return true;
            out.clear();
        }
    }

private:
    std::string path_;
    UniqueFd fd_;
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