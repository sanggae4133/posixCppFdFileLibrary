#pragma once
/// @file UniformFixedRepositoryImpl.hpp
/// @brief 모든 레코드가 동일 크기인 고정 길이 저장소 (Template)

#include "../record/FixedRecordBase.hpp"
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

/// @brief 고정 길이 레코드 저장소 구현체
/// @tparam T 구체적인 레코드 타입 (FixedRecordBase<T> 상속)
template <typename T> class UniformFixedRepositoryImpl : public RecordRepository<T> {
  public:
    UniformFixedRepositoryImpl(const std::string& path, std::error_code& ec) : path_(path) {
        ec.clear();

        // 1. 레코드 사이즈 계산
        // T는 기본 생성자가 있어야 하며, 생성자에서 레이아웃이 정의되어야 함
        T temp;
        // 만약 매크로를 안 쓰고 커스텀 생성자만 있다면 여기서 컴파일 에러 가능.
        // 하지만 FixedRecord 컨셉상 기본 정의가 필수.
        recordSize_ = temp.recordSize();
        if (recordSize_ == 0) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }

        // 2. 파일 열기
        int flags = O_CREAT | O_RDWR;
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
        fd_.reset(::open(path_.c_str(), flags, 0644));
        if (!fd_) {
            ec = std::error_code(errno, std::generic_category());
            return;
        }

        // 3. 파일 크기 검증
        struct stat st{};
        if (::fstat(fd_.get(), &st) < 0) {
            ec = std::error_code(errno, std::generic_category());
            return;
        }
        if (st.st_size > 0 && (static_cast<size_t>(st.st_size) % recordSize_) != 0) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }

        if (st.st_size > 0) {
            remapFile(ec);
        }
    }

    ~UniformFixedRepositoryImpl() = default;

    // =========================================================================
    // RecordRepository Interface Implementation
    // =========================================================================

    bool save(const T& record, std::error_code& ec) override {
        if (record.recordSize() != recordSize_) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        auto idxOpt = findIdxById(record.id(), ec);
        if (ec)
            return false;

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
        ec.clear();
        std::vector<std::unique_ptr<T>> res;

        if (!remapFile(ec))
            return res;
        size_t cnt = slotCount();

        for (size_t i = 0; i < cnt; ++i) {
            auto rec = std::make_unique<T>();
            const char* buf = static_cast<const char*>(mmap_.data()) + i * recordSize_;
            if (rec->deserialize(buf, ec)) {
                res.push_back(std::move(rec));
            } else {
                // Deserialize 실패 시 일단 중단 or 스킵?
                // 여기선 중단
                return res;
            }
        }
        return res;
    }

    std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) override {
        ec.clear();
        // 최적화 필요 시 캐시 사용 가능. 여기선 순회.
        if (!remapFile(ec))
            return nullptr;

        size_t cnt = slotCount();
        T temp; // 임시 객체

        for (size_t i = 0; i < cnt; ++i) {
            const char* buf = static_cast<const char*>(mmap_.data()) + i * recordSize_;
            if (temp.deserialize(buf, ec)) {
                if (temp.id() == id) {
                    return std::make_unique<T>(temp);
                }
            }
        }
        return nullptr;
    }

    bool deleteById(const std::string& id, std::error_code& ec) override {
        auto idxOpt = findIdxById(id, ec);
        if (!idxOpt)
            return true; // not found

        size_t idx = *idxOpt;
        size_t cnt = slotCount();

        // Simple Shift (O(N) data move)
        // mmap data는 char*로 캐스팅
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
        return true;
    }

    bool deleteAll(std::error_code& ec) override {
        mmap_.reset();
        if (::ftruncate(fd_.get(), 0) != 0) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }
        return true;
    }

    size_t count(std::error_code& ec) override {
        if (!remapFile(ec))
            return 0;
        return slotCount();
    }

    bool existsById(const std::string& id, std::error_code& ec) override {
        auto idx = findIdxById(id, ec);
        return idx.has_value();
    }

  private:
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

    std::optional<size_t> findIdxById(const std::string& id, std::error_code& ec) {
        if (!remapFile(ec))
            return std::nullopt;
        size_t cnt = slotCount();
        T temp;
        for (size_t i = 0; i < cnt; ++i) {
            const char* buf = static_cast<const char*>(mmap_.data()) + i * recordSize_;
            if (temp.deserialize(buf, ec)) {
                if (temp.id() == id)
                    return i;
            }
        }
        return std::nullopt;
    }

    std::string path_;
    detail::UniqueFd fd_;
    detail::MmapGuard mmap_;
    size_t recordSize_ = 0;
};

} // namespace FdFile
