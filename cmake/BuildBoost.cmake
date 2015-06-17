cmake_minimum_required(VERSION 2.8)

set(DEFAULT_BOOST_URL ${CMAKE_SOURCE_DIR}/vendor/boost-1.55-bamrc.tar.gz)
if(NOT DEFINED BOOST_URL)
    set(BOOST_URL ${DEFAULT_BOOST_URL})
endif(NOT DEFINED BOOST_URL)


set(BOOST_ROOT ${CMAKE_BINARY_DIR}/vendor/boost)
set(BOOST_SRC ${CMAKE_BINARY_DIR}/vendor/boost-src)
set(BOOST_LOG ${BOOST_SRC}/build.log)

set(BOOST_BUILD_LIBS_STRING "")

foreach(libname ${REQUIRED_BOOST_LIBS})
    set(BOOST_BUILD_LIBS ${BOOST_BUILD_LIBS} --with-${libname})
    set(Boost_LIBRARIES ${Boost_LIBRARIES}
        ${BOOST_ROOT}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}boost_${libname}${CMAKE_STATIC_LIBRARY_SUFFIX}
        )
endforeach(libname ${REQUIRED_BOOST_LIBS})

message("Extracting boost from ${BOOST_URL}")
message("Boost build log can be found at ${BOOST_LOG}")

ExternalDependency_Add(
    boost-1.55
    BUILD_BYPRODUCTS ${Boost_LIBRARIES}
    ARGS
        URL ${BOOST_URL}
        SOURCE_DIR ${BOOST_SRC}
        BINARY_DIR ${BOOST_SRC}
        CONFIGURE_COMMAND "./bootstrap.sh"
        BUILD_COMMAND
            echo "Building boost, build log is ${BOOST_LOG}" &&
            ./b2 --prefix=${BOOST_ROOT} --layout=system link=static
                    threading=multi install ${BOOST_BUILD_LIBS} > ${BOOST_LOG} 2>&1
        INSTALL_COMMAND "true"
)

set(Boost_INCLUDE_DIRS ${BOOST_ROOT}/include)
