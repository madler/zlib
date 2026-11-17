/* testzip64.c -- check zip64 headers for a small entry (#799, #406)
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zlib.h"
#include "zip.h"
#include "unzip.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ZIPNAME "test64.zip"
#define PAYLOAD "hello zip64 extra field test\n"

static uint16_t get16(const unsigned char *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t get32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t get64(const unsigned char *p) {
    int i;
    uint64_t v = 0;

    for (i = 7; i >= 0; i--)
        v = (v << 8) | p[i];
    return v;
}

static void fail(const char *msg) {
    fprintf(stderr, "testzip64 failed: %s\n", msg);
    remove(ZIPNAME);
    exit(1);
}

/* Find the first extra-field block with header id 0x0001. */
static const unsigned char *zip64_block(const unsigned char *extra,
                                        uint16_t len, uint16_t *size) {
    size_t off = 0;

    while (off + 4 <= (size_t)len) {
        uint16_t id = get16(extra + off);
        uint16_t sz = get16(extra + off + 2);

        off += 4;
        if (off + sz > (size_t)len)
            return NULL;
        if (id == 1) {
            *size = sz;
            return extra + off;
        }
        off += sz;
    }
    return NULL;
}

static unsigned char *read_file(const char *path, long *len) {
    FILE *fp;
    unsigned char *buf;

    fp = fopen(path, "rb");
    if (fp == NULL)
        fail("cannot read archive");
    if (fseek(fp, 0, SEEK_END) != 0)
        fail("fseek");
    *len = ftell(fp);
    if (*len < 0)
        fail("ftell");
    if (fseek(fp, 0, SEEK_SET) != 0)
        fail("fseek");
    buf = (unsigned char *)malloc((size_t)*len);
    if (buf == NULL)
        fail("out of memory");
    if (fread(buf, 1, (size_t)*len, fp) != (size_t)*len)
        fail("short read");
    fclose(fp);
    return buf;
}

/* Write one entry with the given zip64 flag. */
static void write_entry(int zip64) {
    zipFile zf;
    int err;

    remove(ZIPNAME);
    zf = zipOpen64(ZIPNAME, APPEND_STATUS_CREATE);
    if (zf == NULL)
        fail("zipOpen64");
    err = zipOpenNewFileInZip2_64(zf, "small.txt", NULL,
                                  NULL, 0, NULL, 0, NULL,
                                  Z_DEFLATED, Z_DEFAULT_COMPRESSION,
                                  0, zip64);
    if (err != ZIP_OK)
        fail("zipOpenNewFileInZip2_64");
    err = zipWriteInFileInZip(zf, PAYLOAD, (unsigned)strlen(PAYLOAD));
    if (err != ZIP_OK)
        fail("zipWriteInFileInZip");
    err = zipCloseFileInZip(zf);
    if (err != ZIP_OK)
        fail("zipCloseFileInZip");
    err = zipClose(zf, NULL);
    if (err != ZIP_OK)
        fail("zipClose");
}

static void read_back(void) {
    unzFile uf;
    char buf[128];
    int n;

    uf = unzOpen64(ZIPNAME);
    if (uf == NULL)
        fail("unzOpen64");
    if (unzLocateFile(uf, "small.txt", 1) != UNZ_OK)
        fail("unzLocateFile");
    if (unzOpenCurrentFile(uf) != UNZ_OK)
        fail("unzOpenCurrentFile");
    n = unzReadCurrentFile(uf, buf, sizeof(buf));
    if (n != (int)strlen(PAYLOAD) || memcmp(buf, PAYLOAD, (size_t)n) != 0)
        fail("unzReadCurrentFile data mismatch");
    if (unzCloseCurrentFile(uf) != UNZ_OK)
        fail("unzCloseCurrentFile");
    if (unzClose(uf) != UNZ_OK)
        fail("unzClose");
}

static void check_headers(int want_zip64, uint64_t uncomp, uint64_t comp) {
    long flen;
    unsigned char *buf;
    size_t off;
    uint16_t local_ver = 0, central_ver = 0;
    uint32_t local_csz = 0, local_usz = 0;
    uint32_t central_csz = 0, central_usz = 0;
    uint16_t local_elen = 0, central_elen = 0;
    const unsigned char *local_extra = NULL;
    const unsigned char *central_extra = NULL;
    int saw_local = 0, saw_central = 0;

    buf = read_file(ZIPNAME, &flen);
    off = 0;
    while (off + 4 <= (size_t)flen) {
        uint32_t sig = get32(buf + off);

        if (sig == 0x04034b50UL) {
            uint16_t nlen;

            if (off + 30 > (size_t)flen)
                fail("truncated local header");
            local_ver = get16(buf + off + 4);
            local_csz = get32(buf + off + 18);
            local_usz = get32(buf + off + 22);
            nlen = get16(buf + off + 26);
            local_elen = get16(buf + off + 28);
            if (off + 30 + nlen + local_elen > (size_t)flen)
                fail("truncated local extra");
            local_extra = buf + off + 30 + nlen;
            saw_local = 1;
            /* skip local header and file data (32-bit size may be 0xffffffff) */
            if (local_csz != 0xffffffffUL)
                off += 30u + nlen + local_elen + local_csz;
            else
                off += 30u + nlen + local_elen + (size_t)comp;
            continue;
        }
        if (sig == 0x02014b50UL) {
            uint16_t nlen, clen;

            if (off + 46 > (size_t)flen)
                fail("truncated central header");
            central_ver = get16(buf + off + 6);
            central_csz = get32(buf + off + 20);
            central_usz = get32(buf + off + 24);
            nlen = get16(buf + off + 28);
            central_elen = get16(buf + off + 30);
            clen = get16(buf + off + 32);
            if (off + 46 + nlen + central_elen + clen > (size_t)flen)
                fail("truncated central extra");
            central_extra = buf + off + 46 + nlen;
            saw_central = 1;
            off += 46u + nlen + central_elen + clen;
            continue;
        }
        if (sig == 0x06054b50UL || sig == 0x06064b50UL || sig == 0x07064b50UL)
            break;
        off++;
    }

    if (!saw_local || !saw_central)
        fail("missing local or central header");

    if (want_zip64) {
        uint16_t bsz = 0;
        const unsigned char *blk;
        uint64_t extra_usz, extra_csz;

        if (local_ver != 45)
            fail("local version needed should be 45");
        if (central_ver != 45)
            fail("central version needed should be 45");
        if (local_csz != 0xffffffffUL || local_usz != 0xffffffffUL)
            fail("local 32-bit sizes should be 0xffffffff when zip64 was requested");
        blk = zip64_block(local_extra, local_elen, &bsz);
        if (blk == NULL || bsz < 16)
            fail("missing or short zip64 extra in local header");
        extra_usz = get64(blk);
        extra_csz = get64(blk + 8);
        if (extra_usz != uncomp || extra_csz != comp) {
            fprintf(stderr, "local zip64 extra uncomp=%llu comp=%llu, expected %llu %llu\n",
                    (unsigned long long)extra_usz, (unsigned long long)extra_csz,
                    (unsigned long long)uncomp, (unsigned long long)comp);
            fail("local zip64 extra sizes are wrong");
        }
        /* sizes fit in 32 bits, so the central record does not need a zip64 extra */
        if (central_csz != (uint32_t)comp || central_usz != (uint32_t)uncomp)
            fail("central 32-bit sizes should hold the real sizes");
    }
    else {
        if (local_ver != 20)
            fail("local version needed should be 20 without zip64");
        if (central_ver != 20)
            fail("central version needed should be 20 without zip64");
        if (local_csz != (uint32_t)comp || local_usz != (uint32_t)uncomp)
            fail("local 32-bit sizes should hold the real sizes without zip64");
        if (central_csz != (uint32_t)comp || central_usz != (uint32_t)uncomp)
            fail("central 32-bit sizes should hold the real sizes without zip64");
        {
            uint16_t dummy = 0;
            if (zip64_block(local_extra, local_elen, &dummy) != NULL)
                fail("local header should not have a zip64 extra without zip64");
            if (zip64_block(central_extra, central_elen, &dummy) != NULL)
                fail("central header should not have a zip64 extra without zip64");
        }
    }

    free(buf);
}

static uint64_t stored_uncomp(void) {
    return (uint64_t)strlen(PAYLOAD);
}

static uint64_t stored_comp_from_local_or_data(int zip64) {
    /* After a successful write, get compressed size from central/local. */
    long flen;
    unsigned char *buf;
    size_t off;
    uint64_t comp = 0;
    int found = 0;

    buf = read_file(ZIPNAME, &flen);
    for (off = 0; off + 46 <= (size_t)flen; off++) {
        if (get32(buf + off) == 0x02014b50UL) {
            uint32_t csz = get32(buf + off + 20);
            uint16_t nlen = get16(buf + off + 28);
            uint16_t elen = get16(buf + off + 30);
            uint16_t bsz = 0;
            const unsigned char *blk;

            if (csz != 0xffffffffUL) {
                comp = csz;
                found = 1;
                break;
            }
            blk = zip64_block(buf + off + 46 + nlen, elen, &bsz);
            if (blk != NULL && bsz >= 16) {
                comp = get64(blk + 8);
                found = 1;
                break;
            }
        }
    }
    free(buf);
    if (!found)
        fail("could not get compressed size");
    (void)zip64;
    return comp;
}

int main(void) {
    uint64_t uncomp = stored_uncomp();
    uint64_t comp;

    /* Control: zip64 not requested, small file stays zip32. */
    write_entry(0);
    comp = stored_comp_from_local_or_data(0);
    check_headers(0, uncomp, comp);
    read_back();

    /* Reported case: zip64 requested, but the entry is smaller than 4GB. */
    write_entry(1);
    comp = stored_comp_from_local_or_data(1);
    check_headers(1, uncomp, comp);
    read_back();

    remove(ZIPNAME);
    printf("testzip64 ok\n");
    return 0;
}
