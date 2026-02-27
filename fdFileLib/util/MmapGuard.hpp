/**
 * @file fdFileLib/util/MmapGuard.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
#pragma once
/// @file MmapGuard.hpp
/// @brief mmap RAII wrapper (내부 구현용)

#include <cstddef>
#include <sys/mman.h>

namespace FdFile {
namespace detail {

/// @brief mmap RAII 래퍼
///
/// 매핑된 메모리 영역을 자동으로 해제한다.
class MmapGuard {
  public:
    MmapGuard() = default;

    MmapGuard(void* ptr, size_t size) : ptr_(ptr), size_(size) {
        if (ptr_ == MAP_FAILED) {
            // 호출자가 MAP_FAILED를 넘겼을 때도 객체를 "유효하지 않은 빈 상태"로 정규화한다.
            // 이후 valid()/bool 연산에서 일관된 결과를 제공하도록 nullptr로 통일한다.
            ptr_ = nullptr;
            size_ = 0;
        }
    }

    ~MmapGuard() { reset(); }

    // 복사 금지
    MmapGuard(const MmapGuard&) = delete;
    MmapGuard& operator=(const MmapGuard&) = delete;

    // 이동 허용
    MmapGuard(MmapGuard&& other) noexcept : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }

    MmapGuard& operator=(MmapGuard&& other) noexcept {
        if (this != &other) {
            // 기존 매핑을 먼저 해제한 뒤 새 매핑 소유권을 가져온다.
            // move assignment 과정에서 매핑 누수를 막기 위한 핵심 순서다.
            reset();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void* get() const noexcept { return ptr_; }
    size_t size() const noexcept { return size_; }
    bool valid() const noexcept { return ptr_ != nullptr; }
    explicit operator bool() const noexcept { return valid(); }

    char* data() const noexcept { return static_cast<char*>(ptr_); }

    void reset() noexcept {
        if (ptr_) {
            // munmap은 커널 자원을 반환하는 핵심 호출이므로,
            // reset을 여러 번 호출해도 안전하도록 null-check 후 정리한다.
            ::munmap(ptr_, size_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }

    /// @brief 새 매핑으로 교체
    void reset(void* ptr, size_t size) noexcept {
        reset();
        if (ptr != MAP_FAILED) {
            ptr_ = ptr;
            size_ = size;
        }
    }

    /// @brief 변경사항을 디스크에 동기화
    bool sync(bool async = false) noexcept {
        if (!ptr_)
            return false;
        // MS_SYNC: 호출 시점까지 동기화 완료를 보장
        // MS_ASYNC: 커널 스케줄에 위임 (지연 허용, 처리량 우선)
        int flags = async ? MS_ASYNC : MS_SYNC;
        return ::msync(ptr_, size_, flags) == 0;
    }

  private:
    void* ptr_ = nullptr;
    size_t size_ = 0;
};

} // namespace detail
} // namespace FdFile
