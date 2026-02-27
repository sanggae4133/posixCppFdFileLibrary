set(FDFILE_SOURCES
    repository/VariableFileRepositoryImpl.cpp
)

set(FDFILE_HEADERS
    record/FieldMeta.hpp
    record/FixedRecordBase.hpp
    record/RecordBase.hpp
    record/VariableRecordBase.hpp
    repository/RecordRepository.hpp
    repository/UniformFixedRepositoryImpl.hpp
    repository/VariableFileRepositoryImpl.hpp
    util/FileLockGuard.hpp
    util/MmapGuard.hpp
    util/UniqueFd.hpp
    util/textFormatUtil.hpp
)

add_library(fdfile ${FDFILE_SOURCES} ${FDFILE_HEADERS})
add_library(fdfile::fdfile ALIAS fdfile)

target_include_directories(fdfile
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/fdFileLib>
)

target_compile_features(fdfile PUBLIC cxx_std_17)

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

set_target_properties(fdfile PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)
