cmake_minimum_required(VERSION 2.8)

project(bamrc)

set(SOURCES
    BasicStat.cpp
    BasicStat.hpp
    Options.cpp
    Options.hpp
    ReadWarnings.hpp
    auxfields.hpp
)

add_library(bamrc ${SOURCES})
target_link_libraries(bamrc ${Boost_LIBRARIES} ${Samtools_LIBRARIES} ${ZLIB_LIBRARIES})
