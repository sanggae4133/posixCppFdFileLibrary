#pragma once
/// @file RecordRepository.hpp
/// @brief 레코드 저장소 인터페이스 (JPA-style)

#include "TextRecordBase.hpp"

#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace FdFile {

/// @brief 레코드 저장소 인터페이스
///
/// Spring JPA의 CrudRepository와 유사한 인터페이스.
/// 파일, 데이터베이스 등 다양한 백엔드로 구현 가능.
///
/// @code
/// class MyRepository : public RecordRepository { ... };
///
/// MyRepository repo;
/// repo.save(record, ec);
/// auto all = repo.findAll(ec);
/// auto found = repo.findById("123", ec);
/// repo.deleteById("123", ec);
/// @endcode
class RecordRepository {
  public:
    virtual ~RecordRepository() = default;

    // =========================================================================
    // Create / Update
    // =========================================================================

    /// @brief 레코드 저장 (id가 이미 존재하면 업데이트)
    /// @param record 저장할 레코드
    /// @param ec 에러 코드
    /// @return 성공 시 true
    virtual bool save(const TextRecordBase& record, std::error_code& ec) = 0;

    /// @brief 여러 레코드 일괄 저장
    /// @param records 저장할 레코드 포인터 목록
    /// @param ec 에러 코드
    /// @return 성공 시 true
    virtual bool saveAll(const std::vector<const TextRecordBase*>& records,
                         std::error_code& ec) = 0;

    // =========================================================================
    // Read
    // =========================================================================

    /// @brief 모든 레코드 조회
    /// @param ec 에러 코드
    /// @return 모든 레코드 목록
    virtual std::vector<std::unique_ptr<TextRecordBase>> findAll(std::error_code& ec) = 0;

    /// @brief ID로 레코드 조회
    /// @param id 찾을 레코드 ID
    /// @param ec 에러 코드
    /// @return 찾은 레코드 (없으면 nullptr)
    virtual std::unique_ptr<TextRecordBase> findById(const std::string& id,
                                                     std::error_code& ec) = 0;

    /// @brief 특정 타입의 모든 레코드 조회
    /// @tparam T 조회할 레코드 타입
    /// @param ec 에러 코드
    /// @return 해당 타입의 레코드 목록
    template <typename T> std::vector<std::unique_ptr<T>> findAllByType(std::error_code& ec) {
        std::vector<std::unique_ptr<T>> result;
        auto all = findAll(ec);
        if (ec)
            return result;

        T dummy;
        const char* targetType = dummy.typeName();

        for (auto& rec : all) {
            if (std::string(rec->typeName()) == targetType) {
                result.push_back(std::unique_ptr<T>(static_cast<T*>(rec.release())));
            }
        }
        return result;
    }

    // =========================================================================
    // Delete
    // =========================================================================

    /// @brief ID로 레코드 삭제
    /// @param id 삭제할 레코드 ID
    /// @param ec 에러 코드
    /// @return 성공 시 true (레코드가 없어도 true)
    virtual bool deleteById(const std::string& id, std::error_code& ec) = 0;

    /// @brief 모든 레코드 삭제
    /// @param ec 에러 코드
    /// @return 성공 시 true
    virtual bool deleteAll(std::error_code& ec) = 0;

    // =========================================================================
    // Utility
    // =========================================================================

    /// @brief 레코드 개수 조회
    /// @param ec 에러 코드
    /// @return 레코드 개수
    virtual size_t count(std::error_code& ec) = 0;

    /// @brief ID 존재 여부 확인
    /// @param id 확인할 레코드 ID
    /// @param ec 에러 코드
    /// @return 존재하면 true
    virtual bool existsById(const std::string& id, std::error_code& ec) = 0;
};

} // namespace FdFile
