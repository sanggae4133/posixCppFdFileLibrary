#pragma once
/// @file FixedA.hpp
/// @brief Strict Fixed Layout 예제 A (Inline Macro)

#include "../../fdFileLib/record/FixedRecordBase.hpp"
#include "../../fdFileLib/record/Macros.hpp"
#include <cstdio>
#include <cstring>
#include <string>

class FixedA : public FdFile::FixedRecordBase<FixedA> {
  public:
    char name[10];
    long age;

    // 인라인 정의 및 기본 생성자 자동 생성
    FD_FIXED_DEFS(FixedA, "FixedA", 10, 10, FD_FIELD(name, name, 10, true),
                  FD_FIELD(age, age, 5, false))

    // 커스텀 생성자 (기본 생성자는 매크로가 이미 생성함)
    FixedA(const char* n, long a, long idVal) {
        std::memset(name, 0, sizeof(name));
        if (n)
            std::strncpy(name, n, sizeof(name));
        age = a;
        setRecordId(std::to_string(idVal));
        defineLayout();
    }
};
