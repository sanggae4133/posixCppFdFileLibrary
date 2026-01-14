#pragma once
/// @file VariableFileRepositoryImpl.hpp
/// @brief 가변 길이 레코드 저장소 구현 (구 FdTextFile)

#include "../record/VariableRecordBase.hpp"
#include "../util/UniqueFd.hpp"

#include "RecordRepository.hpp"
#include <unordered_map>

namespace FdFile {

class VariableFileRepositoryImpl : public RecordRepository<VariableRecordBase> {
  public:
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

    std::string path_;
    detail::UniqueFd fd_;
    std::unordered_map<std::string, std::unique_ptr<VariableRecordBase>> prototypes_;
};

} // namespace FdFile
