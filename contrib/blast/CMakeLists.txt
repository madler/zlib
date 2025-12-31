cmake_minimum_required(VERSION 3.12...3.31)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

project(
    blast
    VERSION 1.3.0
    LANGUAGES C
    DESCRIPTION "A library for creating zipfiles based in zlib"
    HOMEPAGE_URL "https://www.zlib.net")

option(ZLIB_BLAST_BUILD_SHARED "Enable building blast shared library" ON)
option(ZLIB_BLAST_BUILD_STATIC "Enable building blast static library" ON)
option(ZLIB_BLAST_BUILD_TESTING "Enable building tests for blast" ON)

if(WIN32 OR CYGWIN)
    set(zlibAda_static_suffix "s")
    set(CMAKE_DEBUG_POSTFIX "d")
endif(WIN32 OR CYGWIN)

function(blast_findTestEnv testName)
    set(testEnv "PATH=")

    if(MSVC OR MINGW)
        set(separator "\\\;")
    else()
        set(separator ":")
    endif()

    string(APPEND testEnv "$<TARGET_FILE_DIR:ZLIB::ZLIB>${separator}")
    string(APPEND testEnv "$ENV{PATH}")

    set_tests_properties(${testName} PROPERTIES ENVIRONMENT "${testEnv}")
endfunction(blast_findTestEnv testName)

if(ZLIB_BLAST_BUILD_SHARED)
    add_library(blast SHARED
                blast.c
                blast.h)

    if(ZLIB_BLAST_BUILD_TESTING)
        enable_testing()
        add_executable(blast-test blast-test.c)
        target_link_libraries(blast-test blast)

        add_test(NAME blast_blast-test
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tester.cmake
                    "$<TARGET_FILE:blast-test>"
                    "${CMAKE_CURRENT_SOURCE_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}")

        if(MSVC
           OR MSYS
           OR MINGW
           OR CYGWIN)
            blast_findtestenv(blast-test)
        endif(
            MSVC
            OR MSYS
            OR MINGW
            OR CYGWIN)
    endif(ZLIB_BLAST_BUILD_TESTING)
endif(ZLIB_BLAST_BUILD_SHARED)

if(ZLIB_BLAST_BUILD_STATIC)
    add_library(blastStatic STATIC
                blast.c
                blast.h)

    set_target_properties(blastStatic
        PROPERTIES
            OUTPUT_NAME blast${minizip_static_suffix})

    if(ZLIB_BLAST_BUILD_TESTING)
        enable_testing()
        add_executable(blast-testStatic blast-test.c)
        target_link_libraries(blast-testStatic blastStatic)

        add_test(NAME blast_testStatic
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tester.cmake
                    "$<TARGET_FILE:blast-testStatic>"
                    "${CMAKE_CURRENT_SOURCE_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}")
    endif(ZLIB_BLAST_BUILD_TESTING)
endif(ZLIB_BLAST_BUILD_STATIC)
