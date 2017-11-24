cmake_minimum_required(VERSION 2.4.4)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(zlib C)

set(VERSION "1.2.11")

option(ASM686 "Enable building i686 assembly implementation")
option(AMD64 "Enable building amd64 assembly implementation")
option(BUILD_SHARED_LIBS "Build shared (dynamic) libraries instead of static" ON)
option(BUILD_STATIC_AND_SHARED "Build both static and shared libraries in the same build" ON)
option(SKIP_INSTALL_ALL "Skip installation of everything")
option(SKIP_INSTALL_FILES "Skip installation of manual pages and pkgconfig files")
option(SKIP_INSTALL_HEADERS "Skip installation of headers")
option(SKIP_INSTALL_LIBRARIES "Skip installation of binaries, libraries, and symbol files")

if(BUILD_STATIC_AND_SHARED)
    set(BUILD_SHARED_LIBS ON)
endif()

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

#
# Check to see if we have large file support
#
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
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
set(CMAKE_REQUIRED_DEFINITIONS) # clear variable

#
# Check for fseeko
#
check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
    add_definitions(-DNO_FSEEKO)
endif()

#
# Check for unistd.h
#
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
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/zlib.pc.cmakein
        ${ZLIB_PC} @ONLY)
configure_file(    ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})


#============================================================================
# zlib
#============================================================================

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

if(BUILD_SHARED_LIBS AND NOT MINGW)
    set(ZLIB_DLL_SRCS
        win32/zlib1.rc # If present will override custom build rule below.
    )
endif()

if(CMAKE_COMPILER_IS_GNUCC)
    if(ASM686)
        set(ZLIB_ASMS contrib/asm686/match.S)
    elseif (AMD64)
        set(ZLIB_ASMS contrib/amd64/amd64-match.S)
    endif ()

    if(ZLIB_ASMS)
        add_definitions(-DASMV)
        set_source_files_properties(${ZLIB_ASMS} PROPERTIES LANGUAGE C COMPILE_FLAGS -DNO_UNDERLINE)
    endif()
endif()

if(MSVC)
    if(ASM686)
        ENABLE_LANGUAGE(ASM_MASM)
        set(ZLIB_ASMS
            contrib/masmx86/inffas32.asm
            contrib/masmx86/match686.asm
        )
    elseif (AMD64)
        ENABLE_LANGUAGE(ASM_MASM)
        set(ZLIB_ASMS
            contrib/masmx64/gvmat64.asm
            contrib/masmx64/inffasx64.asm
        )
    endif()

    if(ZLIB_ASMS)
        add_definitions(-DASMV -DASMINF)
    endif()
endif()

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})

if(BUILD_SHARED_LIBS AND MINGW)
    # This gets us DLL resource information when compiling on MinGW.
    if(NOT CMAKE_RC_COMPILER)
        set(CMAKE_RC_COMPILER windres.exe)
    endif()

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
                       COMMAND ${CMAKE_RC_COMPILER}
                            -D GCC_WINDRES
                            -I ${CMAKE_CURRENT_SOURCE_DIR}
                            -I ${CMAKE_CURRENT_BINARY_DIR}
                            -o ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
                            -i ${CMAKE_CURRENT_SOURCE_DIR}/win32/zlib1.rc)
    set(ZLIB_DLL_SRCS ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj)
endif()

if(BUILD_STATIC_AND_SHARED)
    # This preserves the original behavior of the zlib cmake build, however combining shared and static
    # builds does not afford customization of compiler options typically used on some platforms for each type
    add_library(zlib       SHARED ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_DLL_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
    add_library(zlibstatic STATIC ${ZLIB_SRCS} ${ZLIB_ASMS}                  ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
else()
    # Use BUILD_SHARED_LIBS to control whether the result is shared or static like any other typical cmake build
    add_library(zlib              ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_DLL_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
endif()

if(UNIX)
    # On unix-like platforms the library is almost always called libz
    set_target_properties(zlib PROPERTIES OUTPUT_NAME z)
    if(BUILD_STATIC_AND_SHARED)
        set_target_properties(zlibstatic PROPERTIES OUTPUT_NAME z)
    endif()
endif()

if(BUILD_SHARED_LIBS)
    message(STATUS "Building shared (dynamic) library")
    set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
    set_target_properties(zlib PROPERTIES SOVERSION 1)

    if(NOT CYGWIN)
        # This property causes shared libraries on Linux to have the full version
        # encoded into their final filename.  We disable this on Cygwin because
        # it causes cygz-${ZLIB_FULL_VERSION}.dll to be created when cygz.dll
        # seems to be the default.
        #
        # This has no effect with MSVC, on that platform the version info for
        # the DLL comes from the resource file win32/zlib1.rc
        set_target_properties(zlib PROPERTIES VERSION ${ZLIB_FULL_VERSION})
    endif()

    if (UNIX AND NOT APPLE)
        set_target_properties(zlib PROPERTIES LINK_FLAGS "-Wl,--version-script,\"${CMAKE_CURRENT_SOURCE_DIR}/zlib.map\"")
    endif()

    if(WIN32)
        # Creates zlib1.dll when building shared library version
        set_target_properties(zlib PROPERTIES SUFFIX "1.dll")
    endif()
endif()

if(BUILD_STATIC_AND_SHARED OR NOT BUILD_SHARED_LIBS)
    message(STATUS "Building static library")
endif()

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
    install(FILES ${ZLIB_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")
endif()
if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
    install(TARGETS zlib
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" )
    if(BUILD_STATIC_AND_SHARED)
        install(TARGETS zlibstatic
            RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
            ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
            LIBRARY DESTINATION "${INSTALL_LIB_DIR}" )
    endif()

    # This entire block is just for handling the PDB file properly!
    # It is all OPTIONAL because there is no way to test and see if 
    # "/Zi" or "/ZI" was used in the build process.
    if(MSVC)
        if(BUILD_SHARED_LIBS)
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Debug/zlibd.pdb"
                CONFIGURATIONS Debug
                DESTINATION "${INSTALL_BIN_DIR}"
                RENAME "zlibd1.pdb"
                OPTIONAL)
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/zlib.pdb"
                CONFIGURATIONS RelWithDebInfo
                DESTINATION "${INSTALL_BIN_DIR}"
                RENAME "zlib1.pdb"
                OPTIONAL)
        endif()
        if(BUILD_STATIC_AND_SHARED)
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zlibstatic.dir/Debug/zlibstatic.pdb"
                CONFIGURATIONS Debug
                DESTINATION "${INSTALL_LIB_DIR}"
                RENAME "zlibstaticd.pdb"
                OPTIONAL)
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zlibstatic.dir/RelWithDebInfo/zlibstatic.pdb"
                CONFIGURATIONS RelWithDebInfo
                DESTINATION "${INSTALL_LIB_DIR}"
                OPTIONAL)
        elseif(NOT BUILD_SHARED_LIBS)
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zlib.dir/Debug/zlib.pdb"
                CONFIGURATIONS Debug
                DESTINATION "${INSTALL_LIB_DIR}"
                RENAME "zlibd.pdb"
                OPTIONAL)
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zlib.dir/RelWithDebInfo/zlib.pdb"
                CONFIGURATIONS RelWithDebInfo
                DESTINATION "${INSTALL_LIB_DIR}"
                OPTIONAL)
        endif()
    endif()
endif()
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
    install(FILES zlib.3 DESTINATION "${INSTALL_MAN_DIR}/man3")
    install(FILES ${ZLIB_PC} DESTINATION "${INSTALL_PKGCONFIG_DIR}")
endif()

#============================================================================
# Example binaries
#============================================================================

add_executable(example test/example.c)
target_link_libraries(example zlib)
add_test(example example)

add_executable(minigzip test/minigzip.c)
target_link_libraries(minigzip zlib)

if(HAVE_OFF64_T)
    add_executable(example64 test/example.c)
    target_link_libraries(example64 zlib)
    set_target_properties(example64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
    add_test(example64 example64)

    add_executable(minigzip64 test/minigzip.c)
    target_link_libraries(minigzip64 zlib)
    set_target_properties(minigzip64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
endif()
