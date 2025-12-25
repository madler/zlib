option(ZLIB_BUILD_MINIZIP "Enable building libminizip contrib library" OFF)

if(ZLIB_BUILD_MINIZIP)
    add_subdirectory(minizip/)
endif(ZLIB_BUILD_MINIZIP)
