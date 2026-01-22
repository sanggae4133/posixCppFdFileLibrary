#pragma once
/// @file UniformFixedRepositoryImpl.hpp
/// @brief Fixed-length record repository where all records have the same size (Template)

#include "../util/FileLockGuard.hpp"
#include "../util/MmapGuard.hpp"
#include "../util/UniqueFd.hpp"
#include "RecordRepository.hpp"

#include <cstring>
#include <fcntl.h>
#include <memory>
#include <optional>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace FdFile {

/// @brief Fixed-length record repository implementation
/// @tparam T Concrete record type (inherits from FixedRecordBase<T>)
///
/// Features:
/// - O(1) lookup via ID cache (on cache hit)
/// - Automatic detection of external file modifications (mtime-based)
/// - Concurrent access control via FileLock
template <typename T> class UniformFixedRepositoryImpl : public RecordRepository<T> {
  public:
    /// @brief Constructor
    /// @param path File path for the repository
    /// @param ec Error code set on failure
    UniformFixedRepositoryImpl(const std::string& path, std::error_code& ec) : path_(path) {
        ec.clear();

        // 1. Calculate record size
        T temp;
        recordSize_ = temp.recordSize();
        if (recordSize_ == 0) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }

        // 2. Open file
        int flags = O_CREAT | O_RDWR;
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
        fd_.reset(::open(path_.c_str(), flags, 0644));
        if (!fd_) {
            ec = std::error_code(errno, std::generic_category());
            return;
        }

        // 3. Initialize file size and mtime
        struct stat st{};
        if (::fstat(fd_.get(), &st) < 0) {
            ec = std::error_code(errno, std::generic_category());
            return;
        }
        if (st.st_size > 0 && (static_cast<size_t>(st.st_size) % recordSize_) != 0) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }

        lastMtime_ = st.st_mtime;
        lastSize_ = st.st_size;

        if (st.st_size > 0) {
            if (!remapFile(ec))
                return;
            rebuildCache(ec);
        }
    }

    ~UniformFixedRepositoryImpl() = default;

    // Copy prohibited
    UniformFixedRepositoryImpl(const UniformFixedRepositoryImpl&) = delete;
    UniformFixedRepositoryImpl& operator=(const UniformFixedRepositoryImpl&) = delete;

    // Move allowed
    UniformFixedRepositoryImpl(UniformFixedRepositoryImpl&&) = default;
    UniformFixedRepositoryImpl& operator=(UniformFixedRepositoryImpl&&) = default;

    // =========================================================================
    // RecordRepository Interface Implementation
    // =========================================================================

    bool save(const T& record, std::error_code& ec) override {
        detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
        if (ec)
            return false;

        if (record.recordSize() != recordSize_) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        // Check for external modifications
        if (!checkAndRefreshCache(ec))
            return false;

        auto idxOpt = findIdxByIdCached(record.getId());

        if (idxOpt) {
            // Update
            if (!remapFile(ec))
                return false;
            char* dst = static_cast<char*>(mmap_.data()) + (*idxOpt * recordSize_);
            record.serialize(dst);
            return mmap_.sync();
        } else {
            // Insert
            struct stat st{};
            ::fstat(fd_.get(), &st);
            size_t newSize = st.st_size + recordSize_;
            if (::ftruncate(fd_.get(), newSize) != 0) {
                ec = std::error_code(errno, std::generic_category());
                return false;
            }
            if (!remapFile(ec))
                return false;

            char* dst = static_cast<char*>(mmap_.data()) + st.st_size;
            record.serialize(dst);

            // Update cache
            size_t newIdx = st.st_size / recordSize_;
            idCache_[record.getId()] = newIdx;
            updateFileStats();

            return mmap_.sync();
        }
    }

    bool saveAll(const std::vector<const T*>& records, std::error_code& ec) override {
        for (const auto* r : records) {
            if (!save(*r, ec))
                return false;
        }
        return true;
    }

    std::vector<std::unique_ptr<T>> findAll(std::error_code& ec) override {
        detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
        if (ec)
            return {};

        ec.clear();
        std::vector<std::unique_ptr<T>> res;

        if (!checkAndRefreshCache(ec))
            return res;

        if (!remapFile(ec))
            return res;

        size_t cnt = slotCount();

        for (size_t i = 0; i < cnt; ++i) {
            auto rec = std::make_unique<T>();
            const char* buf = static_cast<const char*>(mmap_.data()) + i * recordSize_;
            if (rec->deserialize(buf, ec)) {
                res.push_back(std::move(rec));
            } else {
                // Deserialize failed - possibly corrupt from external modification
                return res;
            }
        }
        return res;
    }

    std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) override {
        detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
        if (ec)
            return nullptr;

        ec.clear();

        if (!checkAndRefreshCache(ec))
            return nullptr;

        // O(1) cache lookup
        auto idxOpt = findIdxByIdCached(id);
        if (!idxOpt) {
            return nullptr;
        }

        if (!remapFile(ec))
            return nullptr;

        size_t idx = *idxOpt;
        const char* buf = static_cast<const char*>(mmap_.data()) + idx * recordSize_;

        T temp;
        if (temp.deserialize(buf, ec)) {
            return std::make_unique<T>(temp);
        }
        return nullptr;
    }

    bool deleteById(const std::string& id, std::error_code& ec) override {
        detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
        if (ec)
            return false;

        if (!checkAndRefreshCache(ec))
            return false;

        auto idxOpt = findIdxByIdCached(id);
        if (!idxOpt)
            return true; // not found

        size_t idx = *idxOpt;
        size_t cnt = slotCount();

        // Simple Shift (O(N) data move)
        char* base = static_cast<char*>(mmap_.data());
        char* dst = base + idx * recordSize_;
        char* src = dst + recordSize_;
        size_t moveBytes = (cnt - 1 - idx) * recordSize_;
        if (moveBytes > 0) {
            std::memmove(dst, src, moveBytes);
        }

        size_t newSize = (cnt - 1) * recordSize_;
        mmap_.reset();
        if (::ftruncate(fd_.get(), newSize) != 0) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }

        // Rebuild entire cache since indices shifted
        rebuildCache(ec);
        return true;
    }

    bool deleteAll(std::error_code& ec) override {
        detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
        if (ec)
            return false;

        mmap_.reset();
        if (::ftruncate(fd_.get(), 0) != 0) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }

        // Clear cache
        idCache_.clear();
        updateFileStats();
        return true;
    }

    size_t count(std::error_code& ec) override {
        detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
        if (ec)
            return 0;

        if (!checkAndRefreshCache(ec))
            return 0;

        if (!remapFile(ec))
            return 0;
        return slotCount();
    }

    bool existsById(const std::string& id, std::error_code& ec) override {
        detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
        if (ec)
            return false;

        if (!checkAndRefreshCache(ec))
            return false;

        return findIdxByIdCached(id).has_value();
    }

  private:
    /// @brief Detect file mtime/size changes and refresh cache
    bool checkAndRefreshCache(std::error_code& ec) {
        struct stat st{};
        if (::fstat(fd_.get(), &st) < 0) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }

        // Detect external modifications
        if (st.st_mtime != lastMtime_ || static_cast<size_t>(st.st_size) != lastSize_) {
            // File size not divisible by record size means corrupt
            if (st.st_size > 0 && (static_cast<size_t>(st.st_size) % recordSize_) != 0) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }

            if (!remapFile(ec))
                return false;

            rebuildCache(ec);
            if (ec)
                return false;

            lastMtime_ = st.st_mtime;
            lastSize_ = st.st_size;
        }
        return true;
    }

    /// @brief Rebuild entire cache
    void rebuildCache(std::error_code& ec) {
        idCache_.clear();

        size_t cnt = slotCount();
        T temp;

        for (size_t i = 0; i < cnt; ++i) {
            const char* buf = static_cast<const char*>(mmap_.data()) + i * recordSize_;
            if (temp.deserialize(buf, ec)) {
                idCache_[temp.getId()] = i;
            } else {
                // On deserialize failure, return error
                idCache_.clear();
                return;
            }
        }
        ec.clear();
    }

    /// @brief O(1) ID lookup via cache
    std::optional<size_t> findIdxByIdCached(const std::string& id) {
        auto it = idCache_.find(id);
        if (it != idCache_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /// @brief Update file mtime/size
    void updateFileStats() {
        struct stat st{};
        if (::fstat(fd_.get(), &st) == 0) {
            lastMtime_ = st.st_mtime;
            lastSize_ = st.st_size;
        }
    }

    bool remapFile(std::error_code& ec) {
        ec.clear();
        struct stat st{};
        if (::fstat(fd_.get(), &st) < 0) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }
        if (st.st_size == 0) {
            mmap_.reset();
            return true;
        }
        void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_.get(), 0);
        if (ptr == MAP_FAILED) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }
        mmap_.reset(ptr, st.st_size);
        return true;
    }

    size_t slotCount() const {
        if (!mmap_)
            return 0;
        return mmap_.size() / recordSize_;
    }

    std::string path_;
    detail::UniqueFd fd_;
    detail::MmapGuard mmap_;
    size_t recordSize_ = 0;

    // ID cache (O(1) lookup)
    std::unordered_map<std::string, size_t> idCache_;

    // For external modification detection
    time_t lastMtime_ = 0;
    size_t lastSize_ = 0;
};

} // namespace FdFile
