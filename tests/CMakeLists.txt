# =============================================================================
# FdFileLib Unit Tests
# =============================================================================

# ==== Unit Tests ====
set(UNIT_TEST_SOURCES
    unit/UtilTest.cpp
    unit/UniqueFdTest.cpp
    unit/MmapGuardTest.cpp
    unit/FileLockGuardTest.cpp
    unit/FieldMetaTest.cpp
)

# ==== Scenario Tests ====
set(SCENARIO_TEST_SOURCES
    scenario/FixedRecordScenarioTest.cpp
    scenario/VariableRecordScenarioTest.cpp
)

# ==== Integration Tests ====
set(INTEGRATION_TEST_SOURCES
    integration/ConcurrencyTest.cpp
    integration/FilePermissionTest.cpp
    # Keep existing tests that have corruption scenarios
    FixedRecordTest.cpp
    VariableRecordTest.cpp
)

# Test executable (all tests)
add_executable(fdfile_tests
    ${UNIT_TEST_SOURCES}
    ${SCENARIO_TEST_SOURCES}
    ${INTEGRATION_TEST_SOURCES}
)

# Include paths - use new include directory structure
target_include_directories(fdfile_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/examples
)

# Link libraries
target_link_libraries(fdfile_tests PRIVATE
    fdfile
    GTest::gtest_main
)

# Compiler options
target_compile_options(fdfile_tests PRIVATE -Wall -Wextra -pedantic)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(fdfile_tests)
