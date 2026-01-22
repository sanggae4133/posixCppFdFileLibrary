#pragma once
/// @file RecordRepository.hpp
/// @brief Repository template interface

#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace FdFile {

/// @brief Record repository interface (template)
/// @tparam T Record type (Duck Typing: requires id(), typeName(), etc.)
template <typename T> class RecordRepository {
  public:
    virtual ~RecordRepository() = default;

    // =========================================================================
    // CRUD Operations
    // =========================================================================

    /// @brief Save single record (Insert or Update)
    /// @param record Record to save
    /// @param ec Error code set on failure
    /// @return true on success
    virtual bool save(const T& record, std::error_code& ec) = 0;

    /// @brief Save multiple records
    /// @param records Vector of record pointers to save
    /// @param ec Error code set on failure
    /// @return true on success
    virtual bool saveAll(const std::vector<const T*>& records, std::error_code& ec) = 0;

    /// @brief Find all records
    /// @param ec Error code set on failure
    /// @return Vector of all records
    virtual std::vector<std::unique_ptr<T>> findAll(std::error_code& ec) = 0;

    /// @brief Find record by ID
    /// @param id Record ID to search
    /// @param ec Error code set on failure
    /// @return Found record or nullptr
    virtual std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) = 0;

    /// @brief Find all records by type
    /// @tparam SubT Subtype to search for
    /// @param ec Error code set on failure
    /// @return Vector of records matching the type
    /// @note Since T may already contain type information, this is for subtype searches
    template <typename SubT> std::vector<std::unique_ptr<SubT>> findAllByType(std::error_code& ec) {
        auto all = findAll(ec);
        std::vector<std::unique_ptr<SubT>> result;
        if (ec)
            return result;

        for (auto& rec : all) {
            if (auto* casted = dynamic_cast<SubT*>(rec.get())) {
                (void)rec.release();
                result.push_back(std::unique_ptr<SubT>(casted));
            }
        }
        return result;
    }

    /// @brief Find record by ID and type
    /// @tparam SubT Subtype to search for
    /// @param id Record ID to search
    /// @param ec Error code set on failure
    /// @return Found record or nullptr
    template <typename SubT>
    std::unique_ptr<SubT> findByIdAndType(const std::string& id, std::error_code& ec) {
        auto found = findById(id, ec);
        if (!found)
            return nullptr;

        if (auto* casted = dynamic_cast<SubT*>(found.get())) {
            (void)found.release();
            return std::unique_ptr<SubT>(casted);
        }
        return nullptr;
    }

    /// @brief Delete record by ID
    /// @param id Record ID to delete
    /// @param ec Error code set on failure
    /// @return true on success
    virtual bool deleteById(const std::string& id, std::error_code& ec) = 0;

    /// @brief Delete all records
    /// @param ec Error code set on failure
    /// @return true on success
    virtual bool deleteAll(std::error_code& ec) = 0;

    /// @brief Get record count
    /// @param ec Error code set on failure
    /// @return Number of records
    virtual size_t count(std::error_code& ec) = 0;

    /// @brief Check if record exists by ID
    /// @param id Record ID to check
    /// @param ec Error code set on failure
    /// @return true if record exists
    virtual bool existsById(const std::string& id, std::error_code& ec) = 0;
};

} // namespace FdFile
