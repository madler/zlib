/* gzread_nonblock.c -- test non-blocking gzip reads
 * Copyright (C) 2026 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zlib.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    static const unsigned char compressed[] = {
        0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xcb,
        0xcb, 0xcf, 0x4b, 0xca, 0xc9, 0x4f, 0xce, 0xce, 0xcc, 0x4b, 0x57,
        0x48, 0xaf, 0xca, 0x2c, 0x50, 0x28, 0x2e, 0x29, 0x4a, 0x4d, 0xcc,
        0xe5, 0x02, 0x00, 0xd5, 0x80, 0x6a, 0xb7, 0x18, 0x00, 0x00, 0x00
    };
    static const unsigned char expected[] = "nonblocking gzip stream\n";
    unsigned char output[sizeof(expected)];
    int pipefd[2];
    int got;
    int ret;
    size_t have;
    size_t i;
    gzFile file;

    if (pipe(pipefd) == -1 ||
        fcntl(pipefd[0], F_SETFL,
              fcntl(pipefd[0], F_GETFL) | O_NONBLOCK) == -1) {
        perror("pipe");
        return 1;
    }
    file = gzdopen(pipefd[0], "rbN");
    if (file == NULL) {
        fprintf(stderr, "gzdopen failed\n");
        return 1;
    }

    have = 0;
    for (i = 0; i < sizeof(compressed); i++) {
        if (write(pipefd[1], compressed + i, 1) != 1) {
            perror("write");
            return 1;
        }
        do {
            errno = 0;
            got = gzread(file, output + have,
                         (unsigned)(sizeof(output) - have));
            if (got > 0)
                have += (unsigned)got;
        } while (got > 0);
        if (got != -1 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            fprintf(stderr, "gzread did not report a non-blocking stall\n");
            return 1;
        }
    }

    close(pipefd[1]);
    do {
        got = gzread(file, output + have,
                     (unsigned)(sizeof(output) - have));
        if (got > 0)
            have += (unsigned)got;
    } while (got > 0);
    if (got != 0 || have != sizeof(expected) - 1 ||
        memcmp(output, expected, sizeof(expected) - 1) != 0) {
        fprintf(stderr, "non-blocking gzip output mismatch\n");
        return 1;
    }

    ret = gzclose(file);
    if (ret != Z_OK) {
        fprintf(stderr, "gzclose returned %d after a complete stream\n", ret);
        return 1;
    }
    return 0;
}
