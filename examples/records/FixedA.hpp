#pragma once
/// @file FixedA.hpp
/// @brief 고정 길이 레코드 예제 A (간소화된 매크로 사용)

#include <fdfile/record/FieldMeta.hpp>
#include <fdfile/record/FixedRecordBase.hpp>

#include <cstring>

/// @brief 고정 길이 레코드 예제 A
class FixedA : public FdFile::FixedRecordBase<FixedA> {
  public:
    char name[10]; ///< 이름 (10바이트)
    int64_t age;   ///< 나이 (19자리 고정)

  private:
    /// @brief 필드 메타데이터 튜플
    auto fields() const { return std::make_tuple(FD_STR(name), FD_NUM(age)); }

    /// @brief 멤버 초기화
    void initMembers() {
        std::memset(name, 0, sizeof(name));
        age = 0;
    }

    // 공통 메서드 자동 생성
    FD_RECORD_IMPL(FixedA, "FixedA", 10, 10)

  public:
    /// @brief 커스텀 생성자
    FixedA(const char* n, int64_t a, const char* idStr) {
        initMembers();
        if (n)
            std::strncpy(name, n, sizeof(name));
        age = a;
        setId(idStr);
        defineLayout();
    }
};
