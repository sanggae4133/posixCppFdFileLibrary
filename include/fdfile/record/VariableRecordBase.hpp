#pragma once
/// @file VariableRecordBase.hpp
/// @brief Variable-length record base class (formerly TextRecordBase)

#include "RecordBase.hpp"

#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace FdFile {

/// @brief Variable-length text record base class (JSON-style Key-Value)
/// @details Provides virtual interface for serializing/deserializing data as JSON-style 
///          Key-Value pairs. Inherit from this class to implement concrete variable-length records.
class VariableRecordBase : public RecordBase {
  public:
    /// @brief Virtual destructor
    /// @details Default virtual destructor for polymorphism support.
    virtual ~VariableRecordBase() = default;

    /// @brief Serialize object data to Key-Value pairs
    /// @details Derived classes must implement this to convert member variables to Key-Value format.
    /// @param[out] out Vector to store serialized data.
    ///                 Each element is {Key, {IsString, Value}} format.
    ///                 - Key: Field name
    ///                 - IsString: true if value is string type, false otherwise
    ///                 - Value: String-converted value
    virtual void
    toKv(std::vector<std::pair<std::string, std::pair<bool, std::string>>>& out) const = 0;

    /// @brief Deserialize Key-Value pairs to object
    /// @details Derived classes must implement this to restore member variables from Key-Value data.
    /// @param[in] kv Key-Value data map. {Key, {IsString, Value}} format.
    /// @param[out] ec Error code set on failure
    /// @return true on success, false on failure
    virtual bool fromKv(const std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                        std::error_code& ec) = 0;

    /// @brief Clone object (Deep Copy)
    /// @details Creates and returns a copy of the current object.
    /// @return std::unique_ptr managing the cloned object
    virtual std::unique_ptr<RecordBase> clone() const = 0;
};

} // namespace FdFile
