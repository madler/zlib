/* Regression test: the 16-bit extra-length checks in zipOpenNewFileInZip4_64()
 * must reserve room for the inline Zip64 extended-info blocks that
 * Write_LocalFileHeader() and the central-directory close path add on top of
 * the caller's extra fields.
 *
 * Background: Write_LocalFileHeader() writes
 *     size_extrafield = size_extrafield_local + 20   (inline Zip64 block)
 * as a 16-bit field, and the central-directory close path writes
 *     size_centralExtra = size_extrafield_global + (4 .. 28)
 * the same way. When the sum exceeds 0xffff, zip64local_putValue()'s
 * overflow hack replaces the field with an all-0xff sentinel, so the header
 * declares a length that disagrees with the bytes actually written. A
 * conforming reader then lands at the wrong offset (demonstrated: miniunz
 * reports "error -105 with zipfile in unzCloseCurrentFile", Python's zipfile
 * raises BadZipFile on CRC) while a boundary-exact member extracts cleanly.
 *
 * Expected results AFTER the fix (this test must pass):
 *   local extra 0xffeb + zip64 -> OK   (0xffeb + 20 = 0xffff exactly)
 *   local extra 0xffec + zip64 -> ZIP_PARAMERROR
 *   local extra 0xffec, no zip64 -> OK (no reservation needed)
 *   global extra 0xffe3 + zip64 -> OK  (0xffe3 + 28 = 0xffff exactly)
 *   global extra 0xffe4 + zip64 -> ZIP_PARAMERROR
 * BEFORE the fix, the three ZIP_PARAMERROR cases return ZIP_OK and produce
 * archives whose headers disagree with the bytes written.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zip.h"

static int failures = 0;

static int open_member(const char *zipname, unsigned local_extra,
                       unsigned global_extra, int zip64)
{
    unsigned char *l = NULL, *g = NULL;
    zipFile zf;
    int err;

    if (local_extra)
    {
        l = (unsigned char *)malloc(local_extra);
        memset(l, 0xAA, local_extra);
    }
    if (global_extra)
    {
        g = (unsigned char *)malloc(global_extra);
        memset(g, 0xBB, global_extra);
    }
    zf = zipOpen(zipname, APPEND_STATUS_CREATE);
    err = zipOpenNewFileInZip4_64(zf, "x.bin", NULL, l, local_extra,
                                  g, global_extra, "c", 0, 0, 0, 0, 0, 0,
                                  NULL, 0, 0, 0, zip64);
    if (err == ZIP_OK)
    {
        char buf[16];
        memset(buf, 'Z', sizeof buf);
        zipWriteInFileInZip(zf, buf, sizeof buf);
        zipCloseFileInZip(zf);
    }
    zipClose(zf, NULL);
    free(l);
    free(g);
    return err;
}

static void expect(const char *what, int got, int want)
{
    if ((got == ZIP_OK) == (want == ZIP_OK))
        printf("PASS  %-52s -> %d\n", what, got);
    else
    {
        printf("FAIL  %-52s -> %d (expected %s)\n", what, got,
               want == ZIP_OK ? "ZIP_OK" : "ZIP_PARAMERROR");
        failures++;
    }
}

int main(void)
{
    expect("local 0xffeb + zip64 (0xffeb+20 == 0xffff, boundary)",
           open_member("t1.zip", 0xffeb, 0, 1), ZIP_OK);
    expect("local 0xffec + zip64 (sum 0x10000 would overflow)",
           open_member("t2.zip", 0xffec, 0, 1), ZIP_PARAMERROR);
    expect("local 0xffec without zip64 (must not over-reject)",
           open_member("t3.zip", 0xffec, 0, 0), ZIP_OK);
    expect("local 0xfff8 + zip64 (the corruption case)",
           open_member("t4.zip", 0xfff8, 0, 1), ZIP_PARAMERROR);
    expect("global 0xffe3 + zip64 (0xffe3+28 == 0xffff, boundary)",
           open_member("t5.zip", 0, 0xffe3, 1), ZIP_OK);
    expect("global 0xffe4 + zip64 (sum > 0xffff would overflow)",
           open_member("t6.zip", 0, 0xffe4, 1), ZIP_PARAMERROR);

    remove("t1.zip"); remove("t2.zip"); remove("t3.zip");
    remove("t4.zip"); remove("t5.zip"); remove("t6.zip");

    if (failures)
    {
        printf("%d FAILURES\n", failures);
        return 1;
    }
    printf("all cases pass\n");
    return 0;
}
