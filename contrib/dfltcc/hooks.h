/* contrib/dfltcc/hooks.h -- architecture-specific hook macro defaults for zlib
 * When HAVE_S390X_DFLTCC is defined, the real implementations are provided
 * by contrib/dfltcc/dfltcc_hooks.h instead.  This file only defines the
 * no-op defaults so that the core source files need not include any
 * contrib header unconditionally.
 */

#ifndef ZLIB_CONTRIB_HOOKS_H
#define ZLIB_CONTRIB_HOOKS_H

#ifndef HAVE_S390X_DFLTCC

/* Deflate hooks -- no-ops that restore original behavior */
#define DEFLATE_BOUND_ADJUST_COMPLEN(strm, complen, sourceLen) do {} while (0)
#define DEFLATE_BOUND_COMPLEN(source_len) 0
#define DEFLATE_DONE(strm, flush) 1
#define DEFLATE_GET_DICTIONARY_HOOK(strm, dict, dict_len) do {} while (0)
#define DEFLATE_HOOK(strm, flush, bstate) 0
#define DEFLATE_NEED_CHECKSUM(strm) 1
#define DEFLATE_NEED_CONSERVATIVE_BOUND(strm) 0
#define DEFLATE_PARAMS_HOOK(strm, level, strategy, hook_flush) do {} while (0)
#define DEFLATE_RESET_KEEP_HOOK(strm) do {} while (0)
#define DEFLATE_SET_DICTIONARY_HOOK(strm, dict, dict_len) do {} while (0)

/* Inflate hooks -- no-ops that restore original behavior */
#define INFLATE_GET_DICTIONARY_HOOK(strm, dict, dict_len) do {} while (0)
#define INFLATE_MARK_HOOK(strm) do {} while (0)
#define INFLATE_NEED_CHECKSUM(strm) 1
#define INFLATE_NEED_UPDATEWINDOW(strm) 1
#define INFLATE_PRIME_HOOK(strm, bits, value) do {} while (0)
#define INFLATE_RESET_KEEP_HOOK(strm) do {} while (0)
#define INFLATE_SET_DICTIONARY_HOOK(strm, dict, dict_len) do {} while (0)
#define INFLATE_SYNC_POINT_HOOK(strm) do {} while (0)
#define INFLATE_TYPEDO_HOOK(strm, flush) do {} while (0)

/* Memory hooks -- map to the original allocator calls */
#define TRY_FREE_WINDOW TRY_FREE
#define ZALLOC_STATE ZALLOC
#define ZALLOC_WINDOW ZALLOC
#define ZCOPY_STATE zmemcpy
#define ZCOPY_WINDOW zmemcpy
#define ZFREE_STATE ZFREE
#define ZFREE_WINDOW ZFREE

#endif /* !HAVE_S390X_DFLTCC */

#endif /* ZLIB_CONTRIB_HOOKS_H */
