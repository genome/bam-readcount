cmake_minimum_required(VERSION 2.8)

project(bam-readcount)

set(SOURCES bamreadcount.cpp)

set(EXECUTABLE_NAME bam-readcount)
xadd_executable(${EXECUTABLE_NAME} ${SOURCES})
target_link_libraries(${EXECUTABLE_NAME} bamrc ${Samtools_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES PACKAGE_OUTPUT_NAME ${EXECUTABLE_NAME}${EXE_VERSION_SUFFIX})
install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin/)
