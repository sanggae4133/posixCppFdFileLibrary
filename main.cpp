#include <iostream>
#include "fdFileLib/FdTextFile.hpp"
#include "records/A.hpp"
#include "records/B.hpp"

int main() {
    std::error_code ec;

    FdFile::FdTextFile f(
        "./test/mixed.txt",
        {
            {"A", [](){ return std::make_unique<A>(); }},
            {"B", [](){ return std::make_unique<B>(); }},
        },
        ec
    );
    if (ec) {
        std::cerr << "open: " << ec.message() << "\n";
        return 1;
    }

    A a; a.name = "john"; a.id = 123;
    B b1; b1.name = "albert";  b1.id = 1234;  b1.pw = "1234";
    B b2; b2.name = "albert2"; b2.id = 12345; b2.pw = "1234";

    if (!f.append(a,  true, ec)) { std::cerr << "append A: "  << ec.message() << "\n"; return 1; }
    if (!f.append(b1, true, ec)) { std::cerr << "append B1: " << ec.message() << "\n"; return 1; }
    if (!f.append(b2, true, ec)) { std::cerr << "append B2: " << ec.message() << "\n"; return 1; }

    // 사람이 파일을 편집해서 albert2 -> albert3 로 바꾸면, 변경값 그대로 읽힘.
    // 마지막 줄이 개행 없이 끝나도, 레코드가 완전하면 OK.
    // 하지만 "pw": 로 끊기면 parseLine에서 에러.

    off_t off = 0;
    for (int k = 0; k < 3; ++k) {
        std::unique_ptr<FdFile::TextRecordBase> rec;
        off_t next = 0;
        if (!f.readAt(off, rec, next, ec)) {
            std::cerr << "read error: " << ec.message() << " off=" << off << "\n";
            return 1;
        }

        if (std::string(rec->typeName()) == "A") {
            auto* rr = static_cast<A*>(rec.get());
            std::cout << "A name=" << rr->name << " id=" << rr->id << "\n";
        } else if (std::string(rec->typeName()) == "B") {
            auto* rr = static_cast<B*>(rec.get());
            std::cout << "B name=" << rr->name << " id=" << rr->id << " pw=" << rr->pw << "\n";
        }

        off = next;
    }
}