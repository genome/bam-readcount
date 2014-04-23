# - Try to find TRE 
# Once done, this will define
#
#  TRE - system has TRE
#  TRE_INCLUDE_DIRS - the TRE include directories
#  TRE_LIBRARIES - link these to use TRE

set(TRE_SEARCH_DIRS
    ${TRE_SEARCH_DIRS}
    $ENV{TRE_ROOT}
    /usr
    /usr/local
    /opt/local
    /gsc/pkg/bio/tre
)

set(_tre_ver_path "tre-${TRE_FIND_VERSION}")
include(LibFindMacros)

# Dependencies
libfind_package(TRE ZLIB)

# Include dir
find_path(TRE_INCLUDE_DIR
    NAMES tre/tre.h 
    PATHS ${TRE_SEARCH_DIRS}
    PATH_SUFFIXES include ${_tre_ver_path}
    HINTS ENV TRE_ROOT
)

# Finally the library itself
find_library(TRE_LIBRARY
    NAMES tre libtre.a libtre.so
    PATHS lib ${TRE_INCLUDE_DIR} ${TRE_SEARCH_DIRS}
    NO_DEFAULT_PATH
    PATH_SUFFIXES lib ${_tre_ver_path}
    HINTS ENV TRE_ROOT
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this lib depends on.
set(TRE_PROCESS_INCLUDES TRE_INCLUDE_DIR)
set(TRE_PROCESS_LIBS TRE_LIBRARY)
libfind_process(TRE)
message(" - TRE include dirs: ${TRE_INCLUDE_DIRS}")
message(" - TRE libraries: ${TRE_LIBRARIES}")
