#include <stdio.h>
#include <string.h>

#include "zip.h"

static int check(const char *name, int condition) {
    if (!condition) {
        fprintf(stderr, "failed: %s\n", name);
        return 1;
    }
    return 0;
}

int main(void) {
    char fields[] = {
        1, 0, 2, 0, 'x', 'y',
        2, 0, 1, 0, 'z'
    };
    char unaligned[] = {
        0,
        1, 0, 1, 0, 'x',
        2, 0, 1, 0, 'y'
    };
    char truncated_header[] = {1, 0, 0, 0, 2};
    char truncated_data[] = {1, 0, 2, 0, 'x'};
    char original[sizeof(truncated_data)];
    int len;
    int ret = 0;

    len = (int)sizeof(fields);
    ret |= check("remove matching field",
                 zipRemoveExtraInfoBlock(fields, &len, 1) == ZIP_OK);
    ret |= check("preserve non-matching field",
                 len == 5 && memcmp(fields, "\2\0\1\0z", 5) == 0);

    len = (int)sizeof(unaligned) - 1;
    ret |= check("accept unaligned input",
                 zipRemoveExtraInfoBlock(unaligned + 1, &len, 1) == ZIP_OK);
    ret |= check("preserve unaligned non-matching field",
                 len == 5 && memcmp(unaligned + 1, "\2\0\1\0y", 5) == 0);

    len = (int)sizeof(truncated_header);
    ret |= check("reject truncated header",
                 zipRemoveExtraInfoBlock(truncated_header, &len, 1) ==
                     ZIP_PARAMERROR);

    memcpy(original, truncated_data, sizeof(original));
    len = (int)sizeof(truncated_data);
    ret |= check("reject truncated payload",
                 zipRemoveExtraInfoBlock(truncated_data, &len, 1) ==
                     ZIP_PARAMERROR);
    ret |= check("leave malformed input unchanged",
                 len == (int)sizeof(truncated_data) &&
                     memcmp(truncated_data, original, sizeof(original)) == 0);

    return ret;
}
