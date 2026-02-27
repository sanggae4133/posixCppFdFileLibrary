/**
 * @file fdFileLib/repository/RecordRepository.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
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

        // dynamic_cast 성공 객체만 소유권을 SubT 컨테이너로 이전한다.
        // release()를 호출하지 않으면 rec 소멸 시 이중 해제가 발생할 수 있으므로
        // cast 성공 분기에서 소유권 이전 순서를 반드시 유지해야 한다.
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

        // 단건 조회에서도 동일하게 cast 성공 시 ownership을 SubT 포인터로 이동한다.
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
