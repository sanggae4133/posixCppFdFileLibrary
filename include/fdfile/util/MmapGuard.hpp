#pragma once
/// @file MmapGuard.hpp
/// @brief RAII wrapper for mmap (internal implementation)

#include <cstddef>
#include <sys/mman.h>

namespace FdFile {
namespace detail {

/// @brief RAII wrapper for mmap
///
/// Automatically unmaps memory region on destruction.
class MmapGuard {
  public:
    MmapGuard() = default;

    /// @brief Constructor with mapped memory
    /// @param ptr Mapped memory pointer
    /// @param size Size of mapped region
    MmapGuard(void* ptr, size_t size) : ptr_(ptr), size_(size) {
        if (ptr_ == MAP_FAILED) {
            ptr_ = nullptr;
            size_ = 0;
        }
    }

    ~MmapGuard() { reset(); }

    // Copy prohibited
    MmapGuard(const MmapGuard&) = delete;
    MmapGuard& operator=(const MmapGuard&) = delete;

    // Move allowed
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

    /// @brief Returns mapped memory pointer
    void* get() const noexcept { return ptr_; }
    
    /// @brief Returns mapped region size
    size_t size() const noexcept { return size_; }
    
    /// @brief Checks if mapping is valid
    bool valid() const noexcept { return ptr_ != nullptr; }
    
    /// @brief Bool conversion operator
    explicit operator bool() const noexcept { return valid(); }

    /// @brief Returns data pointer as char*
    char* data() const noexcept { return static_cast<char*>(ptr_); }

    /// @brief Unmaps current region
    void reset() noexcept {
        if (ptr_) {
            ::munmap(ptr_, size_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }

    /// @brief Replaces with new mapping
    /// @param ptr New mapped memory pointer
    /// @param size New region size
    void reset(void* ptr, size_t size) noexcept {
        reset();
        if (ptr != MAP_FAILED) {
            ptr_ = ptr;
            size_ = size;
        }
    }

    /// @brief Syncs changes to disk
    /// @param async If true, use MS_ASYNC; otherwise MS_SYNC
    /// @return true on success
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
