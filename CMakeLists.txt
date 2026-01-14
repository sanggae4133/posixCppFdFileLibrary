cmake_minimum_required(VERSION 3.16)
project(posixCppFdFileLibrary LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add subdirectory for the library
add_subdirectory(fdFileLib)

# Example executable
add_executable(fdfile_example examples/main.cpp)
target_link_libraries(fdfile_example PRIVATE fdfile)
target_compile_options(fdfile_example PRIVATE -Wall -Wextra -pedantic)

# =============================================================================
# Testing with GoogleTest
# =============================================================================
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Enable testing
    enable_testing()

    # Add tests subdirectory
    add_subdirectory(tests)
endif()
