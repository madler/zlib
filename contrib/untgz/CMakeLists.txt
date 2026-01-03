cmake_minimum_required(VERSION 3.12...3.31)

project(
    untgz
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "A tool for listing and uncompresssing tgz files"
    HOMEPAGE_URL "https://www.zlib.net")

option(ZLIB_UNTGZ_BUILD_SHARED "Enable building untgz" ON)
option(ZLIB_UNTGZ_BUILD_STATIC "Enable building static linked untgz" ON)
option(ZLIB_UNTGZ_INSTALL "Enable installation of untgz" ON)

include(GNUInstallDirs)

if(NOT DEFINED ZLIB_BUILD_UNTGZ)
    if(ZLIB_PUFF_BUILD_SHARED)
        list(APPEND REQUIRED_COMPONENTS "shared")
    endif(ZLIB_PUFF_BUILD_SHARED)

    if(ZLIB_PUFF_BUILD_STATIC)
        list(APPEND REQUIRED_COMPONENTS "static")
    endif(ZLIB_PUFF_BUILD_STATIC)

    find_package(ZLIB REQUIRED COMPONENTS ${REQUIRED_COMPONENTS} CONFIG)
endif(NOT DEFINED ZLIB_BUILD_UNTGZ)

if(ZLIB_UNTGZ_BUILD_SHARED)
    add_executable(untgz untgz.c)
    target_link_libraries(untgz PRIVATE ZLIB::ZLIB)

    if(ZLIB_UNTGZ_INSTALL)
        install(
            TARGETS zlib-untgz
            COMPONENT Runtime
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif(ZLIB_UNTGZ_INSTALL)
endif(ZLIB_UNTGZ_BUILD_SHARED)

if(ZLIB_UNTGZ_BUILD_STATIC)
    add_executable(untgzStatic untgz.c)
    target_link_libraries(untgzStatic PRIVATE ZLIB::ZLIBSTATIC)

    if(ZLIB_UNTGZ_INSTALL)
        install(
            TARGETS zlib-untgzStatic
            COMPONENT Runtime
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif(ZLIB_UNTGZ_INSTALL)
endif(ZLIB_UNTGZ_BUILD_STATIC)
