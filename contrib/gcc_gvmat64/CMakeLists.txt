enable_language(ASM)

# Need to push CMAKE_ASM_COMPILE_OBJECT to cache as otherwise it's not
# available in top-level

set(CMAKE_ASM_COMPILE_OBJECT ${CMAKE_ASM_COMPILE_OBJECT} CACHE INTERNAL "" FORCE)

if(ZLIB_BUILD_SHARED)
    target_sources(zlib PRIVATE gvmat64.S)
endif(ZLIB_BUILD_SHARED)

if(ZLIB_BUILD_STATIC)
    target_sources(zlibstatic PRIVATE gvmat64.S)
endif(ZLIB_BUILD_STATIC)
