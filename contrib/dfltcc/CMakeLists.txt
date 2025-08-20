# check if we compile for IBM s390x
#
CHECK_C_SOURCE_COMPILES("
#ifndef __s390x__
    #error
#endif
int main() {return 0;}
" HAS_S390X_SUPPORT)

#
# Check for IBM S390X - DFLTCC extensions
#
if(ZLIB_WITH_DFLTCC AND HAS_S390X_SUPPORT)
    # check if we have static_assert
    check_c_source_compiles("
        #include <assert.h>
        static_assert(1==1,\"true\");
        " HAS_STATIC_ASSERT)

    # check if we have secure_getenv
    set(CMAKE_REQUIRED_FLAGS -D_GNU_SOURCE=1)
    check_c_source_compiles("
        #include <stdlib.h>
        int main() { char* _foo = secure_getenv(\"PWD\");return 0; }
        " HAS_SECURE_GETENV)
    unset(CMAKE_REQUIRED_FLAGS)

    # check for specific headers
    check_include_file(sys/sdt.h HAS_SYS_SDT_H)
    
    set(definitions "-DHAVE_S390X_DFLTCC=1")
    if(HAS_STATIC_ASSERT)
        list(APPEND definitions "-DHAVE_STATIC_ASSERT=1")
    endif()
    if(HAS_SECURE_GETENV)
        list(APPEND definitions "-DHAVE_SECURE_GETENV=1")
        list(APPEND definitions "-D_GNU_SOURCE=1")
    endif()
    if(HAS_SYS_SDT_H)
        list(APPEND definitions "-DHAVE_SYS_SDT_H=1")
    endif()

    # prepare compiling for s390x
    if(ZLIB_BUILD_SHARED)
        target_sources(zlib
            PRIVATE
                dfltcc.c
                dfltcc_hooks.h
                dfltcc_common.h
                hooks.h)
        target_compile_definitions(zlib PUBLIC ${definitions})
    endif(ZLIB_BUILD_SHARED)
    if(ZLIB_BUILD_STATIC)
        target_sources(zlibstatic
            PRIVATE
                dfltcc.c
                dfltcc_hooks.h
                dfltcc_common.h
                hooks.h)
        target_compile_definitions(zlibstatic PUBLIC ${definitions})
    endif(ZLIB_BUILD_STATIC)

endif(ZLIB_WITH_DFLTCC AND HAS_S390X_SUPPORT)
