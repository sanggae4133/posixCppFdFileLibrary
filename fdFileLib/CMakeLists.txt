# =============================================================================
# FdFileLib - Core Library
# =============================================================================

set(FDFILE_HEADERS
    fdfile.hpp
    record/RecordBase.hpp
    record/FieldMeta.hpp
    record/FixedRecordBase.hpp
    record/VariableRecordBase.hpp
    repository/RecordRepository.hpp
    repository/UniformFixedRepositoryImpl.hpp
    repository/VariableFileRepositoryImpl.hpp
    util/UniqueFd.hpp
    util/MmapGuard.hpp
    util/FileLockGuard.hpp
    util/textFormatUtil.hpp
)

set(FDFILE_SOURCES
    repository/VariableFileRepositoryImpl.cpp
)

# Create library target
add_library(fdfile STATIC ${FDFILE_SOURCES})

# Create namespace alias for consistent usage
add_library(fdfile::fdfile ALIAS fdfile)

# Include directories
target_include_directories(fdfile 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include/fdfile>
)

# Require C++17
target_compile_features(fdfile PUBLIC cxx_std_17)

# Set library properties
set_target_properties(fdfile PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# =============================================================================
# Filesystem Library Check
# =============================================================================
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
