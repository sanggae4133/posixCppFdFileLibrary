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
