# Define the option to control static library build
option(BUILD_STATIC_LIBS "Build static libraries" ON)

cmake_minimum_required(VERSION 2.4.4...3.15.0)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(zlib C)

set(VERSION "1.3.0.1")

set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Installation directory for executables")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "Installation directory for libraries")
set(INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "Installation directory for headers")
set(INSTALL_MAN_DIR "${CMAKE_INSTALL_PREFIX}/share/man" CACHE PATH "Installation directory for manual pages")
set(INSTALL_PKGCONFIG_DIR "${CMAKE_INSTALL_PREFIX}/share/pkgconfig" CACHE PATH "Installation directory for pkgconfig (.pc) files")

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
enable_testing()

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)

# Check for large file support
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# Add other definitions for compatibility
if(HAVE_SYS_TYPES_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_TYPES_H)
endif()
if(HAVE_STDINT_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDINT_H)
endif()
if(HAVE_STDDEF_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDDEF_H)
endif()
check_type_size(off64_t OFF64_T)
if(HAVE_OFF64_T)
   add_definitions(-D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS)

# Check for fseeko
check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
    add_definitions(-DNO_FSEEKO)
endif()

# Check for unistd.h
check_include_file(unistd.h Z_HAVE_UNISTD_H)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    # If we're doing an out of source build and the user has a zconf.h
    # in their source tree...
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h)
        message(STATUS "Renaming")
        message(STATUS "    ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h")
        message(STATUS "to 'zconf.h.included' because this file is included with zlib")
        message(STATUS "but CMake generates it automatically in the build directory.")
        file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.included)
  endif()
endif()

set(ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zlib.pc.cmakein ${ZLIB_PC} @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})

# zlib sources and headers
set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
    zlib.h
)
set(ZLIB_PRIVATE_HDRS
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h
)
set(ZLIB_SRCS
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

if(NOT MINGW)
    set(ZLIB_DLL_SRCS
        win32/zlib1.rc
    )
endif()

if(BUILD_SHARED_LIBS)
    add_library(zlib SHARED ${ZLIB_SRCS} ${ZLIB_DLL_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
    set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
    set_target_properties(zlib PROPERTIES SOVERSION 1)
endif()

if(BUILD_STATIC_LIBS)
    add_library(zlibstatic STATIC ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
endif()

if(NOT CYGWIN)
    set_target_properties(zlib zlibstatic PROPERTIES OUTPUT_NAME z)
    if(NOT APPLE)
        set_target_properties(zlib PROPERTIES LINK_FLAGS "-Wl,--version-script,\"${CMAKE_CURRENT_SOURCE_DIR}/zlib.map\"")
    endif()
elseif(BUILD_SHARED_LIBS AND WIN32)
    set_target_properties(zlib PROPERTIES SUFFIX "1.dll")
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
    if(BUILD_SHARED_LIBS)
        install(TARGETS zlib
            RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
            LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
    endif()

    if(BUILD_STATIC_LIBS)
        install(TARGETS zlibstatic
            ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
    endif()
endif()

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
    install(FILES ${ZLIB_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")
endif()

if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
    install(FILES zlib.3 DESTINATION "${INSTALL_MAN_DIR}/man3")
endif()

if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
    install(FILES ${ZLIB_PC} DESTINATION "${INSTALL_PKGCONFIG_DIR}")
endif()

# Example binaries (conditionally built)
if(NOT SKIP_EXAMPLE)
    if(BUILD_SHARED_LIBS)
        add_executable(example test/example.c)
        target_link_libraries(example zlib)
        add_test(example example)

        add_executable(minigzip test/minigzip.c)
        target_link_libraries(minigzip zlib)
    endif()

    if(BUILD_STATIC_LIBS)
        add_executable(example_static test/example.c)
        target_link_libraries(example_static zlibstatic)
        add_test(example_static example_static)

        add_executable(minigzip_static test/minigzip.c)
        target_link_libraries(minigzip_static zlibstatic)
    endif()

    if(HAVE_OFF64_T)
        if(BUILD_SHARED_LIBS)
            add_executable(example64 test/example.c)
            target_link_libraries(example64 zlib)
            set_target_properties(example64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
            add_test(example64 example64)

            add_executable(minigzip64 test/minigzip.c)
            target_link_libraries(minigzip64 zlib)
            set_target_properties(minigzip64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
        endif()

        if(BUILD_STATIC_LIBS)
            add_executable(example64_static test/example.c)
            target_link_libraries(example64_static zlibstatic)
            set_target_properties(example64_static PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
            add_test(example64_static example64_static)

            add_executable(minigzip64_static test/minigzip.c)
            target_link_libraries(minigzip64_static zlibstatic)
            set_target_properties(minigzip64_static PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
        endif()
    endif()
endif()

