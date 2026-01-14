#pragma once

#include <unistd.h>

namespace FdFile {
namespace detail {

// Small RAII wrapper for POSIX file descriptors.
class UniqueFd {
public:
    UniqueFd() noexcept = default;
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    ~UniqueFd() { reset(); }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const noexcept { return fd_; }
    bool valid() const noexcept { return fd_ >= 0; }
    explicit operator bool() const noexcept { return valid(); }

    int release() noexcept {
        int tmp = fd_;
        fd_ = -1;
        return tmp;
    }

    void reset(int newFd = -1) noexcept {
        if (fd_ >= 0)
            ::close(fd_);
        fd_ = newFd;
    }

private:
    int fd_ = -1;
};

} // namespace detail
} // namespace FdFile
