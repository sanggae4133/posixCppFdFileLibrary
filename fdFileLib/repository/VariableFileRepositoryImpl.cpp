/**
 * @file fdFileLib/repository/VariableFileRepositoryImpl.cpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
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

    // 전달받은 prototype 목록을 typeName 기준으로 정규화한다.
    // 동일 typeName 중복이 들어오면 첫 번째 항목을 우선하고 나머지는 무시한다.
    for (auto& p : prototypes) {
        if (!p)
            continue;
        std::string t = p->typeName();
        if (prototypes_.find(t) == prototypes_.end()) {
            prototypes_[t] = std::move(p);
        }
    }

    // 저장 경로의 상위 디렉토리가 없으면 미리 생성한다.
    // 저장소 생성 시점에 경로 오류를 빨리 드러내기 위한 방어 로직이다.
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
    // 외부 프로세스 변경 여부를 먼저 반영해 stale cache 기반 판단을 방지한다.
    if (!checkAndRefreshCache(ec))
        return false;

    if (existsById(record.id(), ec)) {
        // Update 경로: 전체 목록을 불러와 대상 ID를 교체한 뒤 rewriteAll 수행
        // (line-based 파일 포맷 특성상 in-place update가 어렵기 때문)
        auto all = findAll(ec);
        if (ec)
            return false;

        // clone 결과를 VariableRecordBase로 downcast 가능한 경우에만 교체한다.
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
            // 파일 재작성 전에 캐시를 무효화해 이후 read path가 반드시 재로딩되도록 한다.
            invalidateCache();
            return rewriteAll(all, ec);
        }
        return false;
    } else {
        // Insert 경로는 append만 수행하므로 I/O 비용이 작다.
        invalidateCache();
        return appendRecord(record, ec);
    }
}

bool VariableFileRepositoryImpl::saveAll(const std::vector<const VariableRecordBase*>& records,
                                         std::error_code& ec) {
    // 원자적 batch 트랜잭션은 제공하지 않는다.
    // 중간 실패 시 앞에서 성공한 항목은 이미 반영되어 있을 수 있다.
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

    // 조회 경로는 shared lock으로 동시에 여러 reader를 허용한다.
    detail::FileLockGuard lock(fd_.get(), detail::FileLockGuard::Mode::Shared, ec);
    if (ec)
        return result;

    // 캐시 갱신 확인
    if (!checkAndRefreshCache(ec))
        return result;

    // 내부 캐시 원본을 직접 노출하지 않고 clone 사본을 반환한다.
    // 호출자가 반환 객체를 수정해도 저장소 캐시는 오염되지 않는다.
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
    // 단건 조회도 shared lock을 사용해 write와만 상호배제한다.
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

    // 삭제 대상만 제외한 새 벡터를 구성한 뒤 rewriteAll로 파일을 재작성한다.
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
    // truncate는 배타 잠금 하에서 수행해 다른 프로세스의 동시 write와 충돌하지 않게 한다.
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
    // append는 파일 끝으로 이동 후 단일 write 호출로 수행한다.
    // 레코드 경계는 formatLine이 붙여주는 '\n'으로 유지된다.
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
    // 전체 재작성은 가장 단순하지만 안정적인 업데이트 전략이다.
    // 파일을 비운 뒤 현재 스냅샷(records)을 순서대로 다시 기록한다.
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
    // 둘 중 하나라도 변하면 캐시를 폐기하고 파일을 다시 읽는다.
    if (st.st_mtime != lastMtime_ || static_cast<size_t>(st.st_size) != lastSize_) {
        invalidateCache();
        lastMtime_ = st.st_mtime;
        lastSize_ = st.st_size;
    }

    // cache miss 시 파일 전체를 다시 파싱해 캐시를 구성한다.
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
        // chunk 단위 read 결과를 leftover와 이어 붙여 "완전한 줄" 단위로만 파싱한다.
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
                // parseLine 성공 + prototype 존재 + fromKv 성공인 경우에만 캐시에 반영한다.
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
                                // 역직렬화 실패 객체는 즉시 폐기한다.
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
    // fsync 성공 후 stat을 갱신해 다음 checkAndRefreshCache에서 false positive가 나지 않게 한다.
    if (::fsync(fd_.get()) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    // sync 후 파일 stat 업데이트
    updateFileStats();
    return true;
}

} // namespace FdFile
