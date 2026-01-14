#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "examples/records/A.hpp"
#include "examples/records/B.hpp"
#include "fdFileLib/FdTextFile.hpp"

void printMenu() {
    std::cout << R"(
=== Repository Menu ===
1. Save A (id, name)
2. Save B (id, name, pw)
3. Find All
4. Find by ID
5. Find all type A
6. Find all type B
7. Delete by ID
8. Delete All
9. Count
10. Exists by ID
0. Quit
=======================
)";
}

void printRecord(const FdFile::TextRecordBase* rec) {
    if (std::string(rec->typeName()) == "A") {
        auto* r = static_cast<const A*>(rec);
        std::cout << "  [A] id=" << r->id() << " name=" << r->name << "\n";
    } else if (std::string(rec->typeName()) == "B") {
        auto* r = static_cast<const B*>(rec);
        std::cout << "  [B] id=" << r->id() << " name=" << r->name << " pw=" << r->pw << "\n";
    }
}

int main() {
    std::error_code ec;

    // 프로토타입 등록
    std::vector<std::unique_ptr<FdFile::TextRecordBase>> protos;
    protos.push_back(std::make_unique<A>());
    protos.push_back(std::make_unique<B>());

    // Repository 생성
    FdFile::FdTextFile repo("./test/data.txt", std::move(protos), ec);
    if (ec) {
        std::cerr << "Failed to open repository: " << ec.message() << "\n";
        return 1;
    }

    std::cout << "=== FdTextFile Repository CLI ===\n";

    int choice;
    while (true) {
        printMenu();
        std::cout << "선택: ";
        if (!(std::cin >> choice)) {
            break;
        }

        switch (choice) {
        case 0:
            std::cout << "Bye!\n";
            return 0;

        case 1: { // Save A
            long id;
            std::string name;
            std::cout << "ID: ";
            std::cin >> id;
            std::cout << "Name: ";
            std::cin >> name;
            A a(name, id);
            if (repo.save(a, ec)) {
                std::cout << "✓ Saved A: id=" << id << " name=" << name << "\n";
            } else {
                std::cout << "✗ Error: " << ec.message() << "\n";
            }
            break;
        }

        case 2: { // Save B
            long id;
            std::string name, pw;
            std::cout << "ID: ";
            std::cin >> id;
            std::cout << "Name: ";
            std::cin >> name;
            std::cout << "Password: ";
            std::cin >> pw;
            B b(name, id, pw);
            if (repo.save(b, ec)) {
                std::cout << "✓ Saved B: id=" << id << " name=" << name << "\n";
            } else {
                std::cout << "✗ Error: " << ec.message() << "\n";
            }
            break;
        }

        case 3: { // Find All
            auto all = repo.findAll(ec);
            if (ec) {
                std::cout << "✗ Error: " << ec.message() << "\n";
            } else {
                std::cout << "Found " << all.size() << " record(s):\n";
                for (const auto& rec : all) {
                    printRecord(rec.get());
                }
            }
            break;
        }

        case 4: { // Find by ID
            std::string id;
            std::cout << "ID: ";
            std::cin >> id;
            auto found = repo.findById(id, ec);
            if (ec) {
                std::cout << "✗ Error: " << ec.message() << "\n";
            } else if (found) {
                std::cout << "Found:\n";
                printRecord(found.get());
            } else {
                std::cout << "Not found\n";
            }
            break;
        }

        case 5: { // Find all type A
            auto all = repo.findAllByType<A>(ec);
            if (ec) {
                std::cout << "✗ Error: " << ec.message() << "\n";
            } else {
                std::cout << "Found " << all.size() << " A record(s):\n";
                for (const auto& r : all) {
                    printRecord(r.get());
                }
            }
            break;
        }

        case 6: { // Find all type B
            auto all = repo.findAllByType<B>(ec);
            if (ec) {
                std::cout << "✗ Error: " << ec.message() << "\n";
            } else {
                std::cout << "Found " << all.size() << " B record(s):\n";
                for (const auto& r : all) {
                    printRecord(r.get());
                }
            }
            break;
        }

        case 7: { // Delete by ID
            std::string id;
            std::cout << "ID: ";
            std::cin >> id;
            if (repo.deleteById(id, ec)) {
                std::cout << "✓ Deleted: id=" << id << "\n";
            } else {
                std::cout << "✗ Error: " << ec.message() << "\n";
            }
            break;
        }

        case 8: { // Delete All
            if (repo.deleteAll(ec)) {
                std::cout << "✓ All records deleted\n";
            } else {
                std::cout << "✗ Error: " << ec.message() << "\n";
            }
            break;
        }

        case 9: { // Count
            size_t n = repo.count(ec);
            if (ec) {
                std::cout << "✗ Error: " << ec.message() << "\n";
            } else {
                std::cout << "Count: " << n << "\n";
            }
            break;
        }

        case 10: { // Exists by ID
            std::string id;
            std::cout << "ID: ";
            std::cin >> id;
            bool exists = repo.existsById(id, ec);
            if (ec) {
                std::cout << "✗ Error: " << ec.message() << "\n";
            } else {
                std::cout << "exists(" << id << "): " << (exists ? "true" : "false") << "\n";
            }
            break;
        }

        default:
            std::cout << "잘못된 선택\n";
            break;
        }
    }

    return 0;
}
