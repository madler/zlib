option(ZLIB_BUILD_MINIZIP "Enable building libminizip contrib library" OFF)

if(ZLIB_BUILD_MINIZIP)
    set(MINIZIP_BUILD_SHARED ${ZLIB_BUILD_SHARED})
    set(MINIZIP_BUILD_STATIC ${ZLIB_BUILD_STATIC})
    set(MINIZIP_BUILD_TESTING ${ZLIB_BUILD_TESTING})
    add_subdirectory(minizip/)
endif(ZLIB_BUILD_MINIZIP)
