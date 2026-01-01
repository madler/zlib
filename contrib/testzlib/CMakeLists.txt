cmake_minimum_required(VERSION 3.12...3.31)

project(
    testzlib
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "A little program to test zlib"
    HOMEPAGE_URL "https://www.zlib.net")

option(ZLIB_TESTZLIB_BUILD_SHARED "Enable building testzlib" ON)
option(ZLIB_TESTZLIB_BUILD_STATIC "Enable building static linked testzlib" ON)
option(ZLIB_TESTZLIB_BUILD_TESTING "Enable building tests for testzlib" ON)

if(ZLIB_TESTZLIB_BUILD_SHARED)
    add_executable(testzlib testzlib.c)
    target_link_libraries(testzlib PRIVATE ZLIB::ZLIB)
endif(ZLIB_TESTZLIB_BUILD_SHARED)

if(ZLIB_TESTZLIB_BUILD_STATIC)
    add_executable(testzlibStatic testzlib.c)
    target_link_libraries(testzlibStatic PRIVATE ZLIB::ZLIBSTATIC)
endif(ZLIB_TESTZLIB_BUILD_STATIC)
