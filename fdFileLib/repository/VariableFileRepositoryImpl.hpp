/**
 * @file fdFileLib/repository/VariableFileRepositoryImpl.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
#pragma once
/// @file VariableFileRepositoryImpl.hpp
/// @brief 가변 길이 레코드 저장소 구현 (구 FdTextFile)

#include "../record/VariableRecordBase.hpp"
#include "../util/UniqueFd.hpp"

#include "RecordRepository.hpp"
#include <unordered_map>

namespace FdFile {

/// @brief 라인 기반 가변 길이 레코드 저장소 구현체
/// @details
/// - 파일 포맷: `Type { "key": "value", "id": 1 }\n` 형태의 line-delimited text
/// - 프로토타입 맵(prototypes_)을 통해 typeName별 concrete record 복원을 수행한다.
/// - 캐시(cache_)는 read path 성능 최적화를 위해 유지되며, mtime/size 변경 시 즉시 무효화된다.
class VariableFileRepositoryImpl : public RecordRepository<VariableRecordBase> {
  public:
    VariableFileRepositoryImpl(const std::string& path,
                               std::vector<std::unique_ptr<VariableRecordBase>> prototypes,
                               std::error_code& ec);
    ~VariableFileRepositoryImpl() override = default;

    /// @brief 단건 저장 (동일 ID가 있으면 upsert)
    bool save(const VariableRecordBase& record, std::error_code& ec) override;
    /// @brief 다건 저장 (내부적으로 save를 순차 호출)
    bool saveAll(const std::vector<const VariableRecordBase*>& records,
                 std::error_code& ec) override;
    /// @brief 전체 조회
    /// @details 내부 캐시의 clone 사본을 반환해 호출자 수정이 저장소 상태에 영향 주지 않도록 한다.
    std::vector<std::unique_ptr<VariableRecordBase>> findAll(std::error_code& ec) override;
    /// @brief ID 조회
    std::unique_ptr<VariableRecordBase> findById(const std::string& id,
                                                 std::error_code& ec) override;
    /// @brief ID 삭제 (없으면 성공 처리)
    bool deleteById(const std::string& id, std::error_code& ec) override;
    /// @brief 전체 삭제
    bool deleteAll(std::error_code& ec) override;
    /// @brief 레코드 개수
    size_t count(std::error_code& ec) override;
    /// @brief ID 존재 여부
    bool existsById(const std::string& id, std::error_code& ec) override;

  private:
    /// @brief 파일 끝에 레코드 1건 append
    bool appendRecord(const VariableRecordBase& record, std::error_code& ec);
    /// @brief 전달받은 레코드 집합으로 파일 전체를 재작성
    bool rewriteAll(const std::vector<std::unique_ptr<VariableRecordBase>>& records,
                    std::error_code& ec);
    /// @brief 디스크 flush + 내부 stat 갱신
    bool sync(std::error_code& ec);

    /// @brief 파일 mtime/size 변경 감지 및 캐시 갱신
    bool checkAndRefreshCache(std::error_code& ec);
    /// @brief 파일 stat 정보 업데이트
    void updateFileStats();
    /// @brief 캐시에서 전체 레코드 로드
    bool loadAllToCache(std::error_code& ec);
    /// @brief 캐시 무효화
    void invalidateCache();

    std::string path_; ///< 저장 파일 경로
    detail::UniqueFd fd_; ///< 파일 디스크립터 소유자 (RAII)
    std::unordered_map<std::string, std::unique_ptr<VariableRecordBase>> prototypes_; ///< type별 clone 원형

    // 캐시 관련 멤버
    std::vector<std::unique_ptr<VariableRecordBase>> cache_; ///< 최신 파일 스냅샷 캐시
    bool cacheValid_ = false; ///< 캐시 유효 여부
    time_t lastMtime_ = 0; ///< 마지막 관측 mtime
    size_t lastSize_ = 0; ///< 마지막 관측 파일 크기
};

} // namespace FdFile
