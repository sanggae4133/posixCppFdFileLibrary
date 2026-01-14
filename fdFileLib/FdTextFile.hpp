#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <system_error>
#include <filesystem>

#include "TextRecordBase.hpp"
#include "textFormatUtil.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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
        fd_ = ::open(path_.c_str(), flags, mode);
        if (fd_ < 0) { ec = std::error_code(errno, std::generic_category()); return; }
    }

    ~FdTextFile() { if (fd_ >= 0) ::close(fd_); }

    FdTextFile(const FdTextFile&) = delete;
    FdTextFile& operator=(const FdTextFile&) = delete;

    bool ok() const { return fd_ >= 0; }

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
        if (!lockExclusive(ec)) return false;

        bool okWrite = false;
        do {
            if (::lseek(fd_, 0, SEEK_END) < 0) { ec = std::error_code(errno, std::generic_category()); break; }
            if (!writeAll(line.data(), line.size(), ec)) break;
            if (doFsync && !sync(ec)) break;
            okWrite = true;
        } while (0);

        unlockIgnore();
        return okWrite;
    }

    // offset에서 레코드 1개 읽기. 변경된 값이 있으면 "그대로" 읽힘.
    // 포맷 깨짐/중간 잘림이면 에러.
    bool readAt(off_t offset, std::unique_ptr<TextRecordBase>& out, off_t& nextOffset, std::error_code& ec) {
        ec.clear();
        out.reset();
        nextOffset = offset;

        if (!ok()) { ec = std::make_error_code(std::errc::bad_file_descriptor); return false; }

        if (!lockShared(ec)) return false;

        std::string line;
        bool okRead = false;
        do {
            if (::lseek(fd_, offset, SEEK_SET) < 0) { ec = std::error_code(errno, std::generic_category()); break; }

            // ✅ 마지막 줄 개행 없어도 허용
            if (!readLineRelaxed(line, nextOffset, ec)) break;

            okRead = true;
        } while (0);

        unlockIgnore();
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
        if (::fsync(fd_) < 0) { ec = std::error_code(errno, std::generic_category()); return false; }
        return true;
    }

private:
    bool lockShared(std::error_code& ec) {
        ec.clear();
        struct flock fl{};
        fl.l_type = F_RDLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        if (::fcntl(fd_, F_SETLKW, &fl) < 0) { ec = std::error_code(errno, std::generic_category()); return false; }
        return true;
    }

    bool lockExclusive(std::error_code& ec) {
        ec.clear();
        struct flock fl{};
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        if (::fcntl(fd_, F_SETLKW, &fl) < 0) { ec = std::error_code(errno, std::generic_category()); return false; }
        return true;
    }

    void unlockIgnore() {
        struct flock fl{};
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        ::fcntl(fd_, F_SETLK, &fl);
    }

    bool writeAll(const void* buf, size_t n, std::error_code& ec) {
        const char* p = (const char*)buf;
        size_t left = n;
        while (left > 0) {
            ssize_t w = ::write(fd_, p, left);
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

    // ✅ relaxed: EOF에서 줄이 끝나도 out이 비어있지 않으면 마지막 레코드로 인정
    bool readLineRelaxed(std::string& out, off_t& nextOff, std::error_code& ec) {
        out.clear();
        ec.clear();

        off_t cur = ::lseek(fd_, 0, SEEK_CUR);
        if (cur < 0) { ec = std::error_code(errno, std::generic_category()); return false; }
        nextOff = cur;

        char c;
        while (true) {
            ssize_t r = ::read(fd_, &c, 1);
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
                return true;
            }
            nextOff++;
            if (c == '\n') return true;
            out.push_back(c);
        }
    }

private:
    std::string path_;
    int fd_ = -1;
    std::vector<TypeSpec> types_;
    std::unordered_map<std::string, std::function<std::unique_ptr<TextRecordBase>()>> typeMap_;
};

} // namespace FdFile