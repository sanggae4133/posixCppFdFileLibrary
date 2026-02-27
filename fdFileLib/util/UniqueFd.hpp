/**
 * @file fdFileLib/util/UniqueFd.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
#pragma once
/// @file UniqueFd.hpp
/// @brief POSIX 파일 디스크립터용 RAII 래퍼 (내부 구현용)

#include <unistd.h>

namespace FdFile {
namespace detail {

/// @brief POSIX 파일 디스크립터용 RAII 래퍼
///
/// std::unique_ptr과 유사한 의미론으로 파일 디스크립터를 관리한다.
/// 소멸 시 자동으로 close()를 호출한다.
///
/// @note 이 클래스는 라이브러리 내부 구현용이다.
class UniqueFd {
  public:
    /// @brief 기본 생성자. 유효하지 않은 fd(-1)로 초기화
    UniqueFd() noexcept = default;

    /// @brief 파일 디스크립터를 받아 소유권을 가져감
    /// @param fd 소유할 파일 디스크립터
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    /// @brief 소멸자. 유효한 fd가 있으면 close() 호출
    ~UniqueFd() { reset(); }

    // 복사 금지
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    /// @brief 이동 생성자
    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

    /// @brief 이동 대입 연산자
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    /// @brief 파일 디스크립터 값 반환
    /// @return 내부 fd 값 (유효하지 않으면 -1)
    int get() const noexcept { return fd_; }

    /// @brief 유효한 파일 디스크립터인지 확인
    /// @return fd >= 0이면 true
    bool valid() const noexcept { return fd_ >= 0; }

    /// @brief bool 변환 연산자
    explicit operator bool() const noexcept { return valid(); }

    /// @brief 소유권 해제 (close 없이 fd 반환)
    /// @return 기존 fd 값
    int release() noexcept {
        int tmp = fd_;
        fd_ = -1;
        return tmp;
    }

    /// @brief 기존 fd를 닫고 새 fd로 교체
    /// @param newFd 새 파일 디스크립터 (기본값: -1)
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
