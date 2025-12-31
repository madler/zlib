cmake_minimum_required(VERSION 3.12...3.31)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

project(
    iostreamsV3
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "A library for using C++ IOStreams with zlib V3"
    HOMEPAGE_URL "https://www.zlib.net")

option(ZLIB_IOSTREAM3_BUILD_SHARED "Enable building blast shared library" ON)
option(ZLIB_IOSTREAM3_BUILD_STATIC "Enable building blast static library" ON)
option(ZLIB_IOSTREAM3_BUILD_TESTING "Enable building tests for blast" ON)

if(NOT DEFINED ZLIB_BUILD_IOSTREAM3)
    if(ZLIB_IOSTREAM3_BUILD_SHARED)
        list(APPEND REQUIRED_COMPONENTS "shared")
    endif(ZLIB_IOSTREAM3_BUILD_SHARED)

    if(ZLIB_IOSTREAM3_BUILD_STATIC)
        list(APPEND REQUIRED_COMPONENTS "static")
    endif(ZLIB_IOSTREAM3_BUILD_STATIC)

    find_package(ZLIB REQUIRED COMPONENTS ${REQUIRED_COMPONENTS} CONFIG)
endif(NOT DEFINED ZLIB_BUILD_IOSTREAM3)

if(WIN32 OR CYGWIN)
    set(zlibIOStream3_static_suffix "s")
    set(CMAKE_DEBUG_POSTFIX "d")
endif(WIN32 OR CYGWIN)

if(ZLIB_IOSTREAM3_BUILD_SHARED)
    add_library(zlib-iostream3 SHARED
                zfstream.cc
                zfstream.h)

    target_link_libraries(zlib-iostream3
        PUBLIC ZLIB::ZLIB)

    if(ZLIB_IOSTREAM3_BUILD_TESTING)
        enable_testing()

        add_executable(iostream-test test.cc)

        target_link_libraries(iostream-test
            PRIVATE zlib-iostream3)

        add_test(NAME zlib_iostream3_test COMMAND iostream-test)

        set_tests_properties(zlib_iostream3_test
            PROPERTIES
                FIXTURES_REQUIRED iostream3_cleanup)
    endif(ZLIB_IOSTREAM3_BUILD_TESTING)
endif(ZLIB_IOSTREAM3_BUILD_SHARED)

if(ZLIB_IOSTREAM3_BUILD_STATIC)
    add_library(zlib-iostream3Static STATIC
                zfstream.cc
                zfstream.h)

    target_link_libraries(zlib-iostream3Static
        INTERFACE ZLIB::ZLIBSTATIC)

    set_target_properties(zlib-iostream3Static
        PROPERTIES
            OUTPUT_NAME zlib-iostream3${zlib_IOStream3_static_suffix})

    if(ZLIB_IOSTREAM3_BUILD_TESTING)
        enable_testing()

        add_executable(iostream-testStatic test.cc)

        target_link_libraries(iostream-testStatic
            PRIVATE zlib-iostream3Static)
        add_test(NAME zlib_iostream3_testStatic COMMAND iostream-testStatic)

        set_tests_properties(zlib_iostream3_testStatic
            PROPERTIES
                FIXTURES_REQUIRED iostream3_cleanup)
    endif(ZLIB_IOSTREAM3_BUILD_TESTING)
endif(ZLIB_IOSTREAM3_BUILD_STATIC)

if(ZLIB_IOSTREAM3_BUILD_TESTING)
    add_test(NAME zlib_iostream3_cleanup COMMAND ${CMAKE_COMMAND} -E rm
        ${CMAKE_CURRENT_BINARY_DIR}/test1.txt.gz
        ${CMAKE_CURRENT_BINARY_DIR}/test2.txt.gz)

    set_tests_properties(zlib_iostream3_cleanup
        PROPERTIES
            FIXTURES_CLEANUP zlib_iostream3_cleanup)
endif(ZLIB_IOSTREAM3_BUILD_TESTING)
