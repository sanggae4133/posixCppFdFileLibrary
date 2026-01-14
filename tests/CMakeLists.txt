# =============================================================================
# FdFileLib Unit Tests
# =============================================================================

# Test executable
add_executable(fdfile_tests
    FixedRecordTest.cpp
    VariableRecordTest.cpp
    UtilTest.cpp
)

# Include paths
target_include_directories(fdfile_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/fdFileLib
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
