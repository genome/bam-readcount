set(CMAKE_CXX_FLAGS_PACKAGE ${CMAKE_CXX_FLAGS_RELEASE})
set(CMAKE_C_FLAGS_PACKAGE ${CMAKE_CXX_FLAGS_RELEASE})

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
    SET(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} --coverage")
    SET(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
endif ()


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE release CACHE STRING
        "Options: None Debug Release Package RelWithDebInfo MinSizeRel Coverage." FORCE)
    message("No CMAKE_BUILD_TYPE specified, defaulting to release")
endif ()

macro(add_projects dir)
    file(GLOB projects RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${dir}/[a-z]*")
    foreach( proj ${projects} )
        add_subdirectory( ${proj} ${PROJECT_BINARY_DIR}/build/${proj} )
    endforeach(proj ${projects})
endmacro(add_projects dir)
