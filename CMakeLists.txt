cmake_minimum_required(VERSION 2.4.4)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(zlib C)

if(NOT DEFINED BUILD_SHARED_LIBS)
    option(BUILD_SHARED_LIBS "Build a shared library form of zlib" ON)
endif()

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
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE)

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
   add_definitions(-D_LARGEFILE64_SOURCE)
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
check_include_file(unistd.h HAVE_UNISTD_H)

#
# Check for errno.h
check_include_file(errno.h HAVE_ERRNO_H)
if(NOT HAVE_ERRNO_H)
   add_definitions(-DNO_ERRNO_H)
endif()

#
# Check for mmap support
#
set(mmap_test_code "
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
caddr_t hello() {
  return mmap((caddr_t)0, (off_t)0, PROT_READ, MAP_SHARED, 0, (off_t)0);
}
int main() { return 0; }
")
check_c_source_compiles("${mmap_test_code}" USE_MMAP)
if(USE_MMAP)
    add_definitions(-DUSE_MMAP)
endif()

#
# Create the zlibdefs.h file.
# Note: we create it in CMAKE_CURRENT_SOURCE_DIR instead 
#       of CMAKE_CURRENT_BINARY_DIR because an empty zlibdefs.h
#       is shipped with zlib in the source tree.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zlibdefs.h.cmakein
               ${CMAKE_CURRENT_SOURCE_DIR}/zlibdefs.h)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "D")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()

#============================================================================
# zlib
#============================================================================

set(ZLIB_PUBLIC_HDRS
    zconf.h
    zlib.h
    zlibdefs.h
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
    gzio.c
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

add_library(zlib ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
set_target_properties(zlib PROPERTIES VERSION 1.2.3.4)
set_target_properties(zlib PROPERTIES SOVERSION 1)
if(UNIX)
   # On unix like platforms the library is almost always called libz
   set_target_properties(zlib PROPERTIES OUTPUT_NAME z)
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
    install(TARGETS zlib
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib )
endif()
if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL )
    install(FILES ${ZLIB_PUBLIC_HDRS} DESTINATION include)
endif()
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
    install(FILES zlib.3 DESTINATION share/man/man3)
endif()

#============================================================================
# Example binaries
#============================================================================

add_executable(example example.c)
target_link_libraries(example zlib)
add_test(example example)

add_executable(minigzip minigzip.c)
target_link_libraries(minigzip zlib)

if(HAVE_OFF64_T)
    add_executable(example64 example.c)
    target_link_libraries(example64 zlib)
    set_target_properties(example64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
    add_test(example64 example64)

    add_executable(minigzip64 minigzip.c)
    target_link_libraries(minigzip64 zlib)
    set_target_properties(minigzip64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
endif()
