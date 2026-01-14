#pragma once
/// @file FixedB.hpp
/// @brief 고정 길이 레코드 예제 B (간소화된 매크로 사용)

#include "record/FieldMeta.hpp"
#include "record/FixedRecordBase.hpp"

#include <cstring>

/// @brief 고정 길이 레코드 예제 B
class FixedB : public FdFile::FixedRecordBase<FixedB> {
  public:
    char title[20]; ///< 제목 (20바이트)
    int64_t cost;   ///< 비용 (19자리 고정)

  private:
    /// @brief 필드 메타데이터 튜플
    auto fields() const { return std::make_tuple(FD_STR(title), FD_NUM(cost)); }

    /// @brief 멤버 초기화
    void initMembers() {
        std::memset(title, 0, sizeof(title));
        cost = 0;
    }

    // 공통 메서드 자동 생성
    FD_RECORD_IMPL(FixedB, "FixedB", 10, 10)

  public:
    /// @brief 커스텀 생성자
    FixedB(const char* t, int64_t c, const char* idStr) {
        initMembers();
        if (t)
            std::strncpy(title, t, sizeof(title));
        cost = c;
        setId(idStr);
        defineLayout();
    }
};
