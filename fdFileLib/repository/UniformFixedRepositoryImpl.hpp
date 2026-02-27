/**
 * @file fdFileLib/repository/UniformFixedRepositoryImpl.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
#pragma once
/// @file UniformFixedRepositoryImpl.hpp
/// @brief 모든 레코드가 동일 크기인 고정 길이 저장소 (Template)

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
///
/// 특징:
/// - ID 캐시를 통한 O(1) 조회 (캐시 히트 시)
/// - 외부 파일 수정 자동 감지 (mtime 기반)
/// - FileLock을 통한 동시 접근 제어
template <typename T> class UniformFixedRepositoryImpl : public RecordRepository<T> {
  public:
    UniformFixedRepositoryImpl(const std::string& path, std::error_code& ec) : path_(path) {
        ec.clear();

        // 1) 레코드 정적 크기 산출
        // 템플릿 타입 T의 레이아웃 정의 결과를 기준으로 저장소 slot 크기를 고정한다.
        T temp;
        recordSize_ = temp.recordSize();
        if (recordSize_ == 0) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }

        // 2) 파일 열기
        int flags = O_CREAT | O_RDWR;
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
        fd_.reset(::open(path_.c_str(), flags, 0644));
        if (!fd_) {
            ec = std::error_code(errno, std::generic_category());
            return;
        }

        // 3) 파일 무결성(크기 정합성) 검사 및 stat 스냅샷 초기화
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
            // 기존 데이터가 있으면 매핑 + 캐시 리빌드를 즉시 수행해
            // 생성 직후 조회에서도 O(1) id lookup이 가능하도록 준비한다.
            if (!remapFile(ec))
                return;
            rebuildCache(ec);
        }
    }

    ~UniformFixedRepositoryImpl() = default;

    // 복사 금지
    UniformFixedRepositoryImpl(const UniformFixedRepositoryImpl&) = delete;
    UniformFixedRepositoryImpl& operator=(const UniformFixedRepositoryImpl&) = delete;

    // 이동 허용
    UniformFixedRepositoryImpl(UniformFixedRepositoryImpl&&) = default;
    UniformFixedRepositoryImpl& operator=(UniformFixedRepositoryImpl&&) = default;

    // =========================================================================
    // RecordRepository Interface Implementation
    // =========================================================================

    bool save(const T& record, std::error_code& ec) override {
        // write 경로는 배타 잠금으로 직렬화한다.
        detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
        if (ec)
            return false;

        if (record.recordSize() != recordSize_) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        // 외부 수정 체크
        if (!checkAndRefreshCache(ec))
            return false;

        auto idxOpt = findIdxByIdCached(record.getId());

        if (idxOpt) {
            // Update: 기존 slot 위치에 in-place 직렬화
            if (!remapFile(ec))
                return false;
            char* dst = static_cast<char*>(mmap_.data()) + (*idxOpt * recordSize_);
            record.serialize(dst);
            return mmap_.sync();
        } else {
            // Insert: 파일 확장 -> 재매핑 -> 새 slot에 직렬화
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

            // append된 인덱스를 즉시 캐시에 반영해 다음 조회 비용을 줄인다.
            size_t newIdx = st.st_size / recordSize_;
            idCache_[record.getId()] = newIdx;
            updateFileStats();

            return mmap_.sync();
        }
    }

    bool saveAll(const std::vector<const T*>& records, std::error_code& ec) override {
        // save를 재사용하는 단순 구현.
        // 중간 실패 시 앞선 성공분은 롤백되지 않는다.
        for (const auto* r : records) {
            if (!save(*r, ec))
                return false;
        }
        return true;
    }

    std::vector<std::unique_ptr<T>> findAll(std::error_code& ec) override {
        // read 경로는 shared lock으로 다중 reader를 허용한다.
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
                // Deserialize 실패는 외부 손상 가능성을 의미하므로 즉시 종료한다.
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

        // O(1) 캐시 조회로 slot 인덱스를 바로 찾는다.
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

        // 삭제 대상 뒤쪽 데이터를 한 칸씩 앞으로 당겨 빈 구멍을 메운다.
        // 복잡한 free-list 구조 대신 단순 shift 전략을 사용한다.
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

        // delete 이후 모든 인덱스가 바뀔 수 있으므로 캐시를 전체 재구축한다.
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

        // 파일과 캐시를 동시에 비워 논리/물리 상태를 일치시킨다.
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
    /// @brief 파일 mtime/size 변경 감지 및 캐시 리프레시
    bool checkAndRefreshCache(std::error_code& ec) {
        struct stat st{};
        if (::fstat(fd_.get(), &st) < 0) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }

        // 외부 수정 감지: mtime 또는 size가 바뀌면 재매핑 + 캐시 재구축을 수행한다.
        if (st.st_mtime != lastMtime_ || static_cast<size_t>(st.st_size) != lastSize_) {
            // 파일 크기가 레코드 사이즈로 나누어 떨어지지 않으면 corrupt
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

    /// @brief 캐시 전체 리빌드
    void rebuildCache(std::error_code& ec) {
        idCache_.clear();

        size_t cnt = slotCount();
        T temp;

        for (size_t i = 0; i < cnt; ++i) {
            const char* buf = static_cast<const char*>(mmap_.data()) + i * recordSize_;
            if (temp.deserialize(buf, ec)) {
                idCache_[temp.getId()] = i;
            } else {
                // 일부라도 깨진 slot이 있으면 캐시 전체를 버린다.
                // 부분 캐시를 유지하면 존재하지 않는 id 조회 결과가 왜곡될 수 있다.
                idCache_.clear();
                return;
            }
        }
        ec.clear();
    }

    /// @brief 캐시 기반 O(1) ID 조회
    std::optional<size_t> findIdxByIdCached(const std::string& id) {
        auto it = idCache_.find(id);
        if (it != idCache_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /// @brief 파일 mtime/size 업데이트
    void updateFileStats() {
        struct stat st{};
        // fstat 실패 시 기존 값을 유지해 잘못된 timestamp로 덮어쓰지 않는다.
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
            // 빈 파일은 매핑을 해제한 null 상태로 표현한다.
            mmap_.reset();
            return true;
        }
        // MAP_SHARED 매핑으로 write가 파일에 반영되도록 한다.
        void* ptr = ::mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_.get(), 0);
        if (ptr == MAP_FAILED) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }
        mmap_.reset(ptr, st.st_size);
        return true;
    }

    size_t slotCount() const {
        // recordSize_로 나눈 몫이 곧 레코드 개수다(생성자/refresh에서 정합성 검증 전제).
        if (!mmap_)
            return 0;
        return mmap_.size() / recordSize_;
    }

    std::string path_;
    detail::UniqueFd fd_;
    detail::MmapGuard mmap_;
    size_t recordSize_ = 0;

    // ID 캐시 (O(1) lookup)
    std::unordered_map<std::string, size_t> idCache_;

    // 외부 수정 감지용
    time_t lastMtime_ = 0;
    size_t lastSize_ = 0;
};

} // namespace FdFile
