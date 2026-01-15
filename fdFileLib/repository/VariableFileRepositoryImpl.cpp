#include "VariableFileRepositoryImpl.hpp"
#include "../util/FileLockGuard.hpp"
#include "../util/textFormatUtil.hpp"

#include <fcntl.h>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace FdFile {

namespace fs = std::filesystem;

VariableFileRepositoryImpl::VariableFileRepositoryImpl(
    const std::string& path, std::vector<std::unique_ptr<VariableRecordBase>> prototypes,
    std::error_code& ec)
    : path_(path) {
    ec.clear();

    for (auto& p : prototypes) {
        if (!p)
            continue;
        std::string t = p->typeName();
        if (prototypes_.find(t) == prototypes_.end()) {
            prototypes_[t] = std::move(p);
        }
    }

    // 디렉토리 생성
    fs::path p(path_);
    fs::path dir = p.parent_path();
    if (!dir.empty()) {
        std::error_code fec;
        if (!fs::exists(dir, fec)) {
            fs::create_directories(dir, fec);
            if (fec) {
                ec = fec;
                return;
            }
        }
    }

    int flags = O_CREAT | O_RDWR;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    fd_.reset(::open(path_.c_str(), flags, 0644));
    if (!fd_) {
        ec = std::error_code(errno, std::generic_category());
        return;
    }

    // 초기 파일 stat 저장 (외부 수정 감지용)
    updateFileStats();
}

bool VariableFileRepositoryImpl::save(const VariableRecordBase& record, std::error_code& ec) {
    // 캐시 갱신 확인
    if (!checkAndRefreshCache(ec))
        return false;

    if (existsById(record.id(), ec)) {
        // Update
        auto all = findAll(ec);
        if (ec)
            return false;

        // Replace
        bool replaced = false;
        for (auto& r : all) {
            if (r->id() == record.id()) {
                auto cloned = record.clone();
                if (auto* v = dynamic_cast<VariableRecordBase*>(cloned.get())) {
                    (void)cloned.release();
                    r.reset(v);
                    replaced = true;
                }
                break;
            }
        }
        if (replaced) {
            invalidateCache();
            return rewriteAll(all, ec);
        }
        return false;
    } else {
        // Insert
        invalidateCache();
        return appendRecord(record, ec);
    }
}

bool VariableFileRepositoryImpl::saveAll(const std::vector<const VariableRecordBase*>& records,
                                         std::error_code& ec) {
    for (const auto* r : records) {
        if (!save(*r, ec))
            return false;
    }
    return true;
}

std::vector<std::unique_ptr<VariableRecordBase>>
VariableFileRepositoryImpl::findAll(std::error_code& ec) {
    ec.clear();
    std::vector<std::unique_ptr<VariableRecordBase>> result;

    if (!fd_) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return result;
    }

    detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
    if (ec)
        return result;

    // 캐시 갱신 확인
    if (!checkAndRefreshCache(ec))
        return result;

    // 캐시에서 복사본 반환
    for (const auto& r : cache_) {
        auto cloned = r->clone();
        if (auto* v = dynamic_cast<VariableRecordBase*>(cloned.get())) {
            (void)cloned.release();
            result.push_back(std::unique_ptr<VariableRecordBase>(v));
        }
    }
    return result;
}

std::unique_ptr<VariableRecordBase> VariableFileRepositoryImpl::findById(const std::string& id,
                                                                         std::error_code& ec) {
    detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
    if (ec)
        return nullptr;

    // 캐시 갱신 확인
    if (!checkAndRefreshCache(ec))
        return nullptr;

    for (const auto& r : cache_) {
        if (r->id() == id) {
            auto cloned = r->clone();
            if (auto* v = dynamic_cast<VariableRecordBase*>(cloned.get())) {
                (void)cloned.release();
                return std::unique_ptr<VariableRecordBase>(v);
            }
        }
    }
    return nullptr;
}

bool VariableFileRepositoryImpl::deleteById(const std::string& id, std::error_code& ec) {
    // 캐시 갱신 확인
    if (!checkAndRefreshCache(ec))
        return false;

    auto all = findAll(ec);
    if (ec)
        return false;

    std::vector<std::unique_ptr<VariableRecordBase>> kept;
    bool found = false;
    for (auto& r : all) {
        if (r->id() == id) {
            found = true;
            continue;
        }
        kept.push_back(std::move(r));
    }

    if (found) {
        invalidateCache();
        return rewriteAll(kept, ec);
    }
    return true; // Not found acts as success
}

bool VariableFileRepositoryImpl::deleteAll(std::error_code& ec) {
    detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
    if (ec)
        return false;
    if (::ftruncate(fd_.get(), 0) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    invalidateCache();
    return sync(ec);
}

size_t VariableFileRepositoryImpl::count(std::error_code& ec) {
    // 캐시 갱신 확인
    if (!checkAndRefreshCache(ec))
        return 0;
    return cache_.size();
}

bool VariableFileRepositoryImpl::existsById(const std::string& id, std::error_code& ec) {
    // 캐시 갱신 확인
    if (!checkAndRefreshCache(ec))
        return false;

    for (const auto& r : cache_) {
        if (r->id() == id)
            return true;
    }
    return false;
}

bool VariableFileRepositoryImpl::appendRecord(const VariableRecordBase& record,
                                              std::error_code& ec) {
    detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
    if (ec)
        return false;

    if (::lseek(fd_.get(), 0, SEEK_END) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }

    std::vector<std::pair<std::string, std::pair<bool, std::string>>> out;
    record.toKv(out);
    std::string line = util::formatLine(record.typeName(), out);

    if (::write(fd_.get(), line.data(), line.size()) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    return sync(ec);
}

bool VariableFileRepositoryImpl::rewriteAll(
    const std::vector<std::unique_ptr<VariableRecordBase>>& records, std::error_code& ec) {
    detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Exclusive, ec);
    if (ec)
        return false;

    if (::ftruncate(fd_.get(), 0) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    if (::lseek(fd_.get(), 0, SEEK_SET) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }

    for (const auto& r : records) {
        std::vector<std::pair<std::string, std::pair<bool, std::string>>> out;
        r->toKv(out);
        std::string line = util::formatLine(r->typeName(), out);
        if (::write(fd_.get(), line.data(), line.size()) < 0) {
            ec = std::error_code(errno, std::generic_category());
            return false;
        }
    }
    return sync(ec);
}

bool VariableFileRepositoryImpl::checkAndRefreshCache(std::error_code& ec) {
    struct stat st{};
    // path로 stat 호출 - fd로 fstat보다 외부 변경 감지가 더 확실함
    if (::stat(path_.c_str(), &st) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }

    // 외부 수정 감지 (mtime 또는 size 변경)
    if (st.st_mtime != lastMtime_ || static_cast<size_t>(st.st_size) != lastSize_) {
        invalidateCache();
        lastMtime_ = st.st_mtime;
        lastSize_ = st.st_size;
    }

    // 캐시가 유효하지 않으면 로드
    if (!cacheValid_) {
        if (!loadAllToCache(ec)) {
            return false;
        }
    }
    return true;
}

void VariableFileRepositoryImpl::updateFileStats() {
    struct stat st{};
    // path로 stat 호출 - 외부 변경 감지를 위해 일관성 유지
    if (::stat(path_.c_str(), &st) == 0) {
        lastMtime_ = st.st_mtime;
        lastSize_ = st.st_size;
    }
}

bool VariableFileRepositoryImpl::loadAllToCache(std::error_code& ec) {
    cache_.clear();

    // 외부 프로세스가 파일에 추가한 경우, fd의 내부 상태(파일 크기)를 갱신하기 위해
    // 먼저 SEEK_END로 이동하여 커널이 최신 파일 크기를 인식하게 함
    (void)::lseek(fd_.get(), 0, SEEK_END);

    if (::lseek(fd_.get(), 0, SEEK_SET) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }

    char buf[4096];
    std::string leftover;
    ssize_t n;

    while ((n = ::read(fd_.get(), buf, sizeof(buf))) > 0) {
        std::string chunk = leftover + std::string(buf, n);
        std::istringstream iss(chunk);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.empty())
                continue;
            // 마지막 줄이 완전하지 않으면 leftover로
            if (iss.eof() && chunk.back() != '\n') {
                leftover = line;
                break;
            } else {
                // Parse line
                std::string type;
                std::unordered_map<std::string, std::pair<bool, std::string>> kv;
                if (util::parseLine(line, type, kv, ec)) {
                    auto it = prototypes_.find(type);
                    if (it != prototypes_.end()) {
                        auto clo = it->second->clone();
                        if (auto* v = dynamic_cast<VariableRecordBase*>(clo.get())) {
                            (void)clo.release();
                            if (v->fromKv(kv, ec)) {
                                cache_.push_back(std::unique_ptr<VariableRecordBase>(v));
                            } else {
                                delete v;
                            }
                        }
                    }
                }
                leftover.clear();
            }
        }
    }

    cacheValid_ = true;
    return true;
}

void VariableFileRepositoryImpl::invalidateCache() {
    cache_.clear();
    cacheValid_ = false;
}

bool VariableFileRepositoryImpl::sync(std::error_code& ec) {
    if (::fsync(fd_.get()) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    // sync 후 파일 stat 업데이트
    updateFileStats();
    return true;
}

} // namespace FdFile
