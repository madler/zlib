#include <stdio.h>
#include <string.h>

#include "zip.h"

static int expect_rejected(char *data, int data_len) {
    char original[8];
    int original_len = data_len;
    int result;

    memcpy(original, data, (size_t)data_len);
    result = zipRemoveExtraInfoBlock(data, &data_len, 0x1234);
    if (result != ZIP_PARAMERROR || data_len != original_len ||
        memcmp(data, original, (size_t)original_len) != 0) {
        fprintf(stderr, "malformed extra field was not rejected safely\n");
        return 1;
    }
    return 0;
}

int main(void) {
    char short_header[] = {0x34, 0x12, 0x00};
    char truncated_block[] = {0x34, 0x12, 0x01, 0x00};
    char valid_block[] = {0x34, 0x12, 0x02, 0x00, (char)0xab, (char)0xcd};
    int valid_len = (int)sizeof(valid_block);

    if (expect_rejected(short_header, (int)sizeof(short_header)) != 0 ||
        expect_rejected(truncated_block, (int)sizeof(truncated_block)) != 0)
        return 1;

    if (zipRemoveExtraInfoBlock(valid_block, &valid_len, 0x1234) != ZIP_OK ||
        valid_len != 0) {
        fprintf(stderr, "valid extra field was not removed\n");
        return 1;
    }

    return 0;
}
