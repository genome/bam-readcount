# - Try to find Samtools 
# Once done, this will define
#
#  Samtools_FOUND - system has Samtools
#  Samtools_INCLUDE_DIRS - the Samtools include directories
#  Samtools_LIBRARIES - link these to use Samtools

set(SAMTOOLS_SEARCH_DIRS
    ${SAMTOOLS_SEARCH_DIRS}
    $ENV{SAMTOOLS_ROOT}
    /gsc/pkg/bio/samtools
    /usr
)

set(_samtools_ver_path "samtools-${Samtools_FIND_VERSION}")
include(LibFindMacros)

# Dependencies
libfind_package(Samtools ZLIB)

# Include dir
find_path(Samtools_INCLUDE_DIR
    NAMES ${SAMTOOLS_ADDITIONAL_HEADERS} bam.h
    PATHS ${SAMTOOLS_SEARCH_DIRS}
    PATH_SUFFIXES 
        include include/sam include/bam include/samtools${_samtools_ver_path}
        include/samtools
    HINTS ENV SAMTOOLS_ROOT
)

# Finally the library itself
find_library(Samtools_LIBRARY
    NAMES bam libbam.a bam.a
    PATHS ${Samtools_INCLUDE_DIR} ${SAMTOOLS_SEARCH_DIRS}
    NO_DEFAULT_PATH
    PATH_SUFFIXES lib lib64 ${_samtools_ver_path}
    HINTS ENV SAMTOOLS_ROOT
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this lib depends on.
set(Samtools_PROCESS_INCLUDES Samtools_INCLUDE_DIR ZLIB_INCLUDE_DIR)
set(Samtools_PROCESS_LIBS Samtools_LIBRARY ZLIB_LIBRARIES)
libfind_process(Samtools)
message(" - Samtools include dirs: ${Samtools_INCLUDE_DIRS}")
message(" - Samtools libraries: ${Samtools_LIBRARIES}")
