# if we are built from with zlib, use this path's)
if(ZLIB_MINIZIP_INSTALL)
    if(NOT DEFINED ZLIB_BUILD_MINIZIP)
        set(WORK_DIR ${CMAKE_CURRENT_BINARY_DIR})
        set(inst_setup minizip_install)
        set(ZLIB_ARG "-DZLIB_DIR=${ZLIB_DIR}")

        add_test(
            NAME minizip_install
            COMMAND ${CMAKE_COMMAND}
                    --install ${minizip_BINARY_DIR}
                    --prefix ${WORK_DIR}/test_install
                    --config $<CONFIG>
            WORKING_DIRECTORY ${WORK_DIR})

        set_tests_properties(minizip_install
            PROPERTIES
                FIXTURES_SETUP minizip_install)
    endif(NOT DEFINED ZLIB_BUILD_MINIZIP)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/find_package_test.cmake.in
        ${WORK_DIR}/minizip_find_package_test/CMakeLists.txt
        @ONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/find_package_no_components_test.cmake.in
        ${WORK_DIR}/minizip_find_package_no_components_test/CMakeLists.txt
        @ONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/find_package_wrong_components_test.cmake.in
        ${WORK_DIR}/minizip_find_package_wrong_components_test/CMakeLists.txt
        @ONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory_test.cmake.in
        ${WORK_DIR}/minizip_add_subdirectory_test/CMakeLists.txt
        @ONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/add_subdirectory_exclude_test.cmake.in
        ${WORK_DIR}/minizip_add_subdirectory_exclude_test/CMakeLists.txt
        @ONLY)

    # CMAKE_GENERATOR_PLATFORM doesn't work in the if
    set(GENERATOR ${CMAKE_GENERATOR_PLATFORM})

    if(GENERATOR)
        set(PLATFORM "-A ${GENERATOR}")
    endif(GENERATOR)

    #
    # find_package_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_find_package_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/minizip_find_package_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/minizip_find_package_test)

    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_find_package_build
        COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
        WORKING_DIRECTORY ${WORK_DIR}/minizip_find_package_test_build)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_find_package_configure
        PROPERTIES
            FIXTURES_REQUIRED ${inst_setup}
            FIXTURES_SETUP mzfp_config)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_find_package_build
        PROPERTIES
            FIXTURES_REQUIRED mzfp_config)

    #
    # find_package_no_components_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_find_package_no_components_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/minizip_find_package_no_components_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/minizip_find_package_no_components_test)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_find_package_no_components_configure
            PROPERTIES
                FIXTURES_REQUIRED ${inst_setup})

    if(NOT MINIZIP_BUILD_SHARED OR NOT MINIZIP_BUILD_STATIC)
        set_tests_properties(
            ${ZLIB_CONTRIB_PREFIX}minizip_find_package_no_components_configure
                PROPERTIES
                    WILL_FAIL TRUE)
    endif(NOT MINIZIP_BUILD_SHARED OR NOT MINIZIP_BUILD_STATIC)

    #
    # find_package_no_components_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_find_package_wrong_components_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/minizip_find_package_wrong_components_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/minizip_find_package_wrong_components_test)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_find_package_wrong_components_configure
            PROPERTIES
                FIXTURES_REQUIRED ${inst_setup}
                WILL_FAIL TRUE)

    #
    # add_subdirectory_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_add_subdirectory_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/minizip_add_subdirectory_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/minizip_add_subdirectory_test)

    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_add_subdirectory_build
        COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
        WORKING_DIRECTORY ${WORK_DIR}/minizip_add_subdirectory_test_build)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_add_subdirectory_configure
            PROPERTIES
                FIXTURES_REQUIRED ${inst_setup}
                FIXTURES_SETUP mzas_config)

    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_add_subdirectory_build
        PROPERTIES
            FIXTURES_REQUIRED mzas_config)

    #
    # add_subdirectory_exclude_test
    #
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_add_subdirectory_exclude_configure
        COMMAND
            ${CMAKE_COMMAND}
            ${PLATFORM}
            -B${WORK_DIR}/minizip_add_subdirectory_exclude_test_build
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/test_install
            ${ZLIB_ARG}
            $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},"3.24">:--fresh>
            -G "${CMAKE_GENERATOR}"
            -S${WORK_DIR}/minizip_add_subdirectory_exclude_test)

    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_add_subdirectory_exclude_build
        COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
        WORKING_DIRECTORY
            ${WORK_DIR}/minizip_add_subdirectory_exclude_test_build)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_add_subdirectory_exclude_configure
            PROPERTIES
                FIXTURES_REQUIRED ${inst_setup}
                FIXTURES_SETUP mzasx_config)

    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_add_subdirectory_exclude_build
            PROPERTIES
                FIXTURES_REQUIRED mzasx_config)
endif(ZLIB_MINIZIP_INSTALL)

if(MINIZIP_BUILD_SHARED AND
   NOT ${CMAKE_SHARED_LIBRARY_SUFFIX} STREQUAL ".dll")
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_prepare_zip_shared
        COMMAND ${CMAKE_COMMAND} -DCREATE_SHARED=ON -P
                ${CMAKE_CURRENT_SOURCE_DIR}/test_helper.cm
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_zipping_test_file_shared
        COMMAND ${ZLIB_CONTRIB_PREFIX}minizip test_file_shared.zip test_file_shared.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_prepare_unzip_shared
        COMMAND ${CMAKE_COMMAND}
            -DMOVE_SHARED=ON
            -P
            ${CMAKE_CURRENT_SOURCE_DIR}/test_helper.cm
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_miniunzipping_test_file_shared
        COMMAND ${ZLIB_CONTRIB_PREFIX}miniunzip test_file_shared.zip
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_compare_shared
        COMMAND ${CMAKE_COMMAND} -E compare_files test_file_shared.txt
                test_file_shared.orig
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_clean_shared
        COMMAND ${CMAKE_COMMAND} -DDELETE_SHARED=ON -P
                ${CMAKE_CURRENT_SOURCE_DIR}/test_helper.cm
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_prepare_zip_shared
        PROPERTIES
            FIXTURES_SETUP "prepare_shared;mz_shared")
    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_zipping_test_file_shared
            PROPERTIES
                FIXTURES_REQUIRED "prepare_shared;mz_shared"
                FIXTURES_SETUP zip_shared)
    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_prepare_unzip_shared
        PROPERTIES
            FIXTURES_REQUIRED "zip_shared;mz_shared"
            FIXTURES_SETUP prepare_unzip_shared)
    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_miniunzipping_test_file_shared
            PROPERTIES
                FIXTURES_REQUIRED "prepare_unzip_shared;mz_shared"
                FIXTURES_SETUP prepare_compare_shared)
    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_compare_shared
        PROPERTIES
            FIXTURES_REQUIRED "prepare_compare_shared;mz_shared")
    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_clean_shared
        PROPERTIES
            FIXTURES_CLEANUP mz_shared)
endif(MINIZIP_BUILD_SHARED AND
      NOT ${CMAKE_SHARED_LIBRARY_SUFFIX} STREQUAL ".dll")

if(MINIZIP_BUILD_STATIC)
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_prepare_zip_static
        COMMAND ${CMAKE_COMMAND}
                -DCREATE_STATIC=ON
                -P
                ${CMAKE_CURRENT_SOURCE_DIR}/test_helper.cm
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_zipping_test_file_static
        COMMAND ${ZLIB_CONTRIB_PREFIX}minizipstatic
            test_file_static.zip
            test_file_static.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_prepare_unzip_static
        COMMAND ${CMAKE_COMMAND}
                -DMOVE_STATIC=ON
                -P
                ${CMAKE_CURRENT_SOURCE_DIR}/test_helper.cm
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME
            ${ZLIB_CONTRIB_PREFIX}minizip_miniunzipping_test_file_static
        COMMAND ${ZLIB_CONTRIB_PREFIX}miniunzipstatic
            test_file_static.zip
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_compare_static
        COMMAND ${CMAKE_COMMAND}
                -E compare_files
                test_file_static.txt
                test_file_static.orig
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ${ZLIB_CONTRIB_PREFIX}minizip_clean_static
        COMMAND ${CMAKE_COMMAND} -DDELETE_STATIC=ON -P
                ${CMAKE_CURRENT_SOURCE_DIR}/test_helper.cm
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_prepare_zip_static
            PROPERTIES
                FIXTURES_SETUP "prepare_zip_static;mz_static")
    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_zipping_test_file_static
            PROPERTIES
                FIXTURES_REQUIRED "prepare_zip_static;mz_static"
                FIXTURES_SETUP "create_zip_static")
    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_prepare_unzip_static
        PROPERTIES
            FIXTURES_REQUIRED "create_zip_static;mz_static"
            FIXTURES_SETUP "prepare_unzip_static")
    set_tests_properties(
        ${ZLIB_CONTRIB_PREFIX}minizip_miniunzipping_test_file_static
            PROPERTIES
                FIXTURES_REQUIRED "prepare_unzip_static;mz_static"
                FIXTURES_SETUP "prepare_compare_static")
    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_compare_static
        PROPERTIES
            FIXTURES_REQUIRED "prepare_compare_static;mz_static")
            #FIXTURES_SETUP "compared_static")
    set_tests_properties(${ZLIB_CONTRIB_PREFIX}minizip_clean_static
        PROPERTIES
            FIXTURES_CLEANUP mz_static)
endif(MINIZIP_BUILD_STATIC)
