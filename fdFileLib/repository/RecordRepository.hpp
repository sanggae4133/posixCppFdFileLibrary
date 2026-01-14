#pragma once
/// @file RecordRepository.hpp
/// @brief Repository 템플릿 인터페이스

#include "../record/RecordBase.hpp"
#include <memory>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

namespace FdFile {

/// @brief 레코드 저장소 인터페이스 (템플릿)
/// @tparam T 레코드 타입 (Duck Typing: id(), typeName() 등 필요)
template <typename T> class RecordRepository {
    // static_assert(std::is_base_of_v<RecordBase, T>, "T must inherit from RecordBase"); // CRTP
    // 지원을 위해 제거

  public:
    virtual ~RecordRepository() = default;

    // =========================================================================
    // CRUD Operations
    // =========================================================================

    /// @brief 단건 저장 (Insert or Update)
    virtual bool save(const T& record, std::error_code& ec) = 0;

    /// @brief 다건 저장
    virtual bool saveAll(const std::vector<const T*>& records, std::error_code& ec) = 0;

    /// @brief 전체 조회
    virtual std::vector<std::unique_ptr<T>> findAll(std::error_code& ec) = 0;

    /// @brief ID로 조회
    virtual std::unique_ptr<T> findById(const std::string& id, std::error_code& ec) = 0;

    /// @brief 조건 조회 (Type)
    /// @note T가 이미 타입 정보를 포함할 수 있으므로, 하위 타입 검색용
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

    /// @brief ID + Type 조회
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

    /// @brief 삭제
    virtual bool deleteById(const std::string& id, std::error_code& ec) = 0;

    /// @brief 전체 삭제
    virtual bool deleteAll(std::error_code& ec) = 0;

    /// @brief 개수 반환
    virtual size_t count(std::error_code& ec) = 0;

    /// @brief 존재 여부 확인
    virtual bool existsById(const std::string& id, std::error_code& ec) = 0;
};

} // namespace FdFile
