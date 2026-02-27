/**
 * @file examples/records/FixedB.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 API 사용 순서와 레코드 모델링 방식을 빠르게 이해할 수 있도록 제공되는 실행 예제 코드입니다.
 * - 예제는 학습 목적의 최소 흐름을 보여주므로 실제 서비스 코드에서는 에러 처리, 검증, 동시성 제어를 더 강화해야 합니다.
 * - 필드 길이/타입/ID 정책을 변경하면 파일 포맷 결과가 달라질 수 있으므로 출력 데이터와 호환성을 함께 확인해야 합니다.
 * - 예제에서 사용하는 값은 설명용 샘플이며, 운영 환경에서는 입력 정합성과 보안 제약을 별도로 검토해야 합니다.
 * - 신규 예제를 추가할 때는 '정의 -> 저장 -> 조회 -> 검증'의 전체 lifecycle이 끊기지 않도록 구성하는 것이 좋습니다.
 */
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
    /// @details title -> cost 순서가 직렬화 바이트 레이아웃 순서가 된다.
    auto fields() const { return std::make_tuple(FD_STR(title), FD_NUM(cost)); }

    /// @brief 멤버 초기화
    void initMembers() {
        // 고정 길이 문자열 버퍼는 zero-fill로 시작해 디코딩 시 불필요한 garbage를 방지한다.
        std::memset(title, 0, sizeof(title));
        cost = 0;
    }

    // 공통 메서드 자동 생성
    FD_RECORD_IMPL(FixedB, "FixedB", 10, 10)

  public:
    /// @brief 커스텀 생성자
    FixedB(const char* t, int64_t c, const char* idStr) {
        // 레코드 멤버 -> ID -> 레이아웃 순으로 초기화해 serialize 준비 상태를 보장한다.
        initMembers();
        if (t)
            std::strncpy(title, t, sizeof(title));
        cost = c;
        setId(idStr);
        defineLayout();
    }
};
