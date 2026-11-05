/* Bit-exact CRC32 validation: zlib crc32() vs byte table reference.
 * Run after building pl_zlib with CRC32_SIMD_SSE42_PCLMUL (avx2/x86_64).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"

static unsigned long crc32_byte_ref(unsigned long crc,
                                    const unsigned char *buf,
                                    z_size_t len) {
    const z_crc_t FAR *tbl = get_crc_table();
    unsigned long c = crc ^ 0xffffffffUL;
    while (len--) {
        c = tbl[(c ^ *buf++) & 0xff] ^ (c >> 8);
    }
    return c ^ 0xffffffffUL;
}

static int check_len(unsigned long seed, const unsigned char *buf, z_size_t len) {
    unsigned long got = crc32(seed, buf, len);
    unsigned long ref = crc32_byte_ref(seed, buf, len);
    if (got != ref) {
        fprintf(stderr, "MISMATCH seed=0x%lx len=%zu got=0x%lx ref=0x%lx\n",
                seed, (size_t)len, got, ref);
        return 1;
    }
    return 0;
}

int main(void) {
    static const char golden[] = "123456789";
    unsigned long g = crc32(0L, (const unsigned char *)golden, 9);
    if (g != 0xcbf43926UL) {
        fprintf(stderr, "golden vector failed: 0x%lx\n", g);
        return 1;
    }

    unsigned char buf[65536];
    for (size_t i = 0; i < sizeof(buf); i++)
        buf[i] = (unsigned char)(i * 131 + 17);

    int errs = 0;
    for (z_size_t len = 0; len <= 512; len++) {
        errs += check_len(0L, buf, len);
        errs += check_len(0xdeadbeefUL, buf, len);
    }
    for (z_size_t len = 513; len <= sizeof(buf); len += len < 4096 ? 1 : 37) {
        errs += check_len(0L, buf, len);
        errs += check_len(0xcafebabeUL, buf, len);
    }

    /* Chunked crc32 must match one-shot (exercises SIMD + scalar tail). */
    for (z_size_t len = 1; len <= 8192; len++) {
        unsigned long whole = crc32(0L, buf, len);
        z_size_t split = len / 3;
        unsigned long part = crc32(0L, buf, split);
        part = crc32(part, buf + split, len - split);
        if (whole != part) {
            fprintf(stderr, "chunk mismatch len=%zu whole=0x%lx part=0x%lx\n",
                    (size_t)len, whole, part);
            errs++;
        }
    }

    if (errs) {
        fprintf(stderr, "FAIL: %d mismatches\n", errs);
        return 1;
    }
    printf("PASS: crc32 PCLMUL matches byte reference (%zu lengths + golden)\n",
           (size_t)sizeof(buf));
    return 0;
}
