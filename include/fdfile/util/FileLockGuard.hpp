/**
 * @file fdFileLib/util/FileLockGuard.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
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
        // 동일 객체에서 잠금 대상을 바꾸는 경우를 허용하기 위해 기존 잠금부터 정리한다.
        // 잠금 중첩 상태를 남기지 않도록 RAII 객체의 상태를 항상 단일 잠금으로 유지한다.
        unlockIgnore();
        fd_ = fd;

        if (fd_ < 0) {
            ec = std::make_error_code(std::errc::bad_file_descriptor);
            return false;
        }

        // l_len=0은 "현재 위치부터 EOF까지"가 아니라 "파일 전체" 잠금을 의미한다.
        // 즉 레코드 단위 잠금이 아니라 저장소 파일 전체에 대한 상호배제를 건다.
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
        // 소멸자에서도 호출되므로 unlock 실패를 상위로 전파하지 않는다.
        // 실패하더라도 객체 내부 상태는 "잠금 없음"으로 정리해 이중 해제를 방지한다.
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
