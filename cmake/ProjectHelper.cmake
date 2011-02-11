set(CMAKE_CXX_FLAGS_PACKAGE ${CMAKE_CXX_FLAGS_RELEASE})
set(CMAKE_C_FLAGS_PACKAGE ${CMAKE_CXX_FLAGS_RELEASE})

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE release CACHE STRING
        "Options: None Debug Release Package RelWithDebInfo MinSizeRel." FORCE)
    message("No CMAKE_BUILD_TYPE specified, defaulting to release")
endif ()

macro(add_projects dir)
    file(GLOB projects RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${dir}/[a-z]*")
    foreach( proj ${projects} )
        add_subdirectory( ${proj} ${CMAKE_SOURCE_DIR}/build/${proj} )
    endforeach(proj ${projects})
endmacro(add_projects dir)
