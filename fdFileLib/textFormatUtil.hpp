#pragma once

#include <cctype>
#include <string>
#include <vector>
#include <unordered_map>
#include <system_error>
#include <limits>

namespace FdFile::util {

// strict long parse with range check
inline bool parseLongStrict(const std::string& s, long& out, std::error_code& ec) {
    ec.clear();
    try {
        size_t pos = 0;
        long long v = std::stoll(s, &pos, 10);
        if (pos != s.size()) throw 1;

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
    while (p < end && std::isspace((unsigned char)*p)) ++p;
}

inline bool parseIdent(const char*& p, const char* end, std::string& out) {
    skipWs(p, end);
    if (p >= end || !(std::isalpha((unsigned char)*p) || *p == '_')) return false;
    const char* s = p++;
    while (p < end && (std::isalnum((unsigned char)*p) || *p == '_')) ++p;
    out.assign(s, p);
    return true;
}

inline bool parseQuotedString(const char*& p, const char* end, std::string& out) {
    skipWs(p, end);
    if (p >= end || *p != '"') return false;
    ++p;

    std::string s;
    while (p < end) {
        char c = *p++;
        if (c == '"') { out = std::move(s); return true; }
        if (c == '\\') {
            if (p >= end) return false;
            char e = *p++;
            if (e == '"' || e == '\\') s.push_back(e);
            else if (e == 'n') s.push_back('\n');
            else if (e == 't') s.push_back('\t');
            else return false;
        } else {
            s.push_back(c);
        }
    }
    return false;
}

inline bool parseIntToken(const char*& p, const char* end, std::string& out) {
    skipWs(p, end);
    const char* s = p;
    if (p < end && (*p == '-' || *p == '+')) ++p;
    const char* digits = p;
    while (p < end && std::isdigit((unsigned char)*p)) ++p;
    if (p == digits) return false;
    out.assign(s, p);
    return true;
}

inline std::string escapeString(const std::string& in) {
    std::string o;
    o.reserve(in.size() + 4);
    for (char c : in) {
        if (c == '"' || c == '\\') { o.push_back('\\'); o.push_back(c); }
        else if (c == '\n') o += "\\n";
        else if (c == '\t') o += "\\t";
        else o.push_back(c);
    }
    return o;
}

// 한 줄(개행 제외) 파싱: Type { "k": "v", "id": 123 }
inline bool parseLine(const std::string& line,
                       std::string& type,
                       std::unordered_map<std::string, std::pair<bool, std::string>>& kv,
                       std::error_code& ec) {
    ec.clear();
    kv.clear();

    const char* p = line.data();
    const char* end = p + line.size();

    if (!parseIdent(p, end, type)) { ec = std::make_error_code(std::errc::invalid_argument); return false; }

    skipWs(p, end);
    if (p >= end || *p != '{') { ec = std::make_error_code(std::errc::invalid_argument); return false; }
    ++p;

    skipWs(p, end);
    if (p < end && *p == '}') {
        ++p;
        skipWs(p, end);
        if (p != end) { ec = std::make_error_code(std::errc::invalid_argument); return false; }
        return true;
    }

    while (true) {
        std::string key;
        if (!parseQuotedString(p, end, key)) { ec = std::make_error_code(std::errc::invalid_argument); return false; }

        skipWs(p, end);
        if (p >= end || *p != ':') { ec = std::make_error_code(std::errc::invalid_argument); return false; }
        ++p;

        skipWs(p, end);
        bool isStr = false;
        std::string val;

        if (p < end && *p == '"') {
            isStr = true;
            if (!parseQuotedString(p, end, val)) { ec = std::make_error_code(std::errc::invalid_argument); return false; }
        } else {
            isStr = false;
            if (!parseIntToken(p, end, val)) { ec = std::make_error_code(std::errc::invalid_argument); return false; }
        }

        kv.emplace(std::move(key), std::make_pair(isStr, std::move(val)));

        skipWs(p, end);
        if (p >= end) { ec = std::make_error_code(std::errc::invalid_argument); return false; }

        if (*p == ',') { ++p; continue; }
        if (*p == '}') { ++p; break; }

        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }

    skipWs(p, end);
    if (p != end) { ec = std::make_error_code(std::errc::invalid_argument); return false; }
    return true;
}

inline std::string formatLine(
    const char* type,
    const std::vector<std::pair<std::string, std::pair<bool, std::string>>>& fields) {

    std::string out;
    out += type;
    out += " { ";
    for (size_t i = 0; i < fields.size(); ++i) {
        const auto& k = fields[i].first;
        const bool isStr = fields[i].second.first;
        const auto& v = fields[i].second.second;

        out += '"'; out += escapeString(k); out += "\": ";
        if (isStr) { out += '"'; out += escapeString(v); out += '"'; }
        else { out += v; }

        if (i + 1 < fields.size()) out += ", ";
    }
    out += " }";
    return out;
}

} // namespace FdFile::util