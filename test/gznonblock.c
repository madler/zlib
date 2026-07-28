/* gznonblock.c -- test non-blocking gzip writes
 * Copyright (C) 2026 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zlib.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INPUT_SIZE (1024 * 1024)

int main(void) {
    int pipefd[2];
    unsigned char fill[4096];
    unsigned char *input;
    unsigned seed;
    size_t i;
    gzFile file;
    int put;

    if (pipe(pipefd) == -1 ||
        fcntl(pipefd[1], F_SETFL,
              fcntl(pipefd[1], F_GETFL) | O_NONBLOCK) == -1) {
        perror("pipe");
        return 1;
    }

    memset(fill, 0xa5, sizeof(fill));
    while (write(pipefd[1], fill, sizeof(fill)) > 0)
        ;
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("fill pipe");
        return 1;
    }

    /*
     * Use incompressible input larger than the gzip buffers so that the full
     * pipe stalls gzwrite() before it consumes the caller's entire buffer.
     */
    input = (unsigned char *)malloc(INPUT_SIZE);
    if (input == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    seed = 0x12345678U;
    for (i = 0; i < INPUT_SIZE; i++) {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        input[i] = (unsigned char)seed;
    }

    file = gzdopen(pipefd[1], "wbN");
    if (file == NULL) {
        fprintf(stderr, "gzdopen failed\n");
        return 1;
    }

    put = gzwrite(file, input, INPUT_SIZE);
    free(input);
    if (put <= 0 || put >= INPUT_SIZE) {
        fprintf(stderr, "gzwrite did not make partial progress: %d\n", put);
        return 1;
    }

    /*
     * gzwrite() reported the remaining input as unconsumed, so the caller was
     * allowed to release it. A following gzip operation must not access it.
     */
    put = gzprintf(file, "%s", "X");
    if (put != 1) {
        fprintf(stderr, "gzprintf failed after partial gzwrite: %d\n", put);
        return 1;
    }

    if (fcntl(pipefd[0], F_SETFL,
              fcntl(pipefd[0], F_GETFL) | O_NONBLOCK) == -1) {
        perror("read pipe");
        return 1;
    }
    while (read(pipefd[0], fill, sizeof(fill)) > 0)
        ;
    (void)gzclose(file);
    close(pipefd[0]);
    return 0;
}
