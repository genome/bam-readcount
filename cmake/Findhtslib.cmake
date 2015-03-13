# - Try to find htslib 
# Once done, this will define
#
#  htslib_FOUND - system has htslib
#  htslib_INCLUDE_DIRS - the htslib include directories
#  htslib_LIBRARIES - link these to use htslib

set(HTSLIB_SEARCH_DIRS
    ${HTSLIB_SEARCH_DIRS}
    $ENV{HTLSIB_ROOT}
    /gsc/pkg/bio/htslib
    /usr
)

set(_htslib_ver_path "htslib-${htslib_FIND_VERSION}")
include(LibFindMacros)

# Dependencies
libfind_package(htslib ZLIB)

# Include dir
find_path(htslib_INCLUDE_DIR
    NAMES ${HTSLIB_ADDITIONAL_HEADERS} sam.h
    PATHS ${HTSLIB_SEARCH_DIRS}
    PATH_SUFFIXES 
        include include/htslib htslib/${_htslib_ver_path}/htslib
    HINTS ENV HTSLIB_ROOT
)

# Finally the library itself
find_library(htslib_LIBRARY
    NAMES hts libhts.a hts.a
    PATHS ${htslib_INCLUDE_DIR} ${HTSLIB_SEARCH_DIRS}
    NO_DEFAULT_PATH
    PATH_SUFFIXES lib lib64 ${_htslib_ver_path}
    HINTS ENV HTSLIB_ROOT
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this lib depends on.
set(htslib_PROCESS_INCLUDES htslib_INCLUDE_DIR ZLIB_INCLUDE_DIR)
set(htslib_PROCESS_LIBS htslib_LIBRARY ZLIB_LIBRARIES)
libfind_process(htslib)
message(" - HTSlib include dirs: ${htslib_INCLUDE_DIRS}")
message(" - HTSlib libraries: ${htslib_LIBRARIES}")
