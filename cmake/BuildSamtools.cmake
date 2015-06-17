cmake_minimum_required(VERSION 2.8)

set(SAMTOOLS_ROOT ${CMAKE_BINARY_DIR}/vendor/samtools)
set(SAMTOOLS_LOG ${SAMTOOLS_ROOT}/build.log)
set(SAMTOOLS_LIB ${SAMTOOLS_ROOT}/${CMAKE_FIND_LIBRARY_PREFIXES}bam${CMAKE_STATIC_LIBRARY_SUFFIX})
set(SAMTOOLS_BIN ${SAMTOOLS_ROOT}/samtools)

find_package(ZLIB)
if (NOT ZLIB_FOUND)
    set(ZLIB_ROOT ${CMAKE_BINARY_DIR}/vendor/zlib)
    set(ZLIB_SRC ${CMAKE_BINARY_DIR}/vendor/zlib-src)
    set(ZLIB_INCLUDE_DIRS ${ZLIB_ROOT}/include)
    set(ZLIB_LIBRARIES ${ZLIB_ROOT}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}z${CMAKE_STATIC_LIBRARY_SUFFIX})
    ExternalDependency_Add(
        zlib
        BUILD_BYPRODUCTS ${ZLIB_LIBRARIES}
        ARGS
            URL ${CMAKE_SOURCE_DIR}/vendor/zlib-1.2.8.tar.gz
            SOURCE_DIR ${ZLIB_SRC}
            BINARY_DIR ${ZLIB_SRC}
            CONFIGURE_COMMAND ./configure --prefix=${ZLIB_ROOT}
            BUILD_COMMAND make
            INSTALL_COMMAND make install
    )
endif (NOT ZLIB_FOUND)

ExternalDependency_Add(
    samtools-lib
    BUILD_BYPRODUCTS ${SAMTOOLS_LIB} ${SAMTOOLS_BIN}
    ARGS
        URL ${CMAKE_SOURCE_DIR}/vendor/samtools-0.1.19.tar.gz
        SOURCE_DIR ${SAMTOOLS_ROOT}
        BINARY_DIR ${SAMTOOLS_ROOT}
        PATCH_COMMAND patch -p2 -t -N < ${CMAKE_SOURCE_DIR}/vendor/samtools0.1.19.patch
        CONFIGURE_COMMAND echo "Building samtools, build log at ${SAMTOOLS_LOG}"
        BUILD_COMMAND make INCLUDES=-I${ZLIB_INCLUDE_DIRS} libbam.a > ${SAMTOOLS_LOG} 2>&1
        INSTALL_COMMAND "true"
)

set(Samtools_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS};${SAMTOOLS_ROOT})
set(Samtools_LIBRARIES ${SAMTOOLS_LIB} m ${ZLIB_LIBRARIES})

if (NOT ZLIB_FOUND)
    add_dependencies(samtools-lib zlib)
endif (NOT ZLIB_FOUND)
