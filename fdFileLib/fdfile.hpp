#pragma once

/**
 * @file fdfile.hpp
 * @brief Main convenience header for FdFileLib
 * 
 * Include this single header to access all library functionality.
 * 
 * @code
 * #include <fdfile.hpp>
 * // or
 * #include <fdFileLib/fdfile.hpp>
 * @endcode
 * 
 * @example Basic Usage
 * @code
 * #include <fdfile.hpp>
 * 
 * class User : public FdFile::FixedRecordBase<User> {
 * public:
 *     char name[20];
 *     int64_t age;
 * private:
 *     auto fields() const { return std::make_tuple(FD_STR(name), FD_NUM(age)); }
 *     void initMembers() { std::memset(name, 0, sizeof(name)); age = 0; }
 *     FD_RECORD_IMPL(User, "User", 10, 10)
 * };
 * 
 * int main() {
 *     std::error_code ec;
 *     FdFile::UniformFixedRepositoryImpl<User> repo("users.db", ec);
 *     // ...
 * }
 * @endcode
 */

// =============================================================================
// Record Types
// =============================================================================
#include "record/RecordBase.hpp"
#include "record/FieldMeta.hpp"
#include "record/FixedRecordBase.hpp"
#include "record/VariableRecordBase.hpp"

// =============================================================================
// Repository Types
// =============================================================================
#include "repository/RecordRepository.hpp"
#include "repository/UniformFixedRepositoryImpl.hpp"
#include "repository/VariableFileRepositoryImpl.hpp"

// =============================================================================
// Utilities
// =============================================================================
#include "util/UniqueFd.hpp"
#include "util/MmapGuard.hpp"
#include "util/FileLockGuard.hpp"
#include "util/textFormatUtil.hpp"

/**
 * @namespace FdFile
 * @brief Root namespace for FdFileLib library
 * 
 * All library components are contained within this namespace.
 * 
 * Key components:
 * - Record types: RecordBase, FixedRecordBase, VariableRecordBase
 * - Repositories: UniformFixedRepositoryImpl, VariableFileRepositoryImpl
 * - Utilities: UniqueFd, MmapGuard, FileLockGuard
 */
namespace FdFile {

// Version information
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;
constexpr const char* VERSION_STRING = "1.0.0";

} // namespace FdFile
