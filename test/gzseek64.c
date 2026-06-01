/* Exercise overflow handling in gzseek64(). */

#include <limits.h>
#include <stdio.h>

#include "zlib.h"

/* gzseek64() is always built, though zlib.h only declares it when the
   platform's large-file feature macros request that interface. */
extern z_off64_t ZEXPORT gzseek64(gzFile, z_off64_t, int);

static z_off64_t off64_max(void) {
    if (sizeof(z_off64_t) == sizeof(int))
        return INT_MAX;
    if (sizeof(z_off64_t) == sizeof(long))
        return LONG_MAX;
#ifdef LLONG_MAX
    if (sizeof(z_off64_t) == sizeof(long long))
        return LLONG_MAX;
#endif
    return (z_off64_t)-1;
}

int main(void) {
    static char const path[] = "gzseek64.tmp";
    FILE *plain;
    gzFile file;
    int ret = 1;

    plain = fopen(path, "wb");
    if (plain == NULL)
        return 1;
    if (fclose(plain) != 0)
        return 1;

    file = gzopen(path, "rb");
    if (file != NULL) {
        if (gzseek64(file, off64_max(), SEEK_SET) == off64_max() &&
                gzseek64(file, 1, SEEK_CUR) == -1)
            ret = 0;
        gzclose(file);
    }
    remove(path);
    return ret;
}
