cmake_minimum_required(VERSION 3.12...3.31)

project(
    iostreamsV3
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "A library for decompressing deflate with very little footprint"
    HOMEPAGE_URL "https://www.zlib.net")

option(ZLIB_PUFF_BUILD_SHARED "Enable building blast shared library" ON)
option(ZLIB_PUFF_BUILD_STATIC "Enable building blast static library" ON)
option(ZLIB_PUFF_BUILD_TESTING "Enable building tests for blast" ON)

if(NOT DEFINED ZLIB_BUILD_PUFF)
    if(ZLIB_PUFF_BUILD_SHARED)
        list(APPEND REQUIRED_COMPONENTS "shared")
    endif(ZLIB_PUFF_BUILD_SHARED)

    if(ZLIB_PUFF_BUILD_STATIC)
        list(APPEND REQUIRED_COMPONENTS "static")
    endif(ZLIB_PUFF_BUILD_STATIC)

    find_package(ZLIB REQUIRED COMPONENTS ${REQUIRED_COMPONENTS} CONFIG)
endif(NOT DEFINED ZLIB_BUILD_PUFF)

if(WIN32 OR CYGWIN)
    set(zlibpuff_static_suffix "s")
    set(CMAKE_DEBUG_POSTFIX "d")
endif(WIN32 OR CYGWIN)

if(ZLIB_PUFF_BUILD_TESTING)
    add_executable(bin-writer bin-writer.c)

    if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR
       ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
        set(CFLAGS_OLD ${CMAKE_C_FLAGS})
        set(CMAKE_C_FLAGS
            ""
            CACHE STRING "" FORCE)

        if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
            find_program(GCOV_EXECUTABLE gcov)
        endif(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")

        if(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
            set(llvm_names llvm_cov)
            list(APPEND llvm_names llvm-cov)

            foreach(ver RANGE 11 99)
                list(APPEND llvm_names llvm-cov-${ver})
            endforeach(ver RANGE 11 99)

            find_program(GCOV_EXECUTABLE NAMES ${llvm_names})
            set(llvm_option "gcov")
        endif(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    endif(${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR
          ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
endif(ZLIB_PUFF_BUILD_TESTING)

if(ZLIB_PUFF_BUILD_SHARED)
    add_library(zlib-puff SHARED
                puff.c
                puff.h)

    if(ZLIB_PUFF_BUILD_TESTING)
        enable_testing()

        add_executable(puff-test pufftest.c)

        target_link_libraries(puff-test
            PRIVATE zlib-puff)

        add_test(NAME zlib_puff_test
            COMMAND ${CMAKE_COMMAND} -P
                    ${CMAKE_CURRENT_SOURCE_DIR}/tester.cmake
                    "$<TARGET_FILE:puff-test>"
                    "${CMAKE_CURRENT_SOURCE_DIR}")

        if(GCOV_EXECUTABLE)
            add_executable(puff-test-coverage pufftest.c puff.c)

            target_compile_options(puff-test-coverage PRIVATE -coverage)

            target_link_options(puff-test-coverage PRIVATE -coverage)

            add_test(NAME zlib_puff_test_coverage
                COMMAND ${CMAKE_COMMAND} -P
                        ${CMAKE_CURRENT_SOURCE_DIR}/tester-cov.cmake
                        "$<TARGET_FILE:puff-test-coverage>"
                        "${CMAKE_CURRENT_SOURCE_DIR}"
                        "${CMAKE_CURRENT_BINARY_DIR}"
                        ${GCOV_EXECUTABLE}
                        ${llvm_option})
        endif(GCOV_EXECUTABLE)
    endif(ZLIB_PUFF_BUILD_TESTING)
endif(ZLIB_PUFF_BUILD_SHARED)

if(ZLIB_PUFF_BUILD_STATIC)
    add_library(zlib-puffStatic STATIC
                puff.c
                puff.h)

    set_target_properties(zlib-puffStatic
        PROPERTIES
            OUTPUT_NAME zlib-puff${zlib_puff_static_suffix})

    if(ZLIB_PUFF_BUILD_TESTING)
        enable_testing()

        add_executable(puff-testStatic pufftest.c)

        target_link_libraries(puff-testStatic
            PRIVATE zlib-puffStatic)
        add_test(NAME zlib_puff_testStatic
            COMMAND ${CMAKE_COMMAND} -P
                    ${CMAKE_CURRENT_SOURCE_DIR}/tester.cmake
                    "$<TARGET_FILE:puff-testStatic>"
                    "${CMAKE_CURRENT_SOURCE_DIR}")

        if(GCOV_EXECUTABLE)
            add_executable(puff-testStatic-coverage pufftest.c puff.c)

            target_compile_options(puff-testStatic-coverage PRIVATE -coverage)

            target_link_options(puff-testStatic-coverage PRIVATE -coverage)

            add_test(NAME zlib_puff_testStatic_coverage
                COMMAND ${CMAKE_COMMAND} -P
                    ${CMAKE_CURRENT_SOURCE_DIR}/tester-cov.cmake
                    "$<TARGET_FILE:puff-testStatic-coverage>"
                    "${CMAKE_CURRENT_SOURCE_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}"
                    ${GCOV_EXECUTABLE}
                    ${llvm_option})
        endif(GCOV_EXECUTABLE)
    endif(ZLIB_PUFF_BUILD_TESTING)
endif(ZLIB_PUFF_BUILD_STATIC)
