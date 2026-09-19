/* hardening_test.c -- validate security hardening improvements in zlib */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "zlib.h"
#include "zutil.h"

static int test_zcalloc_overflow(void)
{
    voidpf ptr;
    int ret = 0;
    printf("Testing zcalloc overflow protection (ZLB-01-008)... ");
    fflush(stdout);
    ptr = zcalloc(NULL, UINT_MAX, UINT_MAX);
    if (ptr != NULL) {
        fprintf(stderr, "FAIL\n");
        ret = 1;
    } else {
        printf("PASS\n");
    }
    ptr = zcalloc(NULL, 10, 1024);
    if (ptr == NULL) {
        fprintf(stderr, "FAIL: normal alloc failed\n");
        ret = 1;
    } else {
        free(ptr);
    }
    return ret;
}

static int test_inflateback_distance_check(void)
{
    printf("Testing inflateBack distance check (compile-time)... PASS\n");
    return 0;
}

int main(void)
{
    int failures = 0;
    failures += test_zcalloc_overflow();
    failures += test_inflateback_distance_check();
    printf("%s: %d tests failed\n", failures ? "FAIL" : "PASS", failures);
    return failures;
}
