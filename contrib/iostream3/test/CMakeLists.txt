# if we are built from with zlib, use this path's)
if(ZLIB_IOSTREAM3_INSTALL)
    if(NOT DEFINED ZLIB_BUILD_IOSTREAM3)
        set(WORK_DIR ${CMAKE_CURRENT_BINARY_DIR})
        set(inst_setup ${ZLIB_CONTRIB_PREFIX}iostream3_install)
        set(ZLIB_ARG "-DZLIB_DIR=${ZLIB_DIR}")

        add_test(
            NAME ${ZLIB_CONTRIB_PREFIX}iostream3_install
            COMMAND ${CMAKE_COMMAND}
                    --install ${iostreamV3_BINARY_DIR}
                    --prefix ${WORK_DIR}/test_install
                    --config $<CONFIG>
            WORKING_DIRECTORY ${WORK_DIR})

        set_tests_properties(${ZLIB_CONTRIB_PREFIX}iostream3_install
            PROPERTIES
                FIXTURES_SETUP ${ZLIB_CONTRIB_PREFIX}iostream3_install)
    endif(NOT DEFINED ZLIB_BUILD_IOSTREAM3)

    function(iostreamv3_findTestEnv testName)
        set(testEnv "PATH=")

        if(MSVC OR MINGW)
            set(separator "\\\;")
        else()
            set(separator ":")
        endif()

        string(APPEND testEnv
            "$<TARGET_FILE_DIR:${ZLIB_CONTRIB_PREFIX}iostream3_iostreamv3>${separator}")
        string(APPEND testEnv "$<TARGET_FILE_DIR:ZLIB::ZLIB>${separator}")
        string(APPEND testEnv "$ENV{PATH}")

        set_tests_properties(${testName} PROPERTIES ENVIRONMENT "${testEnv}")
    endfunction(iostreamv3_findTestEnv testName)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/find_package_test.cmake.in
        ${WORK_DIR}/iostream3_find_package_test/CMakeLists.txt @ONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory_test.cmake.in
        ${WORK_DIR}/iostream3_add_subdirectory_test/CMakeLists.txt @ONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory_exclude_test.cmake.in
        ${WORK_DIR}/iostream3_add_subdirectory_exclude_test/CMakeLists.txt
        @ONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/find_package_no_components_test.cmake.in
        ${WORK_DIR}/iostream3_find_package_no_components_test/CMakeLists.txt
        @ONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/find_package_wrong_components_test.cmake.in
        ${WORK_DIR}/iostream3_find_package_wrong_components_test/CMakeLists.txt
        @ONLY)

    # CMAKE_GENERATOR_PLATFORM doesn't work in the if
    set(GENERATOR ${CMAKE_GENERATOR_PLATFORM})

    if(GENERATOR)
        set(PLATFORM "-A ${GENERATOR}")
    endif(GENERATOR)

    #
    # findpackage_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}iostream3_find_package_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/iostream3_find_package_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/iostream3_find_package_test)

    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}iostream3_find_package_build
        COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
        WORKING_DIRECTORY ${WORK_DIR}/iostream3_find_package_test_build)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}iostream3_find_package_configure
        PROPERTIES
            FIXTURES_REQUIRED ${inst_setup}
            FIXTURES_SETUP iostream3_fp_config)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}iostream3_find_package_build
        PROPERTIES
            FIXTURES_REQUIRED iostream3_fp_config)

    #
    # add_subdirectory_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}iostream3_add_subdirectory_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/iostream3_add_subdirectory_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/iostream3_add_subdirectory_test)

    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}iostream3_add_subdirectory_build
        COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
        WORKING_DIRECTORY ${WORK_DIR}/iostream3_add_subdirectory_test_build)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}iostream3_add_subdirectory_configure
        PROPERTIES
            FIXTURES_REQUIRED ${inst_setup}
            FIXTURES_SETUP iostream3_as_config)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}iostream3_add_subdirectory_build
        PROPERTIES
            FIXTURES_REQUIRED iostream3_as_config)

    #
    # add_subdirectory_exclude_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}iostream3_add_subdirectory_exclude_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/iostream3_add_subdirectory_exclude_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/iostream3_add_subdirectory_exclude_test)

    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}iostream3_add_subdirectory_exclude_build
        COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
        WORKING_DIRECTORY
            ${WORK_DIR}/iostream3_add_subdirectory_exclude_test_build)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}iostream3_add_subdirectory_exclude_configure
            PROPERTIES
                FIXTURES_REQUIRED ${inst_setup}
                FIXTURES_SETUP iostream3_asx_config)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}iostream3_add_subdirectory_exclude_build
            PROPERTIES
                FIXTURES_REQUIRED iostream3_asx_config)

    #
    # find_package_no_components_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}iostream3_find_package_no_components_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/iostream3_find_package_no_components_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/iostream3_find_package_no_components_test)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}iostream3_find_package_no_components_configure
            PROPERTIES
                FIXTURES_REQUIRED ${inst_setup})

    if(NOT ZLIB_IOSTREAM3_BUILD_SHARED OR NOT ZLIB_IOSTREAM3_BUILD_STATIC)
        set_tests_properties(
            ${ZLIB_CONTRIB_PREFIX}iostream3_find_package_no_components_configure
                PROPERTIES
                    WILL_FAIL TRUE)
    endif(NOT ZLIB_IOSTREAM3_BUILD_SHARED OR NOT ZLIB_IOSTREAM3_BUILD_STATIC)

    #
    # find_package_no_components_test
    #
    add_test(
        NAME
            ${ZLIB_CONTRIB_PREFIX}iostream3_find_package_wrong_components_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/iostream3_find_package_wrong_components_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/iostream3_find_package_wrong_components_test)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}iostream3_find_package_wrong_components_configure
            PROPERTIES
                FIXTURES_REQUIRED ${inst_setup}
                WILL_FAIL TRUE)
endif(ZLIB_IOSTREAM3_INSTALL)

if(ZLIB_IOSTREAM3_BUILD_SHARED)
    add_executable(${ZLIB_CONTRIB_PREFIX}iostream3_test
        ${iostreamV3_SOURCE_DIR}/test.cc
        ${iostreamV3_SOURCE_DIR}/zfstream.h)

    target_link_libraries(${ZLIB_CONTRIB_PREFIX}iostream3_test
        PRIVATE IOSTREAMV3::IOSTREAMV3)

    add_test(NAME ${ZLIB_CONTRIB_PREFIX}iostream3_test
        COMMAND ${ZLIB_CONTRIB_PREFIX}iostream3_test)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}iostream3_test
        PROPERTIES
            FIXTURES_SETUP iostream3_tests
            RESOURCE_LOCK iostram_txt_gz)

    if(MSVC
       OR MSYS
       OR MINGW
       OR CYGWIN)
        iostreamv3_findtestenv(${ZLIB_CONTRIB_PREFIX}iostream3_test)
    endif(
        MSVC
        OR MSYS
        OR MINGW
        OR CYGWIN)
endif(ZLIB_IOSTREAM3_BUILD_SHARED)

if(ZLIB_IOSTREAM3_BUILD_STATIC)
    add_executable(${ZLIB_CONTRIB_PREFIX}iostream3_testStatic
        ${iostreamV3_SOURCE_DIR}/test.cc
        ${iostreamV3_SOURCE_DIR}/zfstream.h)

    target_link_libraries(${ZLIB_CONTRIB_PREFIX}iostream3_testStatic
        PRIVATE IOSTREAMV3::IOSTREAMV3STATIC)

    add_test(NAME ${ZLIB_CONTRIB_PREFIX}iostream3_testStatic
             COMMAND ${ZLIB_CONTRIB_PREFIX}iostream3_testStatic)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}iostream3_testStatic
        PROPERTIES
            FIXTURES_SETUP iostream3_tests
            RESOURCE_LOCK iostram_txt_gz)
endif(ZLIB_IOSTREAM3_BUILD_STATIC)

add_test(NAME ${ZLIB_CONTRIB_PREFIX}iostream3_cleanup
    COMMAND ${CMAKE_COMMAND} -E rm
        ${CMAKE_CURRENT_BINARY_DIR}/test1.txt.gz
        ${CMAKE_CURRENT_BINARY_DIR}/test2.txt.gz)

set_tests_properties(${ZLIB_CONTRIB_PREFIX}iostream3_cleanup
    PROPERTIES
        FIXTURES_CLEANUP iostream3_tests)
