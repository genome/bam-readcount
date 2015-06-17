##############################################################################
# PreDepends.cmake
#
# Macros to cope with the fact that cmake doesn't support the notion
# of dependencies that must be built before any other target when
# doing parallel builds.
#
# SYNOPSIS:
#
# Include this file in your top level CMakeLists.txt and call
#
#   PredependsInit()
#
# precisely once.
#
# Then, use

#   ExternalDependency_Add(<NAME> BUILD_BYPRODUCTS <BYPRODUCTS> ARGS ...)
# instead of
#   ExternalProject_Add(<NAME> BUILD_BYPRODUCTS <BYPRODUCTS> ...)

#   xadd_executable(<NAME> ...)
# instead of
#   add_executable(<NAME> ...)

#   xadd_library(<NAME> ...)
# instead of
#   add_library(<NAME> ...)


# This will ensure that the external dependencies get built before
# any targets added with xadd_{executable,library}.


# METHODS:
# This module sets up a dummy target "__bc_predepends" and provides a
# macro:
#
#   add_predepend(foo)
#
# This macro makes the __bc_predepends target depend on foo. It can
# be used directly, e.g.,
#
#   ExternalProject_Add(foo BUILD_BYPRODUCTS path/to/libfoo.a ...)
#   add_predepend(foo)
#
# but the typical usage is to call
#   ExternalDependency_Add(foo BUILD_BYPRODUCTS path/to/libfoo.a ARGS ...)
# instead.
#
# We provide another set of macros:
#
#   xadd_executable(bar bar.c)
#   xadd_library(baz baz.c)
#
# These function exactly like the native add_executable/add_library
# commands, but they also add a dependency on __bc_predepends
# to the target being generated (bar and baz in this case).
# This forces any registered predepends targets to be built first.
# In the example at hand, we get the following dependency graph:
#
# bar -----------> __bc_predepends -> foo
# libbaz.a ----`
#
# where a -> b is pronounced "a depends on b".
#
# NOTE: cmake 3.2+ is required to use the Ninja generator
cmake_minimum_required(VERSION 2.8)

set(PREDEPENDS_TARGET_NAME "__bc_predepends")

# Deal with multiple inclusion of this file
macro(PreDependsInit)
    add_custom_target(${PREDEPENDS_TARGET_NAME} ALL)
endmacro()

macro(add_predepend __TARGET_NAME)
    add_dependencies(${PREDEPENDS_TARGET_NAME} ${__TARGET_NAME})
endmacro()

macro(xadd_executable __TARGET_NAME)
    add_executable(${__TARGET_NAME} ${ARGN})
    add_dependencies(${__TARGET_NAME} ${PREDEPENDS_TARGET_NAME})
endmacro()

macro(xadd_library __TARGET_NAME)
    add_library(${__TARGET_NAME} ${ARGN})
    add_dependencies(${__TARGET_NAME} ${PREDEPENDS_TARGET_NAME})
endmacro()

macro(ExternalDependency_Add NAME)
    set(multiValueArgs BUILD_BYPRODUCTS ARGS)
    cmake_parse_arguments(extdep_add "" "" "${multiValueArgs}" ${ARGN})

    # Listing the byproducts is not needed for the "Unix Makefiles" generator.
    # It is, however, required for Ninja. I don't know about any of the other
    # generators...
    unset(BYPRODUCTS_LIST)
    if (CMAKE_GENERATOR MATCHES "Ninja")
        if(CMAKE_VERSION VERSION_LESS "3.2")
            message(FATAL_ERROR "The Ninja generator requires CMake 3.2+. Try the \"Unix Makefiles\" generator instead.")
        endif()
        set(BYPRODUCTS_LIST BUILD_BYPRODUCTS "${extdep_add_BUILD_BYPRODUCTS}")
    endif()

    set(arg_list "${extdep_add_ARGS}")
    ExternalProject_Add(
        ${NAME}
        "${BYPRODUCTS_LIST}"
        "${arg_list}"
    )
    add_predepend(${NAME})
endmacro()
