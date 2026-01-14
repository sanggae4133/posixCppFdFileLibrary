#pragma once
/// @file FixedB.hpp
/// @brief Strict Fixed Layout 예제 B (Inline Macro)

#include "../../fdFileLib/record/FixedRecordBase.hpp"
#include "../../fdFileLib/record/Macros.hpp"
#include <cstdio>
#include <cstring>
#include <string>

class FixedB : public FdFile::FixedRecordBase<FixedB> {
  public:
    char title[20];
    long cost;

    // 인라인 정의
    FD_FIXED_DEFS(FixedB, "FixedB", 10, 10, FD_FIELD(title, title, 20, true),
                  FD_FIELD(cost, cost, 8, false))

    // 커스텀 생성자
    FixedB(const char* t, long c, long idVal) {
        std::memset(title, 0, sizeof(title));
        if (t)
            std::strncpy(title, t, sizeof(title));
        cost = c;
        setRecordId(std::to_string(idVal));
        defineLayout(); // 필수 호출
    }
};
