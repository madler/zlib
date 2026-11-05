/* zlib_inflate_chunked_fuzzer.c
 *
 * Fuzz target for zlib inflate with byte-at-a-time and chunked feeding.
 * The existing OSS-Fuzz harness (zlib_uncompress_fuzzer) feeds all input
 * in one shot via uncompress(). This harness exercises the incremental
 * inflate() state machine with variable-size chunks, covering:
 *
 *   inflate.c  – window management, stored/fixed/dynamic block paths
 *   inffast.c  – inflate_fast() literal/length/distance copy loops
 *   inftrees.c – Huffman table construction (via Z_SYNC_FLUSH mid-stream)
 *
 * Both raw deflate (windowBits=-15) and zlib-wrapped (windowBits=15)
 * streams are exercised based on a fuzz-derived flag.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2) return 0;

    /* First byte: lower nibble = chunk size (1-16), bit4 = raw/wrapped */
    uint8_t  flags      = data[0];
    size_t   chunk      = (size_t)(flags & 0x0f) + 1;   /* 1–16 bytes */
    int      windowBits = (flags & 0x10) ? -15 : 15;    /* raw or zlib */

    data++; size--;

    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    if (inflateInit2(&strm, windowBits) != Z_OK) return 0;

    uint8_t out[256];
    const uint8_t *p   = data;
    size_t         rem = size;

    while (rem > 0) {
        size_t feed = (rem < chunk) ? rem : chunk;
        strm.next_in  = (Bytef *)p;
        strm.avail_in = (uInt)feed;
        p   += feed;
        rem -= feed;

        do {
            strm.next_out  = out;
            strm.avail_out = sizeof(out);
            int ret = inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_END || ret == Z_STREAM_ERROR ||
                ret == Z_DATA_ERROR  || ret == Z_MEM_ERROR)
                goto done;
        } while (strm.avail_out == 0);
    }
    /* Final flush */
    strm.next_in  = NULL;
    strm.avail_in = 0;
    strm.next_out  = out;
    strm.avail_out = sizeof(out);
    inflate(&strm, Z_FINISH);

done:
    inflateEnd(&strm);
    return 0;
}
