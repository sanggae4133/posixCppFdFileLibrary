#include "VariableFileRepositoryImpl.hpp"
#include "../util/textFormatUtil.hpp"
#include "../util/FileLockGuard.hpp"

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
    }
}

bool VariableFileRepositoryImpl::save(const VariableRecordBase& record, std::error_code& ec) {
    if (existsById(record.id(), ec)) {
        // Update
        auto all = findAll(ec);
        if (ec)
            return false;

        // Replace
        bool replaced = false;
        for (auto& r : all) {
            if (r->id() == record.id()) {
                // Clone from input record to unique_ptr
                if (auto* derived = dynamic_cast<const VariableRecordBase*>(&record)) {
                    // Clone trick: record.clone() returns RecordBase, we need VariableRecordBase
                    // But our prototypes are VariableRecordBase, so clone() should work
                    // if we cast it. But wait, we need to deep copy the *input* record.
                    // The input record SHOULD implement clone().
                    auto cloned = record.clone();
                    if (auto* v = dynamic_cast<VariableRecordBase*>(cloned.get())) {
                        (void)cloned.release();
                        r.reset(v);
                        replaced = true;
                    }
                }
                break;
            }
        }
        if (replaced)
            return rewriteAll(all, ec);
        return false;
    } else {
        // Insert
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

    if (::lseek(fd_.get(), 0, SEEK_SET) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return result;
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
                                result.push_back(std::unique_ptr<VariableRecordBase>(v));
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
    return result;
}

std::unique_ptr<VariableRecordBase> VariableFileRepositoryImpl::findById(const std::string& id,
                                                                         std::error_code& ec) {
    auto all = findAll(ec);
    if (ec)
        return nullptr;
    for (auto& r : all) {
        if (r->id() == id)
            return std::move(r);
    }
    return nullptr;
}

bool VariableFileRepositoryImpl::deleteById(const std::string& id, std::error_code& ec) {
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

    if (found)
        return rewriteAll(kept, ec);
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
    return sync(ec);
}

size_t VariableFileRepositoryImpl::count(std::error_code& ec) {
    auto all = findAll(ec);
    return all.size();
}

bool VariableFileRepositoryImpl::existsById(const std::string& id, std::error_code& ec) {
    // Optimized scan possible, but reusing findAll for simplicity
    auto p = findById(id, ec);
    return p != nullptr;
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

bool VariableFileRepositoryImpl::sync(std::error_code& ec) {
    if (::fsync(fd_.get()) < 0) {
        ec = std::error_code(errno, std::generic_category());
        return false;
    }
    return true;
}

} // namespace FdFile
