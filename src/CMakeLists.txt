cmake_minimum_required(VERSION 2.8)

project(bamrc)

set(SOURCES
    auxfields.hpp
    BamEntry.hpp
    BamFilter.hpp
    BamReader.cpp
    BamReader.hpp
    BasicStat.cpp
    BasicStat.hpp
    FileRegionSequence.cpp
    FileRegionSequence.hpp
    IRegionSequence.hpp
    Options.cpp
    Options.hpp
    ReadProcessor.cpp
    ReadProcessor.hpp
    ReadWarnings.hpp
    StaticRegionSequence.cpp
    StaticRegionSequence.hpp
)

add_library(bamrc ${SOURCES})
target_link_libraries(bamrc ${Boost_LIBRARIES} ${Samtools_LIBRARIES} ${ZLIB_LIBRARIES})


set(MAIN_SOURCES bamreadcount.cpp)

set(EXECUTABLE_NAME bam-readcount)
add_executable(${EXECUTABLE_NAME} ${MAIN_SOURCES})
target_link_libraries(${EXECUTABLE_NAME} bamrc ${Samtools_LIBRARIES} ${Boost_LIBRARIES})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES PACKAGE_OUTPUT_NAME ${EXECUTABLE_NAME}${EXE_VERSION_SUFFIX})
install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin/)
