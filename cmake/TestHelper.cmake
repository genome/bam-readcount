macro(def_test testName)
    add_executable(Test${testName} Test${testName}.cpp ${COMMON_SOURCES})
    target_link_libraries(Test${testName} ${TEST_LIBS} ${GTEST_BOTH_LIBRARIES})
    if($ENV{BC_UNIT_TEST_VG})
        add_test(NAME Test${testName} COMMAND valgrind --error-exitcode=1 $<TARGET_FILE:Test${testName}>)
    else()
        add_test(NAME Test${testName} COMMAND Test${testName})
    endif()

    set_tests_properties(Test${testName} PROPERTIES LABELS unit)
endmacro(def_test testName)

macro(def_integration_test exe_tgt testName script)
    add_test(
        NAME ${testName}
        COMMAND sh -ec "PYTHONPATH='${CMAKE_SOURCE_DIR}/build-common/python:$ENV{PYTHONPATH}' ${CMAKE_CURRENT_SOURCE_DIR}/${script} $<TARGET_FILE:${exe_tgt}>"
    )
    set_tests_properties(${testName} PROPERTIES LABELS integration)
endmacro(def_integration_test testName script)
