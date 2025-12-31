function(zlib_add_contrib_lib name description dir)
    option(ZLIB_BUILD_${name} "Enable building of ${description}" OFF)

    if(ZLIB_BUILD_${name})
        if(NOT DEFINED ZLIB_${name}_BUILD_SHARED)
            set(ZLIB_${name}_BUILD_SHARED ${ZLIB_BUILD_SHARED})
        endif(NOT DEFINED ZLIB_${name}_BUILD_SHARED)

        if(NOT DEFINED ZLIB_${name}_BUILD_STATIC)
            set(ZLIB_${name}_BUILD_STATIC ${ZLIB_BUILD_STATIC})
        endif(NOT DEFINED ZLIB_${name}_BUILD_STATIC)

        if(NOT DEFINED ZLIB_${name}_BUILD_TESTING)
            set(ZLIB_${name}_BUILD_TESTING ${ZLIB_BUILD_TESTING})
        endif(NOT DEFINED ZLIB_${name}_BUILD_TESTING)

        add_subdirectory(${dir}/)
    endif(ZLIB_BUILD_${name})
endfunction(zlib_add_contrib_lib name description dir)

function(zlib_add_contrib_feature name description dir)
    option(ZLIB_WITH_${name}
           "Enable build ${description}"
           OFF)

    if(ZLIB_WITH_${name})
        add_subdirectory(${dir}/)
    endif(ZLIB_WITH_${name})
endfunction(zlib_add_contrib_feature name description dir)

zlib_add_contrib_feature("GVMAT64"
    "of an optimized longest_match for 32 bits x86_64"
    gcc_gvmat64)

zlib_add_contrib_feature(INFBACK9 "with support for method 9 deflate" infback9)
zlib_add_contrib_lib(ADA "Ada bindings" ada)
zlib_add_contrib_lib(BLAST "blast binary" blast)
zlib_add_contrib_lib(IOSTREAM3 "IOStream C++ bindings V3" iostream3)
zlib_add_contrib_lib(MINIZIP "minizip library" minizip)
