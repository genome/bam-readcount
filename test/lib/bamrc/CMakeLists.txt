cmake_minimum_required(VERSION 2.8)

set(TEST_LIBS 
    bamrc
)
include_directories(${GTEST_INCLUDE_DIRS})

add_unit_tests(TestBamrcLib
    TestAuxFields.cpp
    TestReadWarnings.cpp 
    TestIndelQueue.cpp
    TestIndelQueueEntry.cpp
    )
