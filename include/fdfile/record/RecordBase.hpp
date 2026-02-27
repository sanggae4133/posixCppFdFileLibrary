#pragma once
/// @file RecordBase.hpp
/// @brief Base interface for all record types

#include <string>

namespace FdFile {

/// @brief Base interface for all records stored in files
/// @details All record types (fixed-length, variable-length) managed by this library 
///          must inherit from this interface.
///          Defines common functionality (ID retrieval, type name, cloning).
class RecordBase {
  public:
    /// @brief Virtual destructor
    /// @details Default virtual destructor to support polymorphism.
    virtual ~RecordBase() = default;

    /// @brief Returns the record's unique identifier
    /// @return Record ID string. All records must have a unique ID.
    virtual std::string id() const = 0;

    /// @brief Returns the record type name
    /// @return Type name as C-style string. Used as metadata when saving to files.
    virtual const char* typeName() const = 0;
};

} // namespace FdFile
