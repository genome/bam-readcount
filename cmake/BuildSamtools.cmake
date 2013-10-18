cmake_minimum_required(VERSION 2.8)

include(ExternalProject)

set(SAMTOOLS_ROOT ${CMAKE_BINARY_DIR}/vendor/samtools)
set(SAMTOOLS_LOG ${SAMTOOLS_ROOT}/build.log)
set(SAMTOOLS_LIB ${SAMTOOLS_ROOT}/${CMAKE_FIND_LIBRARY_PREFIXES}bam${CMAKE_STATIC_LIBRARY_SUFFIX})
set(SAMTOOLS_BIN ${SAMTOOLS_ROOT}/samtools)

find_package(ZLIB)
if (NOT ZLIB_FOUND)
    set(ZLIB_ROOT ${CMAKE_BINARY_DIR}/vendor/zlib)
    set(ZLIB_SRC ${CMAKE_BINARY_DIR}/vendor/zlib-src)
    ExternalProject_Add(
        zlib
        URL ${CMAKE_SOURCE_DIR}/vendor/zlib-1.2.8.tar.gz
        SOURCE_DIR ${ZLIB_SRC}
        BINARY_DIR ${ZLIB_SRC}
        CONFIGURE_COMMAND ./configure --prefix=${ZLIB_ROOT}
        BUILD_COMMAND make
        INSTALL_COMMAND make install
    )
    set(ZLIB_INCLUDE_DIRS ${ZLIB_ROOT}/include)
    set(ZLIB_LIBRARIES ${ZLIB_ROOT}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}z${CMAKE_STATIC_LIBRARY_SUFFIX})
    add_library(z STATIC IMPORTED)
    set_property(TARGET z PROPERTY IMPORTED_LOCATION ${ZLIB_LIBRARIES})
endif (NOT ZLIB_FOUND)

ExternalProject_Add(
    samtools-lib
    URL ${CMAKE_SOURCE_DIR}/vendor/samtools-0.1.19.tar.gz
    SOURCE_DIR ${SAMTOOLS_ROOT}
    BINARY_DIR ${SAMTOOLS_ROOT}
    CONFIGURE_COMMAND echo "Building samtools, build log at ${SAMTOOLS_LOG}"
    BUILD_COMMAND make INCLUDES=-I${ZLIB_INCLUDE_DIRS} libbam.a > ${SAMTOOLS_LOG} 2>&1
    INSTALL_COMMAND ""
)

add_library(bam STATIC IMPORTED)
set_property(TARGET bam PROPERTY IMPORTED_LOCATION ${SAMTOOLS_LIB})

set(Samtools_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS};${SAMTOOLS_ROOT})
set(Samtools_LIBRARIES bam m ${ZLIB_LIBRARIES})

if (NOT ZLIB_FOUND)
    add_dependencies(samtools-lib zlib)
endif (NOT ZLIB_FOUND)
