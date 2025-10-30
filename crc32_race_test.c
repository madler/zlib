#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "zlib.h"

#define NUM_THREADS 10
#define DATA_SIZE 1024

void *crc32_thread(void *arg) {
    char data[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = (char)i;
    }
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)data, DATA_SIZE);
    // We don't care about the result, just that it doesn't crash.
    // A real test case would check for correctness of the CRC.
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int i;

    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, crc32_thread, NULL) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }

    printf("Successfully completed concurrent crc32 calls.\n");
    return 0;
}
