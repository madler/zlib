#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "zlib.h"
#include "unzip.h"

enum {
    NEED_U = 1,
    NEED_C = 2,
    NEED_O = 4,
    NEED_D = 8
};

enum {
    PLAIN,
    ZIP64,
    PADDED,
    UNKNOWN_BEFORE,
    BAD_ZERO,
    BAD_SHORT,
    BAD_OVERSIZE,
    BAD_HEADER,
    EXACT_SENTINEL
};

typedef struct {
    unsigned char bytes[2048];
    size_t size;
} Buffer;

typedef struct {
    const char *name;
    unsigned required;
    int mode;
    int valid;
    int extract;
    uint64_t expected_u;
    uint64_t expected_c;
    const char *comment;
} Case;

static void add(Buffer *b, const void *ptr, size_t n) {
    if (n > sizeof(b->bytes) - b->size)
        abort();
    memcpy(b->bytes + b->size, ptr, n);
    b->size += n;
}

static void p16(Buffer *b, uint16_t v) {
    unsigned char x[2] = {
        (unsigned char)v,
        (unsigned char)(v >> 8)
    };
    add(b, x, sizeof x);
}

static void p32(Buffer *b, uint32_t v) {
    p16(b, (uint16_t)v);
    p16(b, (uint16_t)(v >> 16));
}

static void p64(Buffer *b, uint64_t v) {
    p32(b, (uint32_t)v);
    p32(b, (uint32_t)(v >> 32));
}

static void record(
    Buffer *b, uint16_t id, uint16_t claim,
    const Buffer *data
) {
    p16(b, id);
    p16(b, claim);
    add(b, data->bytes, data->size);
}

static Buffer make_zip(const Case *t) {
    Buffer out = {{0}, 0};
    Buffer extra = {{0}, 0};
    Buffer fields = {{0}, 0};

    const char *comment =
        t->comment == NULL ? "COMMENT" : t->comment;

    size_t comment_size = strlen(comment);
    size_t central_start;
    size_t central_size;

    static const char filename[] = "sample.bin";
    static const char content[] = "DATA";

    uint32_t checksum = crc32(0L, Z_NULL, 0);

    if (
        t->mode == ZIP64 ||
        t->mode == PADDED ||
        t->mode == UNKNOWN_BEFORE ||
        t->mode == EXACT_SENTINEL
    ) {
        if (t->required & NEED_U)
            p64(
                &fields,
                t->mode == EXACT_SENTINEL
                    ? 0xffffffffULL : 4
            );

        if (t->required & NEED_C)
            p64(&fields, 4);

        if (t->required & NEED_O)
            p64(&fields, 0);

        if (t->required & NEED_D)
            p32(&fields, 0);

        if (t->mode == PADDED)
            add(&fields, "PAD", 3);

        if (t->mode == UNKNOWN_BEFORE) {
            Buffer unknown = {{0}, 0};
            add(&unknown, "ABCD", 4);
            record(&extra, 0x7777, 4, &unknown);
        }

        record(
            &extra, 1,
            (uint16_t)fields.size,
            &fields
        );
    }
    else if (t->mode == BAD_ZERO) {
        record(&extra, 1, 0, &fields);
        comment = "12345678";
        comment_size = 8;
    }
    else if (t->mode == BAD_SHORT) {
        p32(&fields, 4);
        record(&extra, 1, 4, &fields);
        comment = "1234";
        comment_size = 4;
    }
    else if (t->mode == BAD_OVERSIZE) {
        record(&extra, 0x7777, 16, &fields);
        comment = "1234567890123456";
        comment_size = 16;
    }
    else if (t->mode == BAD_HEADER) {
        add(&extra, "X", 1);
    }

    checksum = crc32(
        checksum,
        (const Bytef *)content,
        sizeof content - 1
    );

    /* Local file header and stored data. */
    p32(&out, 0x04034b50);
    p16(&out, 20);
    p16(&out, 0);
    p16(&out, 0);
    p16(&out, 0);
    p16(&out, 0);
    p32(&out, checksum);
    p32(&out, 4);
    p32(&out, 4);
    p16(&out, (uint16_t)(sizeof filename - 1));
    p16(&out, 0);

    add(&out, filename, sizeof filename - 1);
    add(&out, content, sizeof content - 1);

    central_start = out.size;

    /* Central directory entry. */
    p32(&out, 0x02014b50);
    p16(&out, 20);
    p16(&out, 20);
    p16(&out, 0);
    p16(&out, 0);
    p16(&out, 0);
    p16(&out, 0);
    p32(&out, checksum);

    p32(
        &out,
        t->required & NEED_C ? 0xffffffffu : 4
    );

    p32(
        &out,
        t->required & NEED_U ? 0xffffffffu : 4
    );

    p16(&out, (uint16_t)(sizeof filename - 1));
    p16(&out, (uint16_t)extra.size);
    p16(&out, (uint16_t)comment_size);

    p16(
        &out,
        t->required & NEED_D ? 0xffff : 0
    );

    p16(&out, 0);
    p32(&out, 0);

    p32(
        &out,
        t->required & NEED_O ? 0xffffffffu : 0
    );

    add(&out, filename, sizeof filename - 1);
    add(&out, extra.bytes, extra.size);
    add(&out, comment, comment_size);

    central_size = out.size - central_start;

    /* End-of-central-directory record. */
    p32(&out, 0x06054b50);
    p16(&out, 0);
    p16(&out, 0);
    p16(&out, 1);
    p16(&out, 1);
    p32(&out, (uint32_t)central_size);
    p32(&out, (uint32_t)central_start);
    p16(&out, 0);

    return out;
}

static int run_case(const Case *t) {
    Buffer data = make_zip(t);

    char path[] = "/tmp/minizip-zip64-test-XXXXXX";

    int fd = mkstemp(path);
    int ok = 1;
    int result = -999;
    int close_archive = -999;

    FILE *out;
    unzFile zip;
    unz_file_info64 info;

    char comment[64] = {0};

    if (fd < 0) {
        perror("mkstemp");
        return 1;
    }

    out = fdopen(fd, "wb");

    if (out == NULL) {
        close(fd);
        unlink(path);
        return 1;
    }

    {
        size_t written = fwrite(
            data.bytes, 1, data.size, out
        );
        int close_result = fclose(out);

        if (
            written != data.size ||
            close_result != 0
        ) {
            unlink(path);
            return 1;
        }
    }

    memset(&info, 0, sizeof info);

    zip = unzOpen64(path);

    if (zip != NULL) {
        result = unzGoToFirstFile(zip);

        if (result == UNZ_OK) {
            result = unzGetCurrentFileInfo64(
                zip,
                &info,
                NULL, 0,
                NULL, 0,
                comment, sizeof comment
            );
        }

        if (t->valid && result == UNZ_OK) {
            ok &= (
                (uint64_t)info.uncompressed_size
                == t->expected_u
            );

            ok &= (
                (uint64_t)info.compressed_size
                == t->expected_c
            );

            ok &= strcmp(
                comment,
                t->comment == NULL
                    ? "COMMENT" : t->comment
            ) == 0;

            if (t->extract) {
                char actual[16];
                int opened = unzOpenCurrentFile(zip);

                if (opened == UNZ_OK) {
                    int first = unzReadCurrentFile(
                        zip, actual, sizeof actual
                    );
                    int next = -999;

                    if (first >= 0) {
                        size_t consumed = (size_t)first;
                        next = unzReadCurrentFile(
                            zip,
                            actual + consumed,
                            (unsigned)(
                                sizeof actual - consumed
                            )
                        );
                    }

                    ok &= (
                        first == 4 &&
                        next == 0 &&
                        memcmp(actual, "DATA", 4) == 0
                    );

                    ok &= (
                        unzCloseCurrentFile(zip)
                        == UNZ_OK
                    );
                }
                else {
                    ok = 0;
                }
            }
        }

        close_archive = unzClose(zip);
    }

    unlink(path);

    if (t->valid) {
        ok &= (
            zip != NULL &&
            result == UNZ_OK &&
            close_archive == UNZ_OK
        );
    }
    else {
        ok &= (
            zip == NULL ||
            result != UNZ_OK
        );
    }

    printf(
        "CASE=%s EXPECTED=%s RESULT=%s INFO=%d\n",
        t->name,
        t->valid ? "ACCEPT" : "REJECT",
        ok ? "PASS" : "FAIL",
        result
    );

    return !ok;
}

int main(void) {
    const Case tests[] = {
        {"valid_basic", 0, PLAIN, 1, 1, 4, 4, "COMMENT"},
        {"valid_zip64_u", NEED_U, ZIP64, 1, 1, 4, 4, "COMMENT"},
        {"valid_zip64_both", NEED_U | NEED_C, ZIP64, 1, 1, 4, 4, "COMMENT"},
        {"valid_zip64_offset", NEED_O, ZIP64, 1, 1, 4, 4, "COMMENT"},
        {"valid_zip64_disk", NEED_D, ZIP64, 1, 1, 4, 4, "COMMENT"},
        {"valid_zip64_padded", NEED_U, PADDED, 1, 1, 4, 4, "COMMENT"},
        {"valid_unknown_before", NEED_U, UNKNOWN_BEFORE, 1, 1, 4, 4, "COMMENT"},
        {"encoded_exact_sentinel", NEED_U, EXACT_SENTINEL, 1, 0, 0xffffffffULL, 4, "COMMENT"},
        {"bad_zero", NEED_U, BAD_ZERO, 0, 0, 0, 0, NULL},
        {"bad_short", NEED_U, BAD_SHORT, 0, 0, 0, 0, NULL},
        {"bad_oversize", 0, BAD_OVERSIZE, 0, 0, 0, 0, NULL},
        {"bad_header", 0, BAD_HEADER, 0, 0, 0, 0, NULL},
        {"missing_uncompressed", NEED_U, PLAIN, 0, 0, 0, 0, NULL},
        {"missing_compressed", NEED_C, PLAIN, 0, 0, 0, 0, NULL},
        {"missing_offset", NEED_O, PLAIN, 0, 0, 0, 0, NULL},
        {"missing_disk", NEED_D, PLAIN, 0, 0, 0, 0, NULL}
    };

    size_t i;
    size_t failures = 0;

    for (i = 0; i < sizeof tests / sizeof tests[0]; ++i)
        failures += run_case(&tests[i]);

    printf(
        "TOTAL_CASES=%zu FAILURES=%zu\n",
        sizeof tests / sizeof tests[0],
        failures
    );

    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
