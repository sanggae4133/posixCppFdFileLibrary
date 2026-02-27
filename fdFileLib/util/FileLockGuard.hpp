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
/// @brief POSIX fcntl 파일 잠금용 RAII 가드 (내부 구현용)

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <system_error>

namespace FdFile {
namespace detail {

/// @brief POSIX fcntl 파일 잠금용 RAII 가드
///
/// 생성 시 잠금을 획득하고 소멸 시 자동으로 해제한다.
/// 공유 잠금(읽기용)과 배타 잠금(쓰기용)을 지원한다.
///
/// @note 이 클래스는 라이브러리 내부 구현용이다.
/// @note 블로킹 모드(F_SETLKW)로 동작하므로 잠금 획득까지 대기한다.
class FileLockGuard {
  public:
    /// @brief 잠금 모드
    enum class Mode {
        Shared,   ///< 공유 잠금 (읽기용, 여러 프로세스가 동시에 획득 가능)
        Exclusive ///< 배타 잠금 (쓰기용, 단일 프로세스만 획득 가능)
    };

    /// @brief 기본 생성자. 잠금 없이 생성
    FileLockGuard() = default;

    /// @brief 파일 잠금을 획득하며 생성
    /// @param fd 잠금할 파일 디스크립터
    /// @param mode 잠금 모드
    /// @param ec 에러 코드 (실패 시 설정)
    FileLockGuard(int fd, Mode mode, std::error_code& ec) { lock(fd, mode, ec); }

    /// @brief 소멸자. 잠금이 있으면 해제
    ~FileLockGuard() { unlockIgnore(); }

    // 복사 금지
    FileLockGuard(const FileLockGuard&) = delete;
    FileLockGuard& operator=(const FileLockGuard&) = delete;

    /// @brief 이동 생성자
    FileLockGuard(FileLockGuard&& other) noexcept : fd_(other.fd_), locked_(other.locked_) {
        other.fd_ = -1;
        other.locked_ = false;
    }

    /// @brief 이동 대입 연산자
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

    /// @brief 파일 잠금 획득
    /// @param fd 잠금할 파일 디스크립터
    /// @param mode 잠금 모드
    /// @param ec 에러 코드
    /// @return 성공 시 true
    /// @note 기존 잠금이 있으면 먼저 해제한다
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
        fl.l_len = 0; // 0 = 파일 전체
        if (::fcntl(fd_, F_SETLKW, &fl) < 0) {
            ec = std::error_code(errno, std::generic_category());
            fd_ = -1;
            locked_ = false;
            return false;
        }

        locked_ = true;
        return true;
    }

    /// @brief 잠금 해제 (에러 무시)
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

    /// @brief 현재 잠금 상태 확인
    /// @return 잠금이 활성화되어 있으면 true
    bool locked() const noexcept { return locked_; }

  private:
    int fd_ = -1;
    bool locked_ = false;
};

} // namespace detail
} // namespace FdFile
