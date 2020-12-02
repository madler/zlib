cmake_minimum_required(VERSION 2.8.12)

project(zlib C)

include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckFunctionExists)

# Check include files
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)
check_include_file(unistd.h    HAVE_UNISTD_H)

# Build with large file support
add_definitions(-D_LARGEFILE64_SOURCE=1)

# Build type setting
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS  "Debug;Release;RelWithDebInfo;MinSizeRel")
endif()

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(SKIP_CPUID_CHECK "Assume CPU supports fast CRC" OFF)

if(SKIP_CPUID_CHECK)
    add_definitions(-DSKIP_CPUID_CHECK)
endif()

# Set -fPIC option
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# parse the full version number from zlib.h and include in ZLIB_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" ZLIB_VERSION ${_zlib_h_contents})

# Generate zlib.pc
set(ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/zlib.pc.cmakein
        ${ZLIB_PC} @ONLY)

# Generate zcon.h
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})

# Mark OSX settings as advanced
if(APPLE)
    mark_as_advanced(CMAKE_OSX_ARCHITECTURES CMAKE_OSX_DEPLOYMENT_TARGET CMAKE_OSX_SYSROOT)
    set(CMAKE_MACOSX_RPATH TRUE)
endif()

# Option to use static runtime
include(ucm.cmake)
option(USE_STATIC_RUNTIME "Use static runtime" ON)
if(USE_STATIC_RUNTIME)
    ucm_set_runtime(STATIC)
else()
    ucm_set_runtime(DYNAMIC)
endif()

# Compiler dependent flags
include (CheckCCompilerFlag)
if(UNIX OR MINGW)
    check_c_compiler_flag(-march=armv8-a+crc ARM_CRC)
    if(ARM_CRC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crc")
    else()
        check_c_compiler_flag(-msse2 HAS_SSE2)
        if(HAS_SSE2)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
            add_definitions(-DHAS_SSE2)
        endif()

        check_c_compiler_flag(-mssse3 HAS_SSSE3)
        if(HAS_SSSE3)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mssse3")
            add_definitions(-DHAS_SSSE3)
        endif()

        check_c_compiler_flag(-msse4.2 HAS_SSE42)
        if(HAS_SSE42)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2")
            add_definitions(-DHAS_SSE42)
        endif()

        check_c_compiler_flag(-mpclmul HAS_PCLMUL)
        if(HAS_PCLMUL)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mpclmul")
            add_definitions(-DHAS_PCLMUL)
        endif()
    endif()
elseif(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)

    check_c_compiler_flag(/arch:AVX HAS_AVX)
    if (HAS_AVX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX")
    endif()
endif()

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

if(UNIX OR MINGW)
    # append "inffast_chunk.c" and "adler32_simd.c" for ARMv8 CPU
    if(ARM_CRC)
        list(APPEND ZLIB_SRCS inffast_chunk.c adler32_simd.c)
        add_definitions(-DINFLATE_CHUNK_SIMD_NEON)
        add_definitions(-DINFLATE_CHUNK_READ_64LE)
        add_definitions(-DADLER32_SIMD_NEON)
    endif()

    # append "inffast_chunk.c" and compile with "sse2" if supported by compiler
    if(HAS_SSE2)
        list(APPEND ZLIB_SRCS inffast_chunk.c)
        add_definitions(-DINFLATE_CHUNK_SIMD_SSE2)
        add_definitions(-DINFLATE_CHUNK_READ_64LE)
    endif()

    # append "adler32_simd.c" and compile with "ssse3" if supported by compiler
    if(HAS_SSSE3)
        list(APPEND ZLIB_SRCS adler32_simd.c)
        add_definitions(-DADLER32_SIMD_SSSE3)
    endif()

    # append "crc_simd.c" and compile with "pclmul" if supported by compiler
    if(HAS_PCLMUL)
        list(APPEND ZLIB_SRCS crc32_simd.c)
    endif()
endif()

if(BUILD_SHARED_LIBS)
    # Visibility
    if(UNIX AND NOT CYGWIN)
        check_c_compiler_flag(-fvisibility=hidden HAVE_HIDDEN)
        if(HAVE_HIDDEN)
            add_definitions(-DHAVE_HIDDEN)
        endif()
    endif()

    # DLL resource setting
    if(MSVC)
        set(ZLIB_DLL_SRCS
            win32/zlib1.rc # If present will override custom build rule below.
        )
    elseif(MINGW OR CYGWIN)
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

    add_library(zlib SHARED ${ZLIB_SRCS} ${ZLIB_DLL_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
    set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)

    if(NOT CYGWIN)
        # This property causes shared libraries on Linux to have the full version
        # encoded into their final filename.  We disable this on Cygwin because
        # it causes cygz-${ZLIB_VERSION}.dll to be created when cygz.dll
        # seems to be the default.
        #
        # This has no effect with MSVC, on that platform the version info for
        # the DLL comes from the resource file win32/zlib1.rc
        set_target_properties(zlib PROPERTIES SOVERSION 1)
        set_target_properties(zlib PROPERTIES VERSION ${ZLIB_VERSION})
    endif()

    if(UNIX OR MINGW)
        # On unix-like platforms the library is almost always called libz
        set_target_properties(zlib PROPERTIES OUTPUT_NAME z)
        if(NOT APPLE)
            set_target_properties(zlib PROPERTIES LINK_FLAGS "-Wl,--version-script,\"${CMAKE_CURRENT_SOURCE_DIR}/zlib.map\"")
        endif()
    elseif(WIN32)
        # Creates zlib1.dll when building shared library version
        set_target_properties(zlib PROPERTIES SUFFIX "1.dll")
    endif()
else()
    add_library(zlib STATIC ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
    if(UNIX OR MINGW)
        set_target_properties(zlib PROPERTIES OUTPUT_NAME z)
    endif()
    #============================================================================
    # work around to CMake bug which affects 64-bit Windows
    # see http://public.kitware.com/Bug/view.php?id=11240
    #============================================================================
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND MSVC)
        set_target_properties(zlib PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
    endif()
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
    install(TARGETS zlib
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
endif()

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
    install(FILES ${ZLIB_PUBLIC_HDRS} DESTINATION include)
endif()

if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
    install(FILES zlib.3 DESTINATION share/man/man3)
    install(FILES ${ZLIB_PC} DESTINATION lib/pkgconfig)
endif()

#============================================================================
# Example binaries
#============================================================================

if(BUILD_EXAMPLES)
    add_executable(example test/example.c)
    target_link_libraries(example zlib)
    set_target_properties(example PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
    add_test(example example)

    add_executable(minigzip test/minigzip.c)
    target_link_libraries(minigzip zlib)
    set_target_properties(minigzip PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
endif()
