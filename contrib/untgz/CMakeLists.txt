cmake_minimum_required(VERSION 3.12...3.31)

project(
    untgz
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "A tool for listing and uncompresssing tgz files"
    HOMEPAGE_URL "https://www.zlib.net")

option(ZLIB_UNTGZ_BUILD_SHARED "Enable building untgz" ON)
option(ZLIB_UNTGZ_BUILD_STATIC "Enable building static linked untgz" ON)

if(ZLIB_UNTGZ_BUILD_SHARED)
    add_executable(untgz untgz.c)
    target_link_libraries(untgz PRIVATE ZLIB::ZLIB)
endif(ZLIB_UNTGZ_BUILD_SHARED)

if(ZLIB_UNTGZ_BUILD_STATIC)
    add_executable(untgzStatic untgz.c)
    target_link_libraries(untgzStatic PRIVATE ZLIB::ZLIBSTATIC)
endif(ZLIB_UNTGZ_BUILD_STATIC)
