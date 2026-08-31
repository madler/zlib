#ifndef DFLTCC_HOOKS_H
#define DFLTCC_HOOKS_H

#include "dfltcc.h"

/**
 * DEFLATE HOOKS
 */
#define DEFLATE_BOUND_ADJUST_COMPLEN(strm, complen, source_len) \
    do { \
        if (deflateStateCheck((strm)) || dfltcc_can_deflate((strm))) \
            (complen) = DEFLATE_BOUND_COMPLEN(source_len); \
    } while (0)

#define DEFLATE_BOUND_COMPLEN(source_len) \
    ((DFLTCC_BLOCK_HEADER_BITS + \
      DFLTCC_HLITS_COUNT_BITS + \
      DFLTCC_HDISTS_COUNT_BITS + \
      DFLTCC_HCLENS_COUNT_BITS + \
      DFLTCC_MAX_HCLENS * DFLTCC_HCLEN_BITS + \
      (DFLTCC_MAX_HLITS + DFLTCC_MAX_HDISTS) * DFLTCC_MAX_HLIT_HDIST_BITS + \
      (source_len) * DFLTCC_MAX_SYMBOL_BITS + \
      DFLTCC_MAX_EOBS_BITS + \
      DFLTCC_MAX_PADDING_BITS) >> 3)

#define DEFLATE_DONE dfltcc_deflate_done

#define DEFLATE_GET_DICTIONARY_HOOK(strm, dict, dict_len) \
    do { \
        if (dfltcc_can_deflate((strm))) \
            return dfltcc_deflate_get_dictionary((strm), (dict), (dict_len)); \
    } while (0)

#define DEFLATE_HOOK dfltcc_deflate

#define DEFLATE_NEED_CHECKSUM(strm) (!dfltcc_can_deflate((strm)))

#define DEFLATE_NEED_CONSERVATIVE_BOUND(strm) (dfltcc_can_deflate((strm)))

#define DEFLATE_PARAMS_HOOK(strm, level, strategy, hook_flush) \
    do { \
        int err; \
\
        err = dfltcc_deflate_params((strm), \
                                    (level), \
                                    (strategy), \
                                    (hook_flush)); \
        if (err == Z_STREAM_ERROR) \
            return err; \
    } while (0)

#define DEFLATE_RESET_KEEP_HOOK(strm) \
    dfltcc_reset((strm), sizeof(deflate_state))

#define DEFLATE_SET_DICTIONARY_HOOK(strm, dict, dict_len) \
    do { \
        if (dfltcc_can_deflate((strm))) \
            return dfltcc_deflate_set_dictionary((strm), (dict), (dict_len)); \
    } while (0)


/**
 * INFLATE HOOKS
 */
#define INFLATE_GET_DICTIONARY_HOOK(strm, dict, dict_len) \
    do { \
        if (dfltcc_can_inflate(strm)) \
            return dfltcc_inflate_get_dictionary(strm, dict, dict_len); \
    } while (0)

#define INFLATE_MARK_HOOK(strm) \
    do { \
        if (dfltcc_was_inflate_used((strm))) return -(1L << 16); \
    } while (0)

#define INFLATE_NEED_CHECKSUM(strm) (!dfltcc_can_inflate((strm)))

#define INFLATE_NEED_UPDATEWINDOW(strm) (!dfltcc_can_inflate((strm)))

#define INFLATE_PRIME_HOOK(strm, bits, value) \
    do { if (dfltcc_inflate_disable((strm))) return Z_STREAM_ERROR; } while (0)

#define INFLATE_RESET_KEEP_HOOK(strm) \
    dfltcc_reset((strm), sizeof(struct inflate_state))

#define INFLATE_SET_DICTIONARY_HOOK(strm, dict, dict_len) \
    do { \
        if (dfltcc_can_inflate(strm)) \
            return dfltcc_inflate_set_dictionary(strm, dict, dict_len); \
    } while (0)

#define INFLATE_SYNC_POINT_HOOK(strm) \
    do { \
        if (dfltcc_was_inflate_used((strm))) return Z_STREAM_ERROR; \
    } while (0)

#define INFLATE_TYPEDO_HOOK(strm, flush) \
    if (dfltcc_can_inflate((strm))) { \
        dfltcc_inflate_action action; \
        \
        RESTORE(); \
        action = dfltcc_inflate((strm), (flush), &ret); \
        LOAD(); \
        if (action == DFLTCC_INFLATE_CONTINUE) \
            break; \
        else if (action == DFLTCC_INFLATE_BREAK) \
            goto inf_leave; \
    }


/**
 * MEMORY HOOKS
 */
#define TRY_FREE_WINDOW dfltcc_free_window
#define ZALLOC_STATE dfltcc_alloc_state
#define ZALLOC_WINDOW dfltcc_alloc_window
#define ZCOPY_STATE dfltcc_copy_state
#define ZCOPY_WINDOW dfltcc_copy_window
#define ZFREE_STATE ZFREE
#define ZFREE_WINDOW dfltcc_free_window

#endif /* DFLTCC_HOOKS_H */
