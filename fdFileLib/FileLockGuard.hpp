#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <system_error>

namespace FdFile {

class FileLockGuard {
public:
    enum class Mode { Shared, Exclusive };

    FileLockGuard() = default;
    FileLockGuard(int fd, Mode mode, std::error_code& ec) { lock(fd, mode, ec); }

    ~FileLockGuard() { unlockIgnore(); }

    FileLockGuard(const FileLockGuard&) = delete;
    FileLockGuard& operator=(const FileLockGuard&) = delete;

    FileLockGuard(FileLockGuard&& other) noexcept
        : fd_(other.fd_), locked_(other.locked_) {
        other.fd_ = -1;
        other.locked_ = false;
    }

    FileLockGuard& operator=(FileLockGuard&& other) noexcept {
        if (this != &other) {
            unlockIgnore();
            fd_ = other.fd_;
            locked_ = other.locked_;
            other.fd_ = -1;
            other.locked_ = false;
        }
        return *this;
    }

    bool lock(int fd, Mode mode, std::error_code& ec) {
        ec.clear();
        unlockIgnore();
        fd_ = fd;

        if (fd_ < 0) { ec = std::make_error_code(std::errc::bad_file_descriptor); return false; }

        struct flock fl{};
        fl.l_type = (mode == Mode::Shared) ? F_RDLCK : F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        if (::fcntl(fd_, F_SETLKW, &fl) < 0) {
            ec = std::error_code(errno, std::generic_category());
            fd_ = -1;
            locked_ = false;
            return false;
        }

        locked_ = true;
        return true;
    }

    void unlockIgnore() noexcept {
        if (!locked_ || fd_ < 0) return;
        struct flock fl{};
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        ::fcntl(fd_, F_SETLK, &fl);
        locked_ = false;
        fd_ = -1;
    }

    bool locked() const noexcept { return locked_; }

private:
    int fd_ = -1;
    bool locked_ = false;
};

} // namespace FdFile
