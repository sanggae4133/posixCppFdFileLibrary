#pragma once
/// @file UniqueFd.hpp
/// @brief RAII wrapper for POSIX file descriptors (internal implementation)

#include <unistd.h>

namespace FdFile {
namespace detail {

/// @brief RAII wrapper for POSIX file descriptors
///
/// Manages file descriptors with std::unique_ptr-like semantics.
/// Automatically calls close() on destruction.
///
/// @note This class is for internal library use.
class UniqueFd {
  public:
    /// @brief Default constructor. Initializes with invalid fd(-1)
    UniqueFd() noexcept = default;

    /// @brief Takes ownership of a file descriptor
    /// @param fd File descriptor to own
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    /// @brief Destructor. Closes fd if valid
    ~UniqueFd() { reset(); }

    // Copy prohibited
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    /// @brief Move constructor
    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

    /// @brief Move assignment operator
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    /// @brief Returns the file descriptor value
    /// @return Internal fd value (-1 if invalid)
    int get() const noexcept { return fd_; }

    /// @brief Checks if file descriptor is valid
    /// @return true if fd >= 0
    bool valid() const noexcept { return fd_ >= 0; }

    /// @brief Bool conversion operator
    explicit operator bool() const noexcept { return valid(); }

    /// @brief Releases ownership (returns fd without closing)
    /// @return Previous fd value
    int release() noexcept {
        int tmp = fd_;
        fd_ = -1;
        return tmp;
    }

    /// @brief Closes current fd and replaces with new one
    /// @param newFd New file descriptor (default: -1)
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
