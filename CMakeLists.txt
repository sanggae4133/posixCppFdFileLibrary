cmake_minimum_required(VERSION 3.16)

project(FdFileLib
    VERSION 1.0.0
    DESCRIPTION "C++17 POSIX file repository library with RAII and type safety"
    LANGUAGES CXX
)

# ============================================================================
# C++ Standard
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Build Options
# ============================================================================
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(FDFILE_BUILD_EXAMPLES "Build example programs" ON)
    option(FDFILE_BUILD_TESTS "Build unit tests" ON)
else()
    option(FDFILE_BUILD_EXAMPLES "Build example programs" OFF)
    option(FDFILE_BUILD_TESTS "Build unit tests" OFF)
endif()
option(FDFILE_RUN_TESTS "Run tests after build" OFF)
option(FDFILE_INSTALL "Generate install target" ON)

# ============================================================================
# Compiler Warnings
# ============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type
        -Wno-unused-parameter
    )
endif()

# ============================================================================
# Library Target
# ============================================================================
add_subdirectory(fdFileLib)

# ============================================================================
# Examples
# ============================================================================
if(FDFILE_BUILD_EXAMPLES)
    add_executable(fdfile_example examples/main.cpp)
    target_link_libraries(fdfile_example PRIVATE fdfile::fdfile)
endif()

# ============================================================================
# Tests
# ============================================================================
if(FDFILE_BUILD_TESTS)
    enable_testing()

    find_package(GTest QUIET)

    if(NOT GTest_FOUND)
        message(STATUS "Google Test not found locally, fetching from GitHub...")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_subdirectory(tests)

    if(FDFILE_RUN_TESTS)
        add_custom_target(run_tests ALL
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
            DEPENDS fdfile_tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running unit tests..."
        )
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================
if(FDFILE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS fdfile
        EXPORT fdfileTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fdFileLib
    )

    install(DIRECTORY fdFileLib/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fdFileLib
        FILES_MATCHING PATTERN "*.hpp"
    )

    install(EXPORT fdfileTargets
        FILE FdFileLibTargets.cmake
        NAMESPACE fdfile::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FdFileLib
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FdFileLibConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/FdFileLibConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FdFileLib
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/FdFileLibConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/FdFileLibConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/FdFileLibConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FdFileLib
    )
endif()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "")
message(STATUS "FdFileLib Configuration Summary:")
message(STATUS "  Version:         ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Examples:  ${FDFILE_BUILD_EXAMPLES}")
message(STATUS "  Build Tests:     ${FDFILE_BUILD_TESTS}")
message(STATUS "  Run Tests:       ${FDFILE_RUN_TESTS}")
message(STATUS "  Install:         ${FDFILE_INSTALL}")
message(STATUS "")
