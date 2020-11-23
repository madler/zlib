#ifndef DFLTCC_DEFLATE_H
#define DFLTCC_DEFLATE_H

#include "dfltcc.h"

int ZLIB_INTERNAL dfltcc_can_deflate OF((z_streamp strm));
int ZLIB_INTERNAL dfltcc_deflate OF((z_streamp strm,
                                     int flush,
                                     block_state *result));
int ZLIB_INTERNAL dfltcc_deflate_params OF((z_streamp strm,
                                            int level,
                                            int strategy,
                                            int *flush));
int ZLIB_INTERNAL dfltcc_deflate_done OF((z_streamp strm, int flush));
int ZLIB_INTERNAL dfltcc_deflate_set_dictionary OF((z_streamp strm,
                                                    const Bytef *dictionary,
                                                    uInt dict_length));
int ZLIB_INTERNAL dfltcc_deflate_get_dictionary OF((z_streamp strm,
                                                    Bytef *dictionary,
                                                    uInt* dict_length));

#define DEFLATE_SET_DICTIONARY_HOOK(strm, dict, dict_len) \
    do { \
        if (dfltcc_can_deflate((strm))) \
            return dfltcc_deflate_set_dictionary((strm), (dict), (dict_len)); \
    } while (0)
#define DEFLATE_GET_DICTIONARY_HOOK(strm, dict, dict_len) \
    do { \
        if (dfltcc_can_deflate((strm))) \
            return dfltcc_deflate_get_dictionary((strm), (dict), (dict_len)); \
    } while (0)
#define DEFLATE_RESET_KEEP_HOOK(strm) \
    dfltcc_reset((strm), sizeof(deflate_state))
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
#define DEFLATE_DONE dfltcc_deflate_done
#define DEFLATE_BOUND_ADJUST_COMPLEN(strm, complen, source_len) \
    do { \
        if (dfltcc_can_deflate((strm))) \
            (complen) = (3 + 5 + 5 + 4 + 19 * 3 + (286 + 30) * 7 + \
                         (source_len) * 16 + 15 + 7) >> 3; \
    } while (0)
#define DEFLATE_NEED_CONSERVATIVE_BOUND(strm) (dfltcc_can_deflate((strm)))
#define DEFLATE_HOOK dfltcc_deflate
#define DEFLATE_NEED_CHECKSUM(strm) (!dfltcc_can_deflate((strm)))

#endif
