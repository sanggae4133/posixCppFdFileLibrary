cmake_minimum_required(VERSION 3.16)

project(FdFileLib
    VERSION 1.0.0
    DESCRIPTION "High-performance C++17 file repository library using POSIX APIs"
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
    # Standalone build: enable examples and tests by default
    option(FDFILE_BUILD_EXAMPLES "Build example programs" ON)
    option(FDFILE_BUILD_TESTS "Build unit tests" ON)
else()
    # Subdirectory build: disable examples and tests by default
    option(FDFILE_BUILD_EXAMPLES "Build example programs" OFF)
    option(FDFILE_BUILD_TESTS "Build unit tests" OFF)
endif()
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
    )
endif()

# ============================================================================
# Library Target
# ============================================================================
set(FDFILE_SOURCES
    src/VariableFileRepositoryImpl.cpp
)

set(FDFILE_HEADERS
    include/fdfile/fdfile.hpp
    include/fdfile/record/RecordBase.hpp
    include/fdfile/record/FieldMeta.hpp
    include/fdfile/record/FixedRecordBase.hpp
    include/fdfile/record/VariableRecordBase.hpp
    include/fdfile/repository/RecordRepository.hpp
    include/fdfile/repository/UniformFixedRepositoryImpl.hpp
    include/fdfile/repository/VariableFileRepositoryImpl.hpp
    include/fdfile/util/UniqueFd.hpp
    include/fdfile/util/MmapGuard.hpp
    include/fdfile/util/FileLockGuard.hpp
    include/fdfile/util/textFormatUtil.hpp
)

# Create library target
add_library(fdfile STATIC ${FDFILE_SOURCES})

# Create namespace alias for consistent usage
add_library(fdfile::fdfile ALIAS fdfile)

# Include directories
target_include_directories(fdfile 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Require C++17
target_compile_features(fdfile PUBLIC cxx_std_17)

# Set library properties
set_target_properties(fdfile PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Filesystem Library Check
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES "")
check_cxx_source_compiles("
    #include <filesystem>
    int main() { 
        std::filesystem::path p{\".\"}; 
        return (int)p.string().size(); 
    }
" HAVE_FILESYSTEM_NO_LIB)

if(NOT HAVE_FILESYSTEM_NO_LIB)
    target_link_libraries(fdfile PUBLIC stdc++fs)
endif()

# ============================================================================
# Examples
# ============================================================================
if(FDFILE_BUILD_EXAMPLES)
    add_executable(fdfile_example examples/main.cpp)
    target_link_libraries(fdfile_example PRIVATE fdfile)
endif()

# ============================================================================
# Testing with GoogleTest
# ============================================================================
if(FDFILE_BUILD_TESTS)
    # Try to find system-installed Google Test first
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
        # Prevent GoogleTest from being installed with this project
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    # Enable testing
    enable_testing()

    # Add tests subdirectory
    add_subdirectory(tests)
endif()

# ============================================================================
# Installation
# ============================================================================
if(FDFILE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    
    # Install library
    install(TARGETS fdfile
        EXPORT fdfileTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    # Install headers
    install(DIRECTORY include/fdfile
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    # Install CMake config files
    install(EXPORT fdfileTargets
        FILE fdfileTargets.cmake
        NAMESPACE fdfile::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fdfile
    )
    
    # Generate and install package config
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fdfileConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/fdfileConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fdfile
    )
    
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/fdfileConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/fdfileConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/fdfileConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fdfile
    )
endif()

# ============================================================================
# Configuration Summary
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "FdFileLib Configuration Summary")
message(STATUS "========================================")
message(STATUS "  Version:         ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Examples:  ${FDFILE_BUILD_EXAMPLES}")
message(STATUS "  Build Tests:     ${FDFILE_BUILD_TESTS}")
message(STATUS "  Install:         ${FDFILE_INSTALL}")
message(STATUS "")
message(STATUS "Usage in other projects:")
message(STATUS "  # FetchContent")
message(STATUS "  FetchContent_Declare(fdfilelib GIT_REPOSITORY <url> GIT_TAG v${PROJECT_VERSION})")
message(STATUS "  FetchContent_MakeAvailable(fdfilelib)")
message(STATUS "  target_link_libraries(your_app PRIVATE fdfile::fdfile)")
message(STATUS "")
message(STATUS "  # After install")
message(STATUS "  find_package(fdfile REQUIRED)")
message(STATUS "  target_link_libraries(your_app PRIVATE fdfile::fdfile)")
message(STATUS "========================================")
message(STATUS "")
