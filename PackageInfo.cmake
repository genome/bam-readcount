cmake_minimum_required(VERSION 2.6)

# .deb packaging
set(ARCH "i686")
if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    set(ARCH "x86_64")
endif ()

# The format of the description field is a short summary line followed by a
# longer paragraph indented by a single space on each line
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Bam readcount
 Is awesome.")
set(CPACK_PACKAGE_NAME "bam-readcount${EXE_VERSION_SUFFIX}")
set(CPACK_PACKAGE_VENDOR "wugc")
set(CPACK_PACKAGE_VERSION ${FULL_VERSION}${PACKAGE_VERSION_SUFFIX})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dave Larson <dlarson@genome.wustl.edu>")
set(CPACK_SYSTEM_NAME "Linux-${ARCH}")
set(CPACK_TOPLEVEL_TAG "Linux-${ARCH}")
set(CPACK_DEBIAN_PACKAGE_SECTION science)
set(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.4), libgcc1 (>= 1:4.1.1-21)")
if (CMAKE_BUILD_TYPE MATCHES package)
    set(CPACK_GENERATOR "DEB")
else(CMAKE_BUILD_TYPE MATCHES package)
    set(CPACK_GENERATOR "TGZ")
endif(CMAKE_BUILD_TYPE MATCHES package)

configure_file(debian/postinst.in debian/postinst @ONLY)
configure_file(debian/prerm.in debian/prerm @ONLY)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "debian/postinst;debian/prerm")

include(CPack)
