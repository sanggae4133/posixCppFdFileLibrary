#pragma once
/// @file VariableFileRepositoryImpl.hpp
/// @brief Variable-length record repository implementation (formerly FdTextFile)

#include "../record/VariableRecordBase.hpp"
#include "../util/UniqueFd.hpp"

#include "RecordRepository.hpp"
#include <unordered_map>

namespace FdFile {

/// @brief Variable-length record repository implementation
/// @details Manages variable-length records in JSON-like text format.
///          Supports caching with automatic detection of external file modifications.
class VariableFileRepositoryImpl : public RecordRepository<VariableRecordBase> {
  public:
    /// @brief Constructor
    /// @param path File path for the repository
    /// @param prototypes Prototype instances for each record type
    /// @param ec Error code set on failure
    VariableFileRepositoryImpl(const std::string& path,
                               std::vector<std::unique_ptr<VariableRecordBase>> prototypes,
                               std::error_code& ec);
    ~VariableFileRepositoryImpl() override = default;

    bool save(const VariableRecordBase& record, std::error_code& ec) override;
    bool saveAll(const std::vector<const VariableRecordBase*>& records,
                 std::error_code& ec) override;
    std::vector<std::unique_ptr<VariableRecordBase>> findAll(std::error_code& ec) override;
    std::unique_ptr<VariableRecordBase> findById(const std::string& id,
                                                 std::error_code& ec) override;
    bool deleteById(const std::string& id, std::error_code& ec) override;
    bool deleteAll(std::error_code& ec) override;
    size_t count(std::error_code& ec) override;
    bool existsById(const std::string& id, std::error_code& ec) override;

  private:
    bool appendRecord(const VariableRecordBase& record, std::error_code& ec);
    bool rewriteAll(const std::vector<std::unique_ptr<VariableRecordBase>>& records,
                    std::error_code& ec);
    bool sync(std::error_code& ec);

    /// @brief Detect file mtime/size changes and refresh cache
    bool checkAndRefreshCache(std::error_code& ec);
    /// @brief Update file stat information
    void updateFileStats();
    /// @brief Load all records to cache
    bool loadAllToCache(std::error_code& ec);
    /// @brief Invalidate cache
    void invalidateCache();

    std::string path_;
    detail::UniqueFd fd_;
    std::unordered_map<std::string, std::unique_ptr<VariableRecordBase>> prototypes_;

    // Cache members
    std::vector<std::unique_ptr<VariableRecordBase>> cache_;
    bool cacheValid_ = false;
    time_t lastMtime_ = 0;
    size_t lastSize_ = 0;
};

} // namespace FdFile
