#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"

#define DATA_SIZE 1024

int main() {
    char data[DATA_SIZE];
    char compressed_data[DATA_SIZE];
    char uncompressed_data[DATA_SIZE / 2]; // Buffer is intentionally too small
    uLongf compressed_size = DATA_SIZE;
    uLongf uncompressed_size = DATA_SIZE / 2;
    int result;

    // Fill data with some values
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = i % 256;
    }

    // Compress the data
    result = compress((Bytef *)compressed_data, &compressed_size, (Bytef *)data, DATA_SIZE);
    if (result != Z_OK) {
        printf("Compression failed with error code: %d\n", result);
        return 1;
    }

    // Attempt to uncompress into a buffer that is too small
    result = uncompress((Bytef *)uncompressed_data, &uncompressed_size, (Bytef *)compressed_data, compressed_size);
    if (result == Z_BUF_ERROR) {
        printf("Successfully detected buffer error.\n");
        return 0;
    } else {
        printf("Test failed. Expected Z_BUF_ERROR, but got %d\n", result);
        return 1;
    }
}
