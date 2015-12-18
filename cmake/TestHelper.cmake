# Build/configure gtest

find_package(Threads REQUIRED)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

include(ExternalProject)
set_directory_properties(PROPERTIES
    EP_PREFIX ${CMAKE_BINARY_DIR}/vendor)

set(GTEST_LIB_DIR ${CMAKE_BINARY_DIR}/vendor/gtest160-build)
ExternalProject_Add(
    gtest160
    URL ${CMAKE_CURRENT_SOURCE_DIR}/build-common/vendor/gtest-1.6.0.tar.gz
    INSTALL_COMMAND ""
    CMAKE_ARGS -DCMAKE_CXX_FLAGS=-DGTEST_USE_OWN_TR1_TUPLE=1
    BINARY_DIR ${GTEST_LIB_DIR}
    )
ExternalProject_Get_Property(gtest160 source_dir)
include_directories(${source_dir}/include)

set(GTEST_LIBRARY
    ${GTEST_LIB_DIR}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
)

set(GTEST_MAIN_LIBRARY
    ${GTEST_LIB_DIR}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}
)

add_library(gtest STATIC IMPORTED)
set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY})
add_library(gtest_main STATIC IMPORTED)
set_property(TARGET gtest_main PROPERTY IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY})

macro(add_unit_tests test_name)
    set(src_files ${ARGN})
    add_executable(${test_name} ${src_files} ${COMMON_SOURCES})
    set_target_properties(${test_name} PROPERTIES COMPILE_FLAGS -DGTEST_USE_OWN_TR1_TUPLE=1)
    target_link_libraries(${test_name} ${TEST_LIBS} gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
    add_dependencies(${test_name} gtest160)
    if($ENV{BC_UNIT_TEST_VG})
        add_test(
            NAME ${test_name}
            COMMAND valgrind --leak-check=full --error-exitcode=1 $<TARGET_FILE:${test_name}>
            )
    else()
        add_test(NAME ${test_name} COMMAND ${test_name})
    endif()

    set_tests_properties(${test_name} PROPERTIES LABELS unit)
endmacro(add_unit_tests test_name src_files)

macro(def_test testName)
    add_executable(Test${testName} Test${testName}.cpp ${COMMON_SOURCES})
    set_target_properties(${test_name} PROPERTIES COMPILE_FLAGS -DGTEST_USE_OWN_TR1_TUPLE=1)
    target_link_libraries(Test${testName} ${TEST_LIBS} gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
    add_dependencies(Test${testName} gtest160)
    if($ENV{BC_UNIT_TEST_VG})
        add_test(NAME Test${testName} COMMAND valgrind --leak-check=full --error-exitcode=1 $<TARGET_FILE:Test${testName}>)
    else()
        add_test(NAME Test${testName} COMMAND Test${testName})
    endif()

    set_tests_properties(Test${testName} PROPERTIES LABELS unit)
endmacro(def_test testName)

macro(def_integration_test exe_tgt testName script)
    set(PPATH "${BC_PYTHONPATH_EXTRA}:${CMAKE_SOURCE_DIR}/build-common/python:$ENV{PYTHONPATH}")
    add_test(
        NAME ${testName}
        COMMAND sh -ec "PYTHONPATH='${PPATH}' ${CMAKE_CURRENT_SOURCE_DIR}/${script} $<TARGET_FILE:${exe_tgt}>"
    )
    set_tests_properties(${testName} PROPERTIES LABELS integration)
endmacro(def_integration_test testName script)
