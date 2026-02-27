/**
 * @file fdFileLib/util/textFormatUtil.hpp
 * @brief 코드 이해를 위한 한국어 상세 주석 블록.
 * @details
 * - 이 파일은 POSIX file descriptor 기반 저장소/레코드 처리 흐름의 핵심 구성요소를 담고 있습니다.
 * - 구현의 기본 원칙은 예외 남용을 피하고 std::error_code 중심으로 실패 원인을 호출자에게 전달하는 것입니다.
 * - 직렬화/역직렬화 규약(필드 길이, 문자열 escape, 레코드 경계)은 파일 포맷 호환성과 직접 연결되므로 수정 시 매우 주의해야 합니다.
 * - 동시 접근 시에는 lock 획득 순서, 캐시 무효화 시점, 파일 stat 갱신 타이밍이 데이터 무결성을 좌우하므로 흐름을 깨지 않도록 유지해야 합니다.
 * - 내부 헬퍼를 변경할 때는 단건/다건 저장, 조회, 삭제, 외부 수정 감지 경로까지 함께 점검해야 회귀를 방지할 수 있습니다.
 */
#pragma once

#include <cctype>
#include <limits>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace FdFile::util {

// 숫자 문자열을 long으로 엄격하게 변환한다.
// - 문자열 전체를 소비하지 못하면 실패로 간주한다.
// - 내부 파싱은 stoll(long long)로 수행한 뒤 long 범위 검증을 추가로 수행한다.
// - 호출자는 ec를 통해 실패 원인(invalid_argument / result_out_of_range)을 분기할 수 있다.
inline bool parseLongStrict(const std::string& s, long& out, std::error_code& ec) {
    ec.clear();
    try {
        size_t pos = 0;
        long long v = std::stoll(s, &pos, 10);
        if (pos != s.size())
            throw 1;

        if (v < (long long)std::numeric_limits<long>::min() ||
            v > (long long)std::numeric_limits<long>::max()) {
            ec = std::make_error_code(std::errc::result_out_of_range);
            return false;
        }
        out = (long)v;
        return true;
    } catch (...) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }
}

inline void skipWs(const char*& p, const char* end) {
    // 파서 전반에서 공통으로 사용하는 공백 스킵 루틴.
    // 입력 포인터를 직접 전진시키기 때문에 호출 순서가 곧 파싱 상태 전이가 된다.
    while (p < end && std::isspace((unsigned char)*p))
        ++p;
}

inline bool parseIdent(const char*& p, const char* end, std::string& out) {
    // type token은 C identifier 규칙(첫 글자 alpha/_ + 이후 alnum/_)을 따른다.
    // 파일 포맷의 진입점이므로 실패 시 상위 parseLine은 즉시 invalid_argument를 반환한다.
    skipWs(p, end);
    if (p >= end || !(std::isalpha((unsigned char)*p) || *p == '_'))
        return false;
    const char* s = p++;
    while (p < end && (std::isalnum((unsigned char)*p) || *p == '_'))
        ++p;
    out.assign(s, p);
    return true;
}

inline bool parseQuotedString(const char*& p, const char* end, std::string& out) {
    // 문자열 토큰은 큰따옴표로 감싸진 형식만 허용한다.
    // 허용 escape는 \" , \\ , \n , \t 로 제한하여 포맷 단순성과 예측 가능성을 유지한다.
    skipWs(p, end);
    if (p >= end || *p != '"')
        return false;
    ++p;

    std::string s;
    while (p < end) {
        char c = *p++;
        if (c == '"') {
            out = std::move(s);
            return true;
        }
        if (c == '\\') {
            if (p >= end)
                return false;
            char e = *p++;
            if (e == '"' || e == '\\')
                s.push_back(e);
            else if (e == 'n')
                s.push_back('\n');
            else if (e == 't')
                s.push_back('\t');
            else
                return false;
        } else {
            s.push_back(c);
        }
    }
    return false;
}

inline bool parseIntToken(const char*& p, const char* end, std::string& out) {
    // 정수 토큰은 optional sign(+/-) + 최소 1개 이상의 숫자를 요구한다.
    // 숫자 유효성(범위 포함)은 상위 계층에서 parseLongStrict로 최종 검증한다.
    skipWs(p, end);
    const char* s = p;
    if (p < end && (*p == '-' || *p == '+'))
        ++p;
    const char* digits = p;
    while (p < end && std::isdigit((unsigned char)*p))
        ++p;
    if (p == digits)
        return false;
    out.assign(s, p);
    return true;
}

inline std::string escapeString(const std::string& in) {
    // 직렬화 시 파서가 혼동할 수 있는 문자(" , \ , 개행, 탭)를 escape하여
    // parseLine <-> formatLine 간 round-trip 안정성을 보장한다.
    std::string o;
    o.reserve(in.size() + 4);
    for (char c : in) {
        if (c == '"' || c == '\\') {
            o.push_back('\\');
            o.push_back(c);
        } else if (c == '\n')
            o += "\\n";
        else if (c == '\t')
            o += "\\t";
        else
            o.push_back(c);
    }
    return o;
}

// 한 줄(개행 제외) 파싱: Type { "k": "v", "id": 123 }
// - 성공 시 type / kv를 채우고 true를 반환한다.
// - 포맷 불일치가 하나라도 있으면 즉시 false + ec=invalid_argument로 종료한다.
// - 본 함수는 저장소 로딩 경로에서 반복 호출되므로 "부분 성공"보다 "빠른 실패"를 우선한다.
inline bool parseLine(const std::string& line, std::string& type,
                      std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                      std::error_code& ec) {
    ec.clear();
    kv.clear();

    const char* p = line.data();
    const char* end = p + line.size();

    if (!parseIdent(p, end, type)) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }

    skipWs(p, end);
    if (p >= end || *p != '{') {
        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }
    ++p;

    skipWs(p, end);
    if (p < end && *p == '}') {
        ++p;
        skipWs(p, end);
        if (p != end) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }
        return true;
    }

    while (true) {
        // key-value 항목을 하나씩 읽고, 구분자(',' 또는 '}')를 기준으로 다음 상태로 이동한다.
        // 이 루프는 상태 머신 역할을 하며, 어느 단계에서든 형식이 어긋나면 즉시 실패한다.
        std::string key;
        if (!parseQuotedString(p, end, key)) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        skipWs(p, end);
        if (p >= end || *p != ':') {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }
        ++p;

        skipWs(p, end);
        bool isStr = false;
        std::string val;

        if (p < end && *p == '"') {
            isStr = true;
            if (!parseQuotedString(p, end, val)) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }
        } else {
            isStr = false;
            if (!parseIntToken(p, end, val)) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }
        }

        // duplicate key는 포맷 오류로 처리한다.
        // 동일 key 중복 허용 시 마지막 값 우선/첫 값 우선 정책이 필요해지므로,
        // 구현 단순성과 예측 가능성을 위해 입력 자체를 invalid로 본다.
        if (kv.find(key) != kv.end()) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }
        kv.emplace(std::move(key), std::make_pair(isStr, std::move(val)));

        skipWs(p, end);
        if (p >= end) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        if (*p == ',') {
            ++p;
            continue;
        }
        if (*p == '}') {
            ++p;
            break;
        }

        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }

    skipWs(p, end);
    if (p != end) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }
    return true;
}

inline std::string
formatLine(const char* type,
           const std::vector<std::pair<std::string, std::pair<bool, std::string>>>& fields) {

    // parseLine이 기대하는 형식과 정확히 짝이 맞는 출력 문자열을 생성한다.
    // 줄 끝 개행('\n')은 저장소 파일에서 레코드 경계를 식별하는 핵심 규약이다.
    std::string out;
    out += type;
    out += " { ";
    for (size_t i = 0; i < fields.size(); ++i) {
        const auto& k = fields[i].first;
        const bool isStr = fields[i].second.first;
        const auto& v = fields[i].second.second;

        out += '"';
        out += escapeString(k);
        out += "\": ";
        if (isStr) {
            out += '"';
            out += escapeString(v);
            out += '"';
        } else {
            out += v;
        }

        if (i + 1 < fields.size())
            out += ", ";
    }
    out += " }\n";
    return out;
}

} // namespace FdFile::util
