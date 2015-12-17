cmake_minimum_required(VERSION 2.8)

project(bamrc)

set(SOURCES
    ReadWarnings.hpp
    auxfields.hpp
    BasicStat.hpp
    BasicStat.cpp
    IndelQueueEntry.hpp
)

xadd_library(bamrc ${SOURCES})
target_link_libraries(bamrc ${Boost_LIBRARIES} ${Samtools_LIBRARIES} ${ZLIB_LIBRARIES})
