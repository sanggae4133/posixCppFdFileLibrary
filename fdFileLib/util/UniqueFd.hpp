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
