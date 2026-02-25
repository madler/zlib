enable_language(ASM)

# Need to push CMAKE_ASM_COMPILE_OBJECT to cache as otherwise it's not
# available in top-level

set(CMAKE_ASM_COMPILE_OBJECT ${CMAKE_ASM_COMPILE_OBJECT} CACHE INTERNAL "" FORCE)

if(MSVC)
    #cmake 4.1 doesn't consider cl an asm compiler
    cmake_policy(SET CMP0194 NEW)
endif(MSVC)

add_library(gvmat64 OBJECT gvmat64.S)
set_target_properties(gvmat64 PROPERTIES LINKER_LANGUAGE C)

if(ZLIB_BUILD_SHARED)
    target_sources(zlib PRIVATE $<TARGET_OBJECTS:gvmat64>)
endif(ZLIB_BUILD_SHARED)

if(ZLIB_BUILD_STATIC)
    target_sources(zlibstatic PRIVATE $<TARGET_OBJECTS:gvmat64>)
endif(ZLIB_BUILD_STATIC)
