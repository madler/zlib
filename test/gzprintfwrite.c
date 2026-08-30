/* gzprintfwrite.c -- test a non-blocking gzprintf followed by gzwrite
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
    int pipefd[2];
    unsigned char fill[4096];
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

    file = gzdopen(pipefd[1], "wbN");
    if (file == NULL || gzbuffer(file, 8) != 0) {
        fprintf(stderr, "gzip setup failed\n");
        return 1;
    }

    if (gzprintf(file, "%s", "123456") != 6 ||
        gzprintf(file, "%s", "abcdef") != 6) {
        fprintf(stderr, "gzprintf did not reach the expected stalled state\n");
        return 1;
    }

    put = gzwrite(file, "ABCDEFG", 7);
    if (put != 0) {
        fprintf(stderr, "gzwrite consumed input while the gzip input buffer was stalled: %d\n", put);
        return 1;
    }

    (void)gzclose(file);
    close(pipefd[0]);
    return 0;
}
