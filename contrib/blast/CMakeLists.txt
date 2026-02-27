cmake_minimum_required(VERSION 3.12...3.31)

project(
    blast
    VERSION 1.3.0
    LANGUAGES C
    DESCRIPTION "A library for creating zipfiles based in zlib"
    HOMEPAGE_URL "https://www.zlib.net")

if(ZLIB_BUILD_BLAST)
    set(ZLIB_BLAST_BUILD_SHARED ${ZLIB_BUILD_SHARED} CACHE BOOL "")
    set(ZLIB_BLAST_BUILD_STATIC ${ZLIB_BUILD_STATIC} CACHE BOOL "")
    set(ZLIB_BLAST_BUILD_TESTING ${ZLIB_BUILD_TESTING} CACHE BOOL "")
    set(ZLIB_BLAST_INSTALL ${ZLIB_INSTALL} CACHE BOOL "")
endif(ZLIB_BUILD_BLAST)

option(ZLIB_BLAST_BUILD_SHARED "Enable building blast shared library" ON)
option(ZLIB_BLAST_BUILD_STATIC "Enable building blast static library" ON)
option(ZLIB_BLAST_BUILD_TESTING "Enable building tests for blast" ON)
option(ZLIB_BLAST_INSTALL "Enable installation of blast" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(WIN32 OR CYGWIN)
    set(${ZLIB_CONTRIB_PREFIX}blast_static_suffix "s")
    set(CMAKE_DEBUG_POSTFIX "d")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif(WIN32 OR CYGWIN)

function(blast_findTestEnv testName)
    set(testEnv "PATH=")

    if(MSVC OR MINGW)
        set(separator "\\\;")
    else()
        set(separator ":")
    endif()

    string(APPEND testEnv "$<TARGET_FILE_DIR:BLAST::BLAST>${separator}")
    string(APPEND testEnv "$ENV{PATH}")

    set_tests_properties(${testName} PROPERTIES ENVIRONMENT "${testEnv}")
endfunction(blast_findTestEnv testName)

if(ZLIB_BLAST_BUILD_SHARED)
    add_library(${ZLIB_CONTRIB_PREFIX}blast_blast SHARED
                blast.c
                blast.h)

    add_library(BLAST::BLAST ALIAS ${ZLIB_CONTRIB_PREFIX}blast_blast)

    if(NOT CYGWIN)
        set_target_properties(${ZLIB_CONTRIB_PREFIX}blast_blast
            PROPERTIES
                SOVERSION ${blast_VERSION_MAJOR}
                VERSION ${blast_VERSION})
    endif(NOT CYGWIN)

    set_target_properties(${ZLIB_CONTRIB_PREFIX}blast_blast
        PROPERTIES
            EXPORT_NAME BLAST
            OUTPUT_NAME blast)

    if(ZLIB_BLAST_BUILD_TESTING)
        enable_testing()
        add_executable(${ZLIB_CONTRIB_PREFIX}blast_test blast-test.c)
        target_link_libraries(${ZLIB_CONTRIB_PREFIX}blast_test
            PRIVATE
                ${ZLIB_CONTRIB_PREFIX}blast_blast)

        add_test(NAME ${ZLIB_CONTRIB_PREFIX}blast_blast-test
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tester.cmake
                    "$<TARGET_FILE:${ZLIB_CONTRIB_PREFIX}blast_test>"
                    "${CMAKE_CURRENT_SOURCE_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}")

        if(MSVC
           OR MSYS
           OR MINGW
           OR CYGWIN)
            blast_findtestenv(${ZLIB_CONTRIB_PREFIX}blast_blast-test)
        endif(
            MSVC
            OR MSYS
            OR MINGW
            OR CYGWIN)
    endif(ZLIB_BLAST_BUILD_TESTING)
endif(ZLIB_BLAST_BUILD_SHARED)

if(ZLIB_BLAST_BUILD_STATIC)
    add_library(${ZLIB_CONTRIB_PREFIX}blast_blastStatic STATIC
                blast.c
                blast.h)

    add_library(BLAST::BLASTSTATIC ALIAS ${ZLIB_CONTRIB_PREFIX}blast_blastStatic)

    set_target_properties(${ZLIB_CONTRIB_PREFIX}blast_blastStatic
        PROPERTIES
            EXPORT_NAME BLASTSTATIC
            OUTPUT_NAME blast${${ZLIB_CONTRIB_PREFIX}blast_static_suffix})

    if(ZLIB_BLAST_BUILD_TESTING)
        enable_testing()
        add_executable(${ZLIB_CONTRIB_PREFIX}blast_testStatic blast-test.c)
        target_link_libraries(${ZLIB_CONTRIB_PREFIX}blast_testStatic
            PRIVATE ${ZLIB_CONTRIB_PREFIX}blast_blastStatic)

        add_test(NAME ${ZLIB_CONTRIB_PREFIX}blast_testStatic
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tester.cmake
                    "$<TARGET_FILE:${ZLIB_CONTRIB_PREFIX}blast_testStatic>"
                    "${CMAKE_CURRENT_SOURCE_DIR}"
                    "${CMAKE_CURRENT_BINARY_DIR}")
    endif(ZLIB_BLAST_BUILD_TESTING)
endif(ZLIB_BLAST_BUILD_STATIC)

if(ZLIB_BLAST_BUILD_TESTING)
    add_subdirectory(test)
endif(ZLIB_BLAST_BUILD_TESTING)

if(ZLIB_BLAST_INSTALL)
    if(ZLIB_BLAST_BUILD_SHARED)
        install(
            TARGETS ${ZLIB_CONTRIB_PREFIX}blast_blast
            COMPONENT Runtime
            EXPORT blastSharedExport
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
        install(
            EXPORT blastSharedExport
            FILE blast-shared.cmake
            NAMESPACE BLAST::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/blast)

        if(MSVC)
            install(
                FILES $<TARGET_PDB_FILE:${ZLIB_CONTRIB_PREFIX}blast_blast>
                COMPONENT Development
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                CONFIGURATIONS Debug OR RelWithDebInfo
                OPTIONAL)
        endif(MSVC)
    endif(ZLIB_BLAST_BUILD_SHARED)

    if(ZLIB_BLAST_BUILD_STATIC)
        install(
            TARGETS ${ZLIB_CONTRIB_PREFIX}blast_blastStatic
            COMPONENT Development
            EXPORT blastStaticExport
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
        install(
            EXPORT blastStaticExport
            FILE blast-static.cmake
            NAMESPACE BLAST::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/blast)
    endif(ZLIB_BLAST_BUILD_STATIC)

    configure_package_config_file(
        ${blast_SOURCE_DIR}/blastConfig.cmake.in
        ${blast_BINARY_DIR}/blastConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/blast)

    write_basic_package_version_file(
        "${blast_BINARY_DIR}/blastConfigVersion.cmake"
        VERSION "${blast_VERSION}"
        COMPATIBILITY AnyNewerVersion)

    install(FILES ${blast_BINARY_DIR}/blastConfig.cmake
                  ${blast_BINARY_DIR}/blastConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/blast)
    install(
        FILES blast.h
        COMPONENT Development
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif(ZLIB_BLAST_INSTALL)
