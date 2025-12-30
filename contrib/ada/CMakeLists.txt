cmake_minimum_required(VERSION 3.12...3.31)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

project(
    zlibAda
    VERSION 1.0.0
    LANGUAGES ADA
    DESCRIPTION "A library for creating zipfiles based in zlib"
    HOMEPAGE_URL "https://www.zlib.net")

option(ZLIBADA_BUILD_SHARED "Enable building ada bindings shared library" ON)
option(ZLIBADA_BUILD_STATIC "Enable building ada bindings static library" ON)
option(ZLIBADA_BUILD_TESTING "Enable building tests for ada bindings library" ON)

if(WIN32 OR CYGWIN)
    set(zlibAda_static_suffix "s")
    set(CMAKE_DEBUG_POSTFIX "d")
endif(WIN32 OR CYGWIN)

if(NOT DEFINED ZLIB_BUILD_ADA)
    if(ZLIBADA_BUILD_SHARED)
        list(APPEND REQUIRED_COMPONENTS "shared")
    endif(ZLIBADA_BUILD_SHARED)

    if(ZLIBADA_BUILD_STATIC)
        list(APPEND REQUIRED_COMPONENTS "static")
    endif(ZLIB_BUILD_STATIC)

    find_package(ZLIB REQUIRED COMPONENTS ${REQUIRED_COMPONENTS} CONFIG)
endif(NOT DEFINED ZLIB_BUILD_ADA)

function(ZLIBADA_findTestEnv testName)
    set(testEnv "PATH=")

    if(MSVC OR MINGW)
        set(separator "\\\;")
    else()
        set(separator ":")
    endif()

    string(APPEND testEnv "$<TARGET_FILE_DIR:ZLIB::ZLIB>${separator}")
    string(APPEND testEnv "$ENV{PATH}")

    set_tests_properties(${testName} PROPERTIES ENVIRONMENT "${testEnv}")
endfunction(ZLIBADA_findTestEnv testName)

if(ZLIBADA_BUILD_SHARED)
    ada_add_library(zlib-Ada SHARED
        zlib-thin.adb
        zlib.adb)

    target_link_libraries(zlib-Ada
        INTERFACE ZLIB::ZLIB)

    ada_add_library(zlib-streams SHARED
        zlib-streams.adb)

    target_link_libraries(zlib-streams
        PUBLIC
            zlib-Ada)

    ada_find_ali(zlib-streams)

    if(ZLIBADA_BUILD_TESTING)
        enable_testing()
        ada_add_executable(ada-test test.adb)

        target_link_libraries(ada-test
            PRIVATE
                zlib-Ada
                zlib-streams)

        ada_find_ali(ada-test)

        add_test(NAME zlibAda_ada-test COMMAND ada-test)
        set_tests_properties(zlibAda_ada-test PROPERTIES FIXTURES_REQUIRED ada_cleanup)

        if(MSVC
           OR MSYS
           OR MINGW
           OR CYGWIN)
            zlibada_findtestenv(zlibAda_ada-test)
        endif(
            MSVC
            OR MSYS
            OR MINGW
            OR CYGWIN)

        ada_add_executable(buffer_demo buffer_demo.adb)

        target_link_libraries(buffer_demo
            PRIVATE
                zlib-Ada)

        ada_find_ali(buffer_demo)

        add_test(NAME zlibAda_buffer-demo COMMAND buffer_demo)

        if(MSVC
           OR MSYS
           OR MINGW
           OR CYGWIN)
            zlibada_findtestenv(zlibAda_buffer-demo)
        endif(
            MSVC
            OR MSYS
            OR MINGW
            OR CYGWIN)

        ada_add_executable(mtest mtest.adb)

        target_link_libraries(mtest
            PRIVATE
                zlib-Ada)

        ada_find_ali(mtest)

        #Not adding test as this is an endless-loop

        ada_add_executable(read read.adb)

        target_link_libraries(read
            PRIVATE
                zlib-Ada)

        ada_find_ali(read)

        add_test(NAME zlibAda_read COMMAND read)

        if(MSVC
           OR MSYS
           OR MINGW
           OR CYGWIN)
            zlibada_findtestenv(zlibAda_read)
        endif(
            MSVC
            OR MSYS
            OR MINGW
            OR CYGWIN)
    endif(ZLIBADA_BUILD_TESTING)
endif(ZLIBADA_BUILD_SHARED)

if(ZLIBADA_BUILD_STATIC)
    ada_add_library(zlib-AdaStatic STATIC
        zlib-thin.adb
        zlib.adb)

    target_link_libraries(zlib-AdaStatic
        INTERFACE ZLIB::ZLIBSTATIC)

    set_target_properties(zlib-AdaStatic
        PROPERTIES OUTPUT_NAME zlib-Ada${zlibAda_static_suffix})

    ada_add_library(zlib-streamsStatic STATIC
        zlib-streams.adb)

    target_link_libraries(zlib-streamsStatic
        PUBLIC
            zlib-AdaStatic)

    ada_find_ali(zlib-streamsStatic)

    if(ZLIBADA_BUILD_TESTING)
        enable_testing()
        ada_add_executable(ada-testStatic test.adb)

        target_link_libraries(ada-testStatic
            PRIVATE
                zlib-AdaStatic
                zlib-streamsStatic)

        ada_find_ali(ada-testStatic)

        add_test(NAME zlibAda_ada-testStatic COMMAND ada-testStatic)
        set_tests_properties(zlibAda_ada-testStatic PROPERTIES FIXTURES_REQUIRED ada_cleanup)

        ada_add_executable(buffer_demoStatic buffer_demo.adb)

        target_link_libraries(buffer_demoStatic
            PRIVATE
                zlib-AdaStatic)

        ada_find_ali(buffer_demoStatic)

        add_test(NAME zlibAda-buffer-demoStatic COMMAND buffer_demoStatic)

        ada_add_executable(mtestStatic mtest.adb)

        target_link_libraries(mtestStatic
            PRIVATE
                zlib-AdaStatic)

        ada_find_ali(mtestStatic)

        # Not adding test as this is an endless-loop

        ada_add_executable(readStatic read.adb)

        target_link_libraries(readStatic
            PRIVATE
                zlib-AdaStatic)

        ada_find_ali(readStatic)

        add_test(NAME zlibAda_readStatic COMMAND readStatic)
    endif(ZLIBADA_BUILD_TESTING)
endif(ZLIBADA_BUILD_STATIC)

if(ZLIBADA_BUILD_TESTING)
    add_test(NAME zlibAda_cleanup COMMAND ${CMAKE_COMMAND} -E rm ${CMAKE_CURRENT_BINARY_DIR}/testzlib.in
        ${CMAKE_CURRENT_BINARY_DIR}/testzlib.out ${CMAKE_CURRENT_BINARY_DIR}/testzlib.zlb)
    set_tests_properties(zlibAda_cleanup PROPERTIES FIXTURES_CLEANUP ada_cleanup)
endif(ZLIBADA_BUILD_TESTING)
