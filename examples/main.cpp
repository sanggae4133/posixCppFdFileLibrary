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

// =============================================================================
// Helper Macros for Test Output
// =============================================================================

#define TEST_SECTION(name) std::cout << "\n=== " << name << " ===\n"
#define TEST_CASE(name) std::cout << "\n--- " << name << " ---\n"
#define TEST_PASS(msg) std::cout << "[PASS] " << msg << "\n"
#define TEST_FAIL(msg) std::cout << "[FAIL] " << msg << "\n"
#define CHECK_EC(ec, context)                                                                      \
    if (ec) {                                                                                      \
        TEST_FAIL(context << ": " << ec.message());                                                \
        return;                                                                                    \
    }

// =============================================================================
// Variable Record Tests
// =============================================================================

void testVariable() {
    TEST_SECTION("Variable Repository Tests");
    std::error_code ec;

    // Clean start - delete existing file
    ::remove("./test_var.txt");

    std::vector<std::unique_ptr<VariableRecordBase>> protos;
    protos.push_back(std::make_unique<A>());
    protos.push_back(std::make_unique<B>());

    VariableFileRepositoryImpl repo("./test_var.txt", std::move(protos), ec);
    CHECK_EC(ec, "Repo init");

    // =========================================================================
    // 1. Insert Multiple Records (Mixed Types)
    // =========================================================================
    TEST_CASE("Insert Multiple Records (Mixed Types)");

    A alice("alice", 1);
    A bob("bob", 2);
    A charlie("charlie", 3);
    B user1("user1", 101, "password123");
    B user2("user2", 102, "secret456");

    repo.save(alice, ec);
    CHECK_EC(ec, "Save alice");
    TEST_PASS("Saved A(alice, id=1)");

    repo.save(bob, ec);
    CHECK_EC(ec, "Save bob");
    TEST_PASS("Saved A(bob, id=2)");

    repo.save(charlie, ec);
    CHECK_EC(ec, "Save charlie");
    TEST_PASS("Saved A(charlie, id=3)");

    repo.save(user1, ec);
    CHECK_EC(ec, "Save user1");
    TEST_PASS("Saved B(user1, id=101)");

    repo.save(user2, ec);
    CHECK_EC(ec, "Save user2");
    TEST_PASS("Saved B(user2, id=102)");

    size_t cnt = repo.count(ec);
    CHECK_EC(ec, "Count");
    std::cout << "Count after inserts: " << cnt << " (expected: 5)\n";
    if (cnt == 5) {
        TEST_PASS("Count is correct");
    } else {
        TEST_FAIL("Count mismatch, got " + std::to_string(cnt));
    }

    // =========================================================================
    // 2. Update Existing Record
    // =========================================================================
    TEST_CASE("Update Existing Record");

    A updatedAlice("alice_updated", 1); // Same ID, new name
    repo.save(updatedAlice, ec);
    CHECK_EC(ec, "Update alice");
    TEST_PASS("Updated alice's name to 'alice_updated'");

    // Verify update
    auto foundAlice = repo.findById("1", ec);
    CHECK_EC(ec, "FindById alice after update");
    if (foundAlice) {
        // Access the record as A type
        auto* aPtr = dynamic_cast<A*>(foundAlice.get());
        if (aPtr && aPtr->name == "alice_updated") {
            TEST_PASS("Verified alice's name is now 'alice_updated'");
        } else {
            TEST_FAIL("Name not updated correctly");
        }
    } else {
        TEST_FAIL("Alice not found after update");
    }

    // Count should still be 5 (update, not insert)
    cnt = repo.count(ec);
    if (cnt == 5) {
        TEST_PASS("Count unchanged after update (still 5)");
    } else {
        TEST_FAIL("Count changed unexpectedly: " + std::to_string(cnt));
    }

    // =========================================================================
    // 3. FindById
    // =========================================================================
    TEST_CASE("FindById");

    auto foundBob = repo.findById("2", ec);
    CHECK_EC(ec, "FindById bob");
    if (foundBob) {
        std::cout << "Found: type=" << foundBob->typeName() << ", id=" << foundBob->id() << "\n";
        TEST_PASS("FindById succeeded");
    } else {
        TEST_FAIL("Bob not found");
    }

    // Find B type record
    auto foundUser1 = repo.findById("101", ec);
    CHECK_EC(ec, "FindById user1");
    if (foundUser1) {
        auto* bPtr = dynamic_cast<B*>(foundUser1.get());
        if (bPtr) {
            std::cout << "Found B: name=" << bPtr->name << ", pw=" << bPtr->pw << "\n";
            TEST_PASS("FindById B type succeeded");
        }
    } else {
        TEST_FAIL("User1 not found");
    }

    // Non-existent ID
    auto notFound = repo.findById("999", ec);
    CHECK_EC(ec, "FindById non-existent");
    if (!notFound) {
        TEST_PASS("Non-existent ID correctly returns nullptr");
    } else {
        TEST_FAIL("Should not find id=999");
    }

    // =========================================================================
    // 4. FindAll
    // =========================================================================
    TEST_CASE("FindAll");

    auto all = repo.findAll(ec);
    CHECK_EC(ec, "FindAll");
    std::cout << "FindAll returned " << all.size() << " records:\n";
    for (const auto& rec : all) {
        std::cout << "  - type=" << rec->typeName() << ", id=" << rec->id() << "\n";
    }
    if (all.size() == 5) {
        TEST_PASS("FindAll returned correct count");
    } else {
        TEST_FAIL("FindAll count mismatch");
    }

    // =========================================================================
    // 5. ExistsById
    // =========================================================================
    TEST_CASE("ExistsById");

    bool exists = repo.existsById("1", ec);
    CHECK_EC(ec, "ExistsById 1");
    if (exists) {
        TEST_PASS("existsById(1) = true");
    } else {
        TEST_FAIL("existsById(1) should be true");
    }

    exists = repo.existsById("999", ec);
    CHECK_EC(ec, "ExistsById 999");
    if (!exists) {
        TEST_PASS("existsById(999) = false");
    } else {
        TEST_FAIL("existsById(999) should be false");
    }

    // =========================================================================
    // 6. DeleteById
    // =========================================================================
    TEST_CASE("DeleteById");

    repo.deleteById("2", ec); // Delete bob
    CHECK_EC(ec, "DeleteById bob");
    TEST_PASS("Deleted bob (id=2)");

    cnt = repo.count(ec);
    std::cout << "Count after delete: " << cnt << " (expected: 4)\n";
    if (cnt == 4) {
        TEST_PASS("Count decreased to 4");
    } else {
        TEST_FAIL("Count should be 4, got " + std::to_string(cnt));
    }

    // Verify bob is gone
    auto deletedBob = repo.findById("2", ec);
    if (!deletedBob) {
        TEST_PASS("Bob no longer exists");
    } else {
        TEST_FAIL("Bob should have been deleted");
    }

    // =========================================================================
    // 7. DeleteAll
    // =========================================================================
    TEST_CASE("DeleteAll");

    repo.deleteAll(ec);
    CHECK_EC(ec, "DeleteAll");
    TEST_PASS("DeleteAll called");

    cnt = repo.count(ec);
    std::cout << "Count after deleteAll: " << cnt << " (expected: 0)\n";
    if (cnt == 0) {
        TEST_PASS("All records deleted");
    } else {
        TEST_FAIL("Count should be 0, got " + std::to_string(cnt));
    }

    // =========================================================================
    // 8. Re-insert after DeleteAll
    // =========================================================================
    TEST_CASE("Re-insert After DeleteAll");

    A newRecord("new_user", 999);
    repo.save(newRecord, ec);
    CHECK_EC(ec, "Save new_user");

    cnt = repo.count(ec);
    if (cnt == 1) {
        TEST_PASS("Can insert after deleteAll, count=1");
    } else {
        TEST_FAIL("Insert after deleteAll failed");
    }

    std::cout << "\nFinal count: " << repo.count(ec) << "\n";
}

// =============================================================================
// Fixed Record Tests (FixedA)
// =============================================================================

void testFixedA() {
    TEST_SECTION("Fixed Repository Tests (FixedA)");
    std::error_code ec;

    // Clean start - delete existing file
    ::remove("./test_fixed_a.txt");

    UniformFixedRepositoryImpl<FixedA> repo("./test_fixed_a.txt", ec);
    CHECK_EC(ec, "Repo init");

    // =========================================================================
    // 1. Insert Multiple Records
    // =========================================================================
    TEST_CASE("Insert Multiple Records");

    FixedA alice("alice", 25, "001");
    FixedA bob("bob", 30, "002");
    FixedA charlie("charlie", 35, "003");

    repo.save(alice, ec);
    CHECK_EC(ec, "Save alice");
    TEST_PASS("Saved alice (id=001, age=25)");

    repo.save(bob, ec);
    CHECK_EC(ec, "Save bob");
    TEST_PASS("Saved bob (id=002, age=30)");

    repo.save(charlie, ec);
    CHECK_EC(ec, "Save charlie");
    TEST_PASS("Saved charlie (id=003, age=35)");

    size_t cnt = repo.count(ec);
    CHECK_EC(ec, "Count");
    std::cout << "Count after inserts: " << cnt << " (expected: 3)\n";
    if (cnt == 3) {
        TEST_PASS("Count is correct");
    } else {
        TEST_FAIL("Count mismatch");
    }

    // =========================================================================
    // 2. Update Existing Record (Upsert)
    // =========================================================================
    TEST_CASE("Update Existing Record");

    bob.age = 31; // Update age
    repo.save(bob, ec);
    CHECK_EC(ec, "Update bob");
    TEST_PASS("Updated bob's age to 31");

    // Verify update
    auto foundBob = repo.findById("002", ec);
    CHECK_EC(ec, "FindById bob");
    if (foundBob && foundBob->age == 31) {
        TEST_PASS("Verified bob's age is now 31");
    } else {
        TEST_FAIL("Update verification failed");
    }

    // Count should still be 3 (update, not insert)
    cnt = repo.count(ec);
    if (cnt == 3) {
        TEST_PASS("Count unchanged after update (still 3)");
    } else {
        TEST_FAIL("Count changed unexpectedly: " + std::to_string(cnt));
    }

    // =========================================================================
    // 3. FindById
    // =========================================================================
    TEST_CASE("FindById");

    auto foundAlice = repo.findById("001", ec);
    CHECK_EC(ec, "FindById alice");
    if (foundAlice) {
        std::cout << "Found: name=" << foundAlice->name << ", age=" << foundAlice->age
                  << ", id=" << foundAlice->getId() << "\n";
        TEST_PASS("FindById succeeded");
    } else {
        TEST_FAIL("Alice not found");
    }

    // Non-existent ID
    auto notFound = repo.findById("999", ec);
    CHECK_EC(ec, "FindById non-existent");
    if (!notFound) {
        TEST_PASS("Non-existent ID correctly returns nullptr");
    } else {
        TEST_FAIL("Should not find id=999");
    }

    // =========================================================================
    // 4. FindAll
    // =========================================================================
    TEST_CASE("FindAll");

    auto all = repo.findAll(ec);
    CHECK_EC(ec, "FindAll");
    std::cout << "FindAll returned " << all.size() << " records:\n";
    for (const auto& rec : all) {
        std::cout << "  - name=" << rec->name << ", age=" << rec->age << ", id=" << rec->getId()
                  << "\n";
    }
    if (all.size() == 3) {
        TEST_PASS("FindAll returned correct count");
    } else {
        TEST_FAIL("FindAll count mismatch");
    }

    // =========================================================================
    // 5. ExistsById
    // =========================================================================
    TEST_CASE("ExistsById");

    bool exists = repo.existsById("001", ec);
    CHECK_EC(ec, "ExistsById 001");
    if (exists) {
        TEST_PASS("existsById(001) = true");
    } else {
        TEST_FAIL("existsById(001) should be true");
    }

    exists = repo.existsById("999", ec);
    CHECK_EC(ec, "ExistsById 999");
    if (!exists) {
        TEST_PASS("existsById(999) = false");
    } else {
        TEST_FAIL("existsById(999) should be false");
    }

    // =========================================================================
    // 6. DeleteById
    // =========================================================================
    TEST_CASE("DeleteById");

    repo.deleteById("002", ec); // Delete bob
    CHECK_EC(ec, "DeleteById bob");
    TEST_PASS("Deleted bob (id=002)");

    cnt = repo.count(ec);
    std::cout << "Count after delete: " << cnt << " (expected: 2)\n";
    if (cnt == 2) {
        TEST_PASS("Count decreased to 2");
    } else {
        TEST_FAIL("Count should be 2");
    }

    // Verify bob is gone
    auto deletedBob = repo.findById("002", ec);
    if (!deletedBob) {
        TEST_PASS("Bob no longer exists");
    } else {
        TEST_FAIL("Bob should have been deleted");
    }

    // =========================================================================
    // 7. DeleteAll
    // =========================================================================
    TEST_CASE("DeleteAll");

    repo.deleteAll(ec);
    CHECK_EC(ec, "DeleteAll");
    TEST_PASS("DeleteAll called");

    cnt = repo.count(ec);
    std::cout << "Count after deleteAll: " << cnt << " (expected: 0)\n";
    if (cnt == 0) {
        TEST_PASS("All records deleted");
    } else {
        TEST_FAIL("Count should be 0");
    }

    // =========================================================================
    // 8. Edge Cases
    // =========================================================================
    TEST_CASE("Edge Cases");

    // Empty name
    FixedA emptyName("", 0, "E01");
    repo.save(emptyName, ec);
    CHECK_EC(ec, "Save empty name");
    TEST_PASS("Saved record with empty name");

    // Large number
    FixedA largeAge("max", 9223372036854775807LL, "E02"); // INT64_MAX
    repo.save(largeAge, ec);
    CHECK_EC(ec, "Save large age");
    TEST_PASS("Saved record with INT64_MAX age");

    auto foundLarge = repo.findById("E02", ec);
    if (foundLarge && foundLarge->age == 9223372036854775807LL) {
        TEST_PASS("Large number preserved correctly");
    } else {
        TEST_FAIL("Large number not preserved");
    }

    std::cout << "\nFinal count: " << repo.count(ec) << "\n";
}

// =============================================================================
// Fixed Record Tests (FixedB)
// =============================================================================

void testFixedB() {
    TEST_SECTION("Fixed Repository Tests (FixedB)");
    std::error_code ec;

    // Clean start
    ::remove("./test_fixed_b.txt");

    UniformFixedRepositoryImpl<FixedB> repo("./test_fixed_b.txt", ec);
    CHECK_EC(ec, "Repo init");

    // Insert
    TEST_CASE("Insert FixedB Records");

    FixedB item1("Laptop", 1500000, "P001");
    FixedB item2("Phone", 800000, "P002");
    FixedB item3("Tablet", 500000, "P003");

    repo.save(item1, ec);
    CHECK_EC(ec, "Save item1");
    repo.save(item2, ec);
    CHECK_EC(ec, "Save item2");
    repo.save(item3, ec);
    CHECK_EC(ec, "Save item3");

    TEST_PASS("Saved 3 FixedB records");

    // FindAll
    TEST_CASE("FindAll FixedB");
    auto all = repo.findAll(ec);
    CHECK_EC(ec, "FindAll");
    std::cout << "FixedB records:\n";
    for (const auto& rec : all) {
        std::cout << "  - title=" << rec->title << ", cost=" << rec->cost << ", id=" << rec->getId()
                  << "\n";
    }

    if (all.size() == 3) {
        TEST_PASS("FindAll returned 3 records");
    } else {
        TEST_FAIL("FindAll count mismatch");
    }

    // Update
    TEST_CASE("Update FixedB");
    item2.cost = 750000; // Discount!
    repo.save(item2, ec);
    CHECK_EC(ec, "Update item2");

    auto updated = repo.findById("P002", ec);
    if (updated && updated->cost == 750000) {
        TEST_PASS("Phone price updated to 750000");
    } else {
        TEST_FAIL("Update failed");
    }

    std::cout << "\nFinal count: " << repo.count(ec) << "\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "    FdFileLib Comprehensive Tests\n";
    std::cout << "========================================\n";

    testVariable();
    testFixedA();
    testFixedB();

    std::cout << "\n========================================\n";
    std::cout << "    All Tests Completed\n";
    std::cout << "========================================\n";

    return 0;
}
