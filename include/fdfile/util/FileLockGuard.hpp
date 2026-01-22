#pragma once
/// @file FileLockGuard.hpp
/// @brief RAII guard for POSIX fcntl file locking (internal implementation)

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <system_error>

namespace FdFile {
namespace detail {

/// @brief RAII guard for POSIX fcntl file locking
///
/// Acquires lock on construction and releases on destruction.
/// Supports both shared (read) and exclusive (write) locks.
///
/// @note This class is for internal library use.
/// @note Operates in blocking mode (F_SETLKW), waits until lock is acquired.
class FileLockGuard {
  public:
    /// @brief Lock mode
    enum class Mode {
        Shared,   ///< Shared lock (read, multiple processes can acquire simultaneously)
        Exclusive ///< Exclusive lock (write, only one process can acquire)
    };

    /// @brief Default constructor. Creates without acquiring lock
    FileLockGuard() = default;

    /// @brief Constructor that acquires file lock
    /// @param fd File descriptor to lock
    /// @param mode Lock mode
    /// @param ec Error code (set on failure)
    FileLockGuard(int fd, Mode mode, std::error_code& ec) { lock(fd, mode, ec); }

    /// @brief Destructor. Releases lock if held
    ~FileLockGuard() { unlockIgnore(); }

    // Copy prohibited
    FileLockGuard(const FileLockGuard&) = delete;
    FileLockGuard& operator=(const FileLockGuard&) = delete;

    /// @brief Move constructor
    FileLockGuard(FileLockGuard&& other) noexcept : fd_(other.fd_), locked_(other.locked_) {
        other.fd_ = -1;
        other.locked_ = false;
    }

    /// @brief Move assignment operator
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

    /// @brief Acquire file lock
    /// @param fd File descriptor to lock
    /// @param mode Lock mode
    /// @param ec Error code
    /// @return true on success
    /// @note Releases existing lock first if held
    bool lock(int fd, Mode mode, std::error_code& ec) {
        ec.clear();
        unlockIgnore();
        fd_ = fd;

        if (fd_ < 0) {
            ec = std::make_error_code(std::errc::bad_file_descriptor);
            return false;
        }

        struct flock fl{};
        fl.l_type = (mode == Mode::Shared) ? F_RDLCK : F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0; // 0 = entire file
        if (::fcntl(fd_, F_SETLKW, &fl) < 0) {
            ec = std::error_code(errno, std::generic_category());
            fd_ = -1;
            locked_ = false;
            return false;
        }

        locked_ = true;
        return true;
    }

    /// @brief Release lock (ignores errors)
    void unlockIgnore() noexcept {
        if (!locked_ || fd_ < 0)
            return;
        struct flock fl{};
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        ::fcntl(fd_, F_SETLK, &fl);
        locked_ = false;
        fd_ = -1;
    }

    /// @brief Check current lock status
    /// @return true if lock is held
    bool locked() const noexcept { return locked_; }

  private:
    int fd_ = -1;
    bool locked_ = false;
};

} // namespace detail
} // namespace FdFile
