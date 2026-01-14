cmake_minimum_required(VERSION 3.16)
project(posixCppFdFileLibrary LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only library
add_library(fdfile INTERFACE)
target_include_directories(fdfile INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Some older libstdc++ versions require -lstdc++fs for <filesystem>
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES "")
check_cxx_source_compiles("
  #include <filesystem>
  int main(){ std::filesystem::path p{\".\"}; return (int)p.string().size(); }
" HAVE_FILESYSTEM_NO_LIB)
if(NOT HAVE_FILESYSTEM_NO_LIB)
  target_link_libraries(fdfile INTERFACE stdc++fs)
endif()

add_executable(fdfile_example examples/main.cpp)
target_link_libraries(fdfile_example PRIVATE fdfile)
target_compile_options(fdfile_example PRIVATE -Wall -Wextra -pedantic)
