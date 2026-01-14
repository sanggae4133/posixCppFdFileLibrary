#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Records
#include "records/A.hpp"      // Variable Record
#include "records/B.hpp"      // Variable Record
#include "records/FixedA.hpp" // Fixed Record
#include "records/FixedB.hpp" // Fixed Record

// Repositories
#include "repository/UniformFixedRepositoryImpl.hpp"
#include "repository/VariableFileRepositoryImpl.hpp"

using namespace FdFile;

void testVariable() {
    std::cout << "\n=== Testing Variable Repository ===\n";
    std::error_code ec;

    std::vector<std::unique_ptr<VariableRecordBase>> protos;
    protos.push_back(std::make_unique<A>());
    protos.push_back(std::make_unique<B>());

    VariableFileRepositoryImpl repo("./test_var.txt", std::move(protos), ec);
    if (ec) {
        std::cout << "Repo init failed: " << ec.message() << "\n";
        return;
    }

    A a("alice", 1);
    repo.save(a, ec);
    std::cout << "Saved A(alice)\n";

    auto found = repo.findById("1", ec);
    if (found)
        std::cout << "Found: " << found->id() << "\n";

    std::cout << "Count: " << repo.count(ec) << "\n";
}

void testFixed() {
    std::cout << "\n=== Testing Uniform Fixed Repository ===\n";
    std::error_code ec;

    // FixedA 전용 저장소
    UniformFixedRepositoryImpl<FixedA> repo("./test_fixed.txt", ec);
    if (ec) {
        std::cout << "Repo init failed: " << ec.message() << "\n";
        return;
    }

    FixedA fa("bob", 20, 100);
    repo.save(fa, ec);
    std::cout << "Saved FixedA(bob, 100)\n";

    auto found = repo.findById("100", ec);
    if (found) {
        std::cout << "Found: name=" << found->name << " age=" << found->age << "\n";
    }

    std::cout << "Count: " << repo.count(ec) << "\n";
}

int main() {
    testVariable();
    testFixed();
    return 0;
}
