cmake_minimum_required(VERSION 3.15)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0048 NEW)
    cmake_policy(SET CMP0091 NEW)
endif()

### Why does this need to be MSVC only? ###
set(CMAKE_DEBUG_POSTFIX "d")

# parse the full version number from zlib.h and include in _ZLIB_FULL_VERSION
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/zlib.h" _ZLIB_H)
string(REGEX REPLACE "^.*#[ \t]*define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*$" "\\1" _ZLIB_FULL_VERSION ${_ZLIB_H})
unset(_ZLIB_H)

string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*$" "\\1" ZLIB_VERSION_MAJOR ${_ZLIB_FULL_VERSION})
string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+.*$" "\\1" ZLIB_VERSION_MINOR ${_ZLIB_FULL_VERSION})
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" ZLIB_VERSION_PATCH ${_ZLIB_FULL_VERSION})
set(ZLIB_VERSION_STRING "${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}.${ZLIB_VERSION_PATCH}")

set(ZLIB_VERSION_TWEAK "")
if(_ZLIB_FULL_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+).*$")
    set(ZLIB_VERSION_TWEAK ${CMAKE_MATCH_1})
    string(APPEND ZLIB_VERSION_STRING ".${ZLIB_VERSION_TWEAK}")
endif()

unset(_ZLIB_H)

message(STATUS "Configuring zlib version ${ZLIB_VERSION_STRING}")
project(zlib VERSION ${ZLIB_VERSION_STRING} LANGUAGES C)

option(ZLIB_NO_SHARED_LIBRARY "Disable building the shared version of zlib" OFF)
option(ZLIB_NO_STATIC_LIBRARY "Disable building the static version of zlib" OFF)
option(ZLIB_PREFER_STATIC_LIBRARY "Prefer linking against the static zlib library" OFF)

if(DEFINED BUILD_SHARED_LIBS)
    if(BUILD_SHARED_LIBS AND ZLIB_NO_SHARED_LIBRARY)
        message(WARNING "BUILD_SHARED_LIBS is enabled alongside ZLIB_NO_SHARED_LIBRARY")
        set(ZLIB_NO_SHARED_LIBRARY FALSE CACHE INTERNAL "IGNORED AND DISABLED!")
    elseif(NOT BUILD_SHARED_LIBS AND ZLIB_NO_STATIC_LIBRARY)
        message(WARNING "BUILD_SHARED_LIBS is disabled while ZLIB_NO_STATIC_LIBRARY is enabled")
        set(ZLIB_NO_STATIC_LIBRARY FALSE CACHE INTERNAL "IGNORED AND DISABLED!")

        if(NOT ZLIB_NO_SHARED_LIBRARY)
            set(ZLIB_PREFER_STATIC_LIBRARY TRUE CACHE INTERNAL "ALWAYS ENABLED!")
        endif()
    endif()
endif()

# End processing if both the shared and static version of zlib are disabled (I mean, who would do this realisiticly?)
if(ZLIB_NO_SHARED_LIBRARY AND ZLIB_NO_STATIC_LIBRARY)
    message(FATAL_ERROR "You must either build the shared or static version of zlib!")
endif()

# Ignore linking against the static version of zlib is the static version is disabled
if(ZLIB_PREFER_STATIC_LIBRARY AND ZLIB_NO_STATIC_LIBRARY)
    message(WARNING "Ignoring ZLIB_PREFER_STATIC_LIBRARY because the static zlib library is disabled")
    unset(ZLIB_PREFER_STATIC_LIBRARY)
endif()

# Enable the ability to use the static MSVC C runtime if targeting Windows.
if(WIN32)
    option(ZLIB_SHARED_USE_STATIC_MSVCRT "Link the shared zlib library against the static MSVC C runtime" OFF)
    option(ZLIB_STATIC_USE_STATIC_MSVCRT "Link the static zlib library against the static MSVC C runtime" OFF)
endif()

set(ZLIB_SHARED_NAME "" CACHE STRING "The name of zlib's shared library (Defaults to: z/zlib/zlib1)")
set(ZLIB_STATIC_NAME "" CACHE STRING "The name of zlib's static library (Defaults to: z/zlibstatic)'")

macro(_ZLIB_NORMALIZE_LIBRARY_NAME lib_link default_name)
    if("${ZLIB_${lib_lik}_NAME}" STREQUAL "")
        set(_ZLIB_${lib_link}_NAME "${default_name}" CACHE INTERNAL "")
    else()
         set(_ZLIB_${lib_link}_NAME "${ZLIB_${lib_lik}_NAME}" CACHE INTERNAL "")
    endif()
endmacro()

_ZLIB_NORMALIZE_LIBRARY_NAME(SHARED "z$<$<NOT:$<BOOL:${UNIX}>>:lib$<$<BOOL:${WIN32}>:1>>")
_ZLIB_NORMALIZE_LIBRARY_NAME(STATIC "z$<$<NOT:$<BOOL:${UNIX}>>:libstatic>")

option(ZLIB_USE_ZPREFIX "Build zlib with the \"Z_PREFIX\" macro" OFF)
option(ZLIB_USE_ZSOLO "Build zlib with the \"Z_SOLO\" macro" OFF)
option(ZLIB_USE_ZCONST "Build zlib with the \"ZLIB_CONST\" macro" OFF)
### Only allow the ZLIB_WINAPI macro on the Windows platform ###
if(WIN32)
    option(ZLIB_USE_WINAPI "Build zlib with the \"ZLIB_WINAPI\" macro" OFF)
endif()

include(GNUInstallDirs)
### Hack in GNUInstallDirs compatible variables ###
_GNUInstallDirs_cache_path_fallback(CMAKE_INSTALL_PKGCONFIGDIR "${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig" "pkgconfig (.pc) files")
_GNUInstallDirs_cache_path_fallback(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake" "CMake (.cmake) files")
mark_as_advanced(CMAKE_INSTALL_PKGCONFIGDIR CMAKE_INSTALL_CMAKEDIR)

GNUInstallDirs_get_absolute_install_dir(CMAKE_INSTALL_FULL_PKGCONFIGDIR CMAKE_INSTALL_PKGCONFIGDIR PKGCONFIG)
GNUInstallDirs_get_absolute_install_dir(CMAKE_INSTALL_FULL_CMAKEDIR CMAKE_INSTALL_CMAKEDIR CMAKEDIR)

include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckCSourceCompiles)
include(CTest)

check_include_file(stdarg.h      HAVE_STDARG_H)
### The follow include files are automatically searched for ###
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)
### Check for unistd.h ###
check_include_file(unistd.h HAVE_UNISTD_H)

### Check for large file support ###
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64)
check_type_size(off64_t OFF64_T)
list(REMOVE_ITEM CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64)

### Check for fseeko ###
check_function_exists(fseeko HAVE_FSEEKO)

# Check if compiler supports GCC's hidden attribute
check_c_source_compiles(
    "int __attribute__ (( visibility (\"hidden\") )) get_foo() { return 0; }
int main() { return get_foo(); }"
    HAVE_GCC_HIDDEN
)

macro(_ZLIB_MAP_CONFIG_OPTION option macro)
    set(${macro} ${${option}} CACHE INTERNAL "zlib C macro \"${macro}\"")
endmacro()

_ZLIB_MAP_CONFIG_OPTION(ZLIB_USE_ZPREFIX Z_PREFIX)
_ZLIB_MAP_CONFIG_OPTION(ZLIB_USE_ZSOLO Z_SOLO)
_ZLIB_MAP_CONFIG_OPTION(ZLIB_USE_ZCONST ZLIB_CONST)
if(WIN32)
    _ZLIB_MAP_CONFIG_OPTION(ZLIB_USE_WINAPI ZLIB_WINAPI)
endif()

### Configure a list of private compile definitions for zlib ###
list(APPEND _ZLIB_REQUIRED_DEFINITIONS
    $<$<NOT:$<BOOL:${HAVE_FSEEKO}>>:NO_FSEEKO>
    $<$<BOOL:${HAVE_GCC_HIDDEN}>:HAVE_HIDDEN>)

list(APPEND _ZLIB_REQUIRED_PUBLIC_DEFINITIONS
    $<$<BOOL:${HAVE_OFF64_T}>:_FILE_OFFSET_BITS=64 _LARGEFILE64_SOURCE=1>)

if(MSVC)
    list(APPEND _ZLIB_REQUIRED_DEFINITIONS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
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

set(_ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
configure_file(${PROJECT_SOURCE_DIR}/zlib.pc.cmakein ${_ZLIB_PC} @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/zconf.h.cmakein ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)

set(_ZLIB_PUBLIC_HDRS ${CMAKE_CURRENT_BINARY_DIR}/zconf.h zlib.h)
set(_ZLIB_PRIVATE_HDRS crc32.h deflate.h gzguts.h inffast.h inffixed.h inflate.h inftrees.h trees.h zutil.h)
set(_ZLIB_SRCS adler32.c crc32.c deflate.c infback.c inffast.c inflate.c inftrees.c trees.c zutil.c
    compress.c uncompr.c gzclose.c gzlib.c gzread.c gzwrite.c)

if(NOT ZLIB_NO_SHARED_LIBRARY)
    if(NOT MINGW)
        set(_ZLIB_DLL_SRCS win32/zlib1.rc)
        set_source_files_properties(win32/zlib1.rc PROPERTIES COMPILE_DEFINITIONS ZLIB_DLL_NAME="${_ZLIB_SHARED_NAME}")
    else()
        # This gets us DLL resource information when compiling on MinGW.
        if(NOT CMAKE_RC_COMPILER)
            find_program(CMAKE_RC_COMPILER windres.exe REQUIRED NO_DEFAULT_PATH NO_PACKAGE_ROOT_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
        endif()

        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
            COMMAND ${CMAKE_RC_COMPILER}
            -D GCC_WINDRES
            -D "ZLIB_DLL_NAME=\"${_ZLIB_SHARED_NAME}\""
            -I ${PROJECT_SOURCE_DIR}
            -I ${CMAKE_CURRENT_BINARY_DIR}
            -o ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
            -i ${PROJECT_SOURCE_DIR}/win32/zlib1.rc)
        set(_ZLIB_DLL_SRCS ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj)
    endif()
endif()

list(APPEND ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

macro(_ZLIB_CONFIGURE_TARGET_SHARED target)
    target_compile_definitions(${target} PRIVATE ${_ZLIB_REQUIRED_DEFINITIONS} PUBLIC ${_ZLIB_REQUIRED_PUBLIC_DEFINITIONS})
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>)
    list(APPEND _ZLIB_INSTALL_TARGETS ${target})
endmacro()

macro(_ZLIB_CREATE_SHARED_LIBRARY name)
    message(STATUS "Creating shared library ${name}")
    add_library(${name} SHARED ${_ZLIB_SRCS} ${_ZLIB_DLL_SRCS} ${_ZLIB_PUBLIC_HDRS} ${_ZLIB_PRIVATE_HDRS})
    target_compile_definitions(${name} PUBLIC ZLIB_DLL)
    set_target_properties(${name} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${ZLIB_SHARED_USE_STATIC_MSVCRT}>>:DLL>"
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME "${_ZLIB_SHARED_NAME}")
    _ZLIB_CONFIGURE_TARGET_SHARED(${name})
endmacro()

macro(_ZLIB_CREATE_STATIC_LIBRARY name)
    message(STATUS "Creating static library ${name}")
    add_library(${name} STATIC ${_ZLIB_SRCS} ${_ZLIB_PUBLIC_HDRS} ${_ZLIB_PRIVATE_HDRS})
    set_target_properties(${name} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${ZLIB_STATIC_USE_STATIC_MSVCRT}>>:DLL>"
        OUTPUT_NAME "${_ZLIB_STATIC_NAME}")
    _ZLIB_CONFIGURE_TARGET_SHARED(${name})
endmacro()

### While "BUILD_SHARED_LIBS" controls add_library, we'll go with the explicit way ###

if(BUILD_SHARED_LIBS OR NOT ZLIB_NO_SHARED_LIBRARY)
    _ZLIB_CREATE_SHARED_LIBRARY(zlib)
    if(NOT ZLIB_NO_STATIC_LIBRARY)
        _ZLIB_CREATE_STATIC_LIBRARY(zlibstatic)
    endif()

    list(APPEND ZLIB_LIBRARY "$<TARGET_FILE:zlib>" "$<TARGET_FILE:zlibstatic>")
    set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
else()
    _ZLIB_CREATE_STATIC_LIBRARY(zlib)

    list(APPEND ZLIB_LIBRARY "$<TARGET_FILE:zlib>")
    set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
endif()

if(NOT CYGWIN)
    # This property causes shared libraries on Linux to have the full version
    # encoded into their final filename.  We disable this on Cygwin because
    # it causes cygz-${PROJECT_VERSION}.dll to be created when cygz.dll
    # seems to be the default.
    #
    # This has no effect with MSVC, on that platform the version info for
    # the DLL comes from the resource file win32/zlib1.rc
    set_target_properties(zlib PROPERTIES VERSION ${PROJECT_VERSION})
    if(TARGET zlibstatic)
        set_target_properties(zlibstatic PROPERTIES VERSION ${PROJECT_VERSION})
    endif()
endif()

if(UNIX AND (BUILD_SHARED_LIBS OR NOT ZLIB_NO_SHARED_LIBRARY))
    if(NOT APPLE)
        if(NOT "${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")
            set_target_properties(zlib PROPERTIES LINK_FLAGS "-Wl,--version-script,\"${PROJECT_SOURCE_DIR}/zlib.map\"")
        endif()
        target_link_libraries(zlib -lc)
    endif()
endif()

if(ZLIB_PREFER_STATIC_LIBRARY AND TARGET zlibstatic)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
else()
    add_library(ZLIB::ZLIB ALIAS zlib)
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
    install(TARGETS ${_ZLIB_INSTALL_TARGETS}
        EXPORT ZLIBTarget
        RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
    install(EXPORT ZLIBTarget
        NAMESPACE ZLIB::
        DESTINATION ${CMAKE_INSTALL_FULL_CMAKEDIR}
        FILE ZLIBTargets.cmake)

    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/ZLIBConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
    configure_package_config_file(${PROJECT_SOURCE_DIR}/ZLIBConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/ZLIBConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_FULL_CMAKEDIR}
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ZLIBConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ZLIBConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_FULL_CMAKEDIR})
endif()

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
    install(FILES ${_ZLIB_PUBLIC_HDRS} DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
endif()

if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
    install(FILES zlib.3 DESTINATION "${CMAKE_INSTALL_FULL_MANDIR}/man3")
    install(FILES ${_ZLIB_PC} DESTINATION "${CMAKE_INSTALL_FULL_PKGCONFIGDIR}")

    if(MSVC)
        install(FILES "$<TARGET_PDB_FILE:zlib>" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
    endif()
endif()

unset(_ZLIB_INSTALL_TARGETS)
set(ZLIB_FOUND TRUE)

if(BUILD_TESTING)
    add_executable(example test/example.c)
    target_link_libraries(example ZLIB::ZLIB)
	target_compile_definitions(example PRIVATE ${_ZLIB_REQUIRED_DEFINITIONS})
    add_test(example example)

    add_executable(minigzip test/minigzip.c)
    target_link_libraries(minigzip ZLIB::ZLIB)
	target_compile_definitions(minigzip PRIVATE ${_ZLIB_REQUIRED_DEFINITIONS})

    if(HAVE_OFF64_T)
        add_executable(example64 test/example.c)
        target_link_libraries(example64 ZLIB::ZLIB)
		target_compile_definitions(example64 PRIVATE ${_ZLIB_REQUIRED_DEFINITIONS} -D_FILE_OFFSET_BITS=64)
        add_test(example64 example64)

        add_executable(minigzip64 test/minigzip.c)
        target_link_libraries(minigzip64 ZLIB::ZLIB)
		target_compile_definitions(minigzip64 PRIVATE ${_ZLIB_REQUIRED_DEFINITIONS} -D_FILE_OFFSET_BITS=64)
    endif()
endif()
