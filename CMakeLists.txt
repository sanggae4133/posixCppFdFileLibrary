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
