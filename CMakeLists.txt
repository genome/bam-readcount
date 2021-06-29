cmake_minimum_required(VERSION 2.8.3)

project(bam-readcount)

if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()

set(CMAKE_MODULE_PATH 
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/build-common/cmake
)

include(CMakePrintHelpers)
include(TestHelper)
include(VersionHelper)
include(ProjectHelper)
include(PreDepends)
PreDependsInit()

#set(EXE_VERSION_SUFFIX ${FULL_VERSION})

configure_file(version/version.h.in version/version.h @ONLY)
include_directories(${PROJECT_BINARY_DIR}/version)

set(REQUIRED_BOOST_LIBS program_options regex)
include(BuildBoost)
include_directories(${Boost_INCLUDE_DIRS})

include(BuildSamtools)
include_directories(${Samtools_INCLUDE_DIRS} ${Htslib_INCLUDE_DIRS})

option(CODE_COVERAGE "Enable coverage reporting" OFF)

if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    if(CODE_COVERAGE)
        include(CodeCoverage)
        set(CMAKE_CXX_FLAGS "-Wall -std=c++0x --coverage -g -O0")
        #append_coverage_compiler_flags()
        setup_target_for_coverage_lcov(
            NAME codecoverage
            EXECUTABLE ctest -V
            BASE_DIRECTORY "./"
            NO_DEMANGLE
            EXCLUDE "build/*" "test/*"
        )
    else()
        set(CMAKE_CXX_FLAGS "-Wall -std=c++0x")
    endif()
endif()

find_package(Threads)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

# make sure to pick up headers from library dirs
include_directories("src/lib")

# main project

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_projects(src/lib)
add_projects(src/exe)

###########################################################################
## Testing
set(TEST_DATA_DIRECTORY ${CMAKE_SOURCE_DIR}/test-data)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test-bin)
enable_testing(true)
add_projects(test/lib)

# integration testing
add_subdirectory(integration-test)

###########################################################################
## Packaging
include(PackageInfo.cmake)
