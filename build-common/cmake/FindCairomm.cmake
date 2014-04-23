# - Try to find Cairomm 1.0
# Once done, this will define
#
#  Cairomm_FOUND - system has Cairomm
#  Cairomm_INCLUDE_DIRS - the Cairomm include directories
#  Cairomm_LIBRARIES - link these to use Cairomm

include(LibFindMacros)

# Dependencies
libfind_package(Cairomm Cairo)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Cairomm_PKGCONF cairomm-1.0)

# Main include dir
find_path(Cairomm_INCLUDE_DIR
  NAMES cairomm/cairomm.h
  PATHS ${Cairomm_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES cairomm-1.0
)

libfind_library(Cairomm cairomm 1.0)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Cairomm_PROCESS_INCLUDES Cairomm_INCLUDE_DIR Cairo_INCLUDE_DIRS)
set(Cairomm_PROCESS_LIBS Cairomm_LIBRARY Cairo_LIBRARIES)
libfind_process(Cairomm)

# FIXME: figure out how to let libfind locate libsigc++
set(Cairomm_INCLUDE_DIRS ${Cairomm_PKGCONF_INCLUDE_DIRS})

