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
        int flags = async ? MS_ASYNC : MS_SYNC;
        return ::msync(ptr_, size_, flags) == 0;
    }

  private:
    void* ptr_ = nullptr;
    size_t size_ = 0;
};

} // namespace detail
} // namespace FdFile
