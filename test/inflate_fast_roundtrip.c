/* inflate_fast_roundtrip.c -- check that inflate_fast decodes losslessly
 *
 * Compresses a mix of repetitive text and pseudo-random bytes at levels 1, 6
 * and 9, decompresses it, and checks the result matches the input byte for
 * byte. The input is large enough to keep inflate in its fast path, with many
 * length/distance pairs, extra bits and second-level Huffman codes.
 *
 * Returns 0 on success, 1 on failure.
 */

#include "zlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    static const char text[] =
        "the quick brown fox jumps over the lazy dog 0123456789 ";
    const size_t text_len = sizeof(text) - 1;
    const size_t text_reps = 4000;
    const size_t random_len = 120000;
    const size_t size = text_len * text_reps + random_len;
    static const int levels[] = {1, 6, 9};
    unsigned char *data, *comp, *dec;
    unsigned s = 0x1234567u;
    uLong bound;
    size_t i, pos = 0;
    int l, failed = 0;

    data = malloc(size);
    bound = compressBound((uLong)size);
    comp = malloc(bound);
    dec = malloc(size);
    if (data == NULL || comp == NULL || dec == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    /* Repetitive region: long matches with length/distance extra bits. */
    for (i = 0; i < text_reps; i++) {
        memcpy(data + pos, text, text_len);
        pos += text_len;
    }
    /* Higher-entropy region: more literals and second-level codes. */
    for (i = 0; i < random_len; i++) {
        s = s * 1664525u + 1013904223u;
        data[pos++] = (unsigned char)((s >> 16) & 0xff);
    }

    for (l = 0; l < (int)(sizeof(levels) / sizeof(levels[0])); l++) {
        uLongf clen = bound;
        uLongf dlen = (uLongf)size;
        int ret;

        ret = compress2(comp, &clen, data, (uLong)size, levels[l]);
        if (ret != Z_OK) {
            fprintf(stderr, "level %d: compress2 returned %d\n",
                    levels[l], ret);
            failed = 1;
            continue;
        }
        ret = uncompress(dec, &dlen, comp, clen);
        if (ret != Z_OK) {
            fprintf(stderr, "level %d: uncompress returned %d\n",
                    levels[l], ret);
            failed = 1;
            continue;
        }
        if (dlen != (uLongf)size || memcmp(data, dec, size) != 0) {
            fprintf(stderr, "level %d: round trip does not match\n",
                    levels[l]);
            failed = 1;
            continue;
        }
        printf("level %d: %lu -> %lu bytes, round trip OK\n", levels[l],
               (unsigned long)size, (unsigned long)clen);
    }

    free(data);
    free(comp);
    free(dec);
    return failed;
}
