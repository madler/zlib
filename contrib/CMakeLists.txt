option(ZLIB_BUILD_ADA "Enable building of Ada bindings" OFF)
option(ZLIB_BUILD_BLAST "Enable building of blast binary" OFF)
option(ZLIB_BUILD_MINIZIP "Enable building libminizip contrib library" OFF)
option(ZLIB_WITH_GVMAT64
       "Enable an optiomized longest_match for 32 bits x86_64"
       OFF)

if(ZLIB_BUILD_ADA)
    set(ZLIBADA_BUILD_SHARED ${ZLIB_BUILD_SHARED})
    set(ZLIBADA_BUILD_STATIC ${ZLIB_BUILD_STATIC})
    set(ZLIBADA_BUILD_TESTING ${ZLIB_BUILD_TESTING})
    add_subdirectory(ada/)
endif(ZLIB_BUILD_ADA)

if(ZLIB_BUILD_BLAST)
    set(ZLIBBLAST_BUILD_SHARED ${ZLIB_BUILD_SHARED})
    set(ZLIBBLAST_BUILD_STATIC ${ZLIB_BUILD_STATIC})
    set(ZLIBBLAST_BUILD_TESTING ${ZLIB_BUILD_TESTING})
    add_subdirectory(blast/)
endif(ZLIB_BUILD_BLAST)

if(ZLIB_BUILD_MINIZIP)
    set(MINIZIP_BUILD_SHARED ${ZLIB_BUILD_SHARED})
    set(MINIZIP_BUILD_STATIC ${ZLIB_BUILD_STATIC})
    set(MINIZIP_BUILD_TESTING ${ZLIB_BUILD_TESTING})
    add_subdirectory(minizip/)
endif(ZLIB_BUILD_MINIZIP)

if(ZLIB_WITH_GVMAT64)
   add_subdirectory(gcc_gvmat64/)
endif(ZLIB_WITH_GVMAT64)
