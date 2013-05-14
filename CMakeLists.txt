cmake_minimum_required(VERSION 2.8)

project(bam-readcount)

set(CMAKE_INSTALL_PREFIX "/usr")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build-common/cmake")
include(TestHelper)
include(VersionHelper)
include(ProjectHelper)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "-Wall")
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-Wall")
endif ()

find_package(Samtools 0.1.17 REQUIRED)
include_directories(${Samtools_INCLUDE_DIRS})

find_package(Threads)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

# make sure to pick up headers from library dirs
include_directories("src/lib")

# unit tests
find_package(GTest)
if(GTEST_FOUND)
    message("Google Test framework found, building unit tests")
    enable_testing(true)
    add_projects(test/lib)
else()
    message("Google Test framework not found, no tests will be built")
    message("GTest is available at http://code.google.com/p/googletest/")
    message("Ubuntu users can likely sudo apt-get install gtest")
endif()

# main project

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_projects(src/lib)
add_projects(src/exe)

include(PackageInfo.cmake)
