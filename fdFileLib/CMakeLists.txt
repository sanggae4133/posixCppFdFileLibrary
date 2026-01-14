# fdFileLib library

add_library(fdfile STATIC
    repository/VariableFileRepositoryImpl.cpp
)

target_include_directories(fdfile PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Filesystem check
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES "")
check_cxx_source_compiles("
  #include <filesystem>
  int main(){ std::filesystem::path p{\".\"}; return (int)p.string().size(); }
" HAVE_FILESYSTEM_NO_LIB)
if(NOT HAVE_FILESYSTEM_NO_LIB)
  target_link_libraries(fdfile PUBLIC stdc++fs)
endif()
