#include <stdio.h>
#include <string.h>

#include "unzip.h"
#include "zip.h"

int main(void) {
    const char *archive_name = "minizip-filename-termination-test.zip";
    const char *long_name = "regression-filename-that-is-too-long.txt";
    const char expected_prefix[] = "regress";
    char filename[sizeof(expected_prefix)];
    zip_fileinfo file_info;
    zipFile zip_file;
    unzFile unzip_file;
    int result = 1;

    memset(&file_info, 0, sizeof(file_info));
    remove(archive_name);

    zip_file = zipOpen(archive_name, APPEND_STATUS_CREATE);
    if (zip_file == NULL) {
        fprintf(stderr, "zipOpen failed\n");
        return result;
    }

    if (zipOpenNewFileInZip(zip_file, long_name, &file_info, NULL, 0, NULL,
                            0, NULL, 0, 0) != ZIP_OK) {
        fprintf(stderr, "zipOpenNewFileInZip failed\n");
        zipClose(zip_file, NULL);
        remove(archive_name);
        return result;
    }

    if (zipWriteInFileInZip(zip_file, "x", 1) != ZIP_OK) {
        fprintf(stderr, "zipWriteInFileInZip failed\n");
        zipCloseFileInZip(zip_file);
        zipClose(zip_file, NULL);
        remove(archive_name);
        return result;
    }

    if (zipCloseFileInZip(zip_file) != ZIP_OK ||
        zipClose(zip_file, NULL) != ZIP_OK) {
        fprintf(stderr, "zip close failed\n");
        remove(archive_name);
        return result;
    }

    unzip_file = unzOpen(archive_name);
    if (unzip_file == NULL) {
        fprintf(stderr, "unzOpen failed\n");
        remove(archive_name);
        return result;
    }

    memset(filename, 0xa5, sizeof(filename));
    if (unzGetCurrentFileInfo64(unzip_file, NULL, filename, sizeof(filename),
                                NULL, 0, NULL, 0) != UNZ_OK) {
        fprintf(stderr, "unzGetCurrentFileInfo64 failed\n");
        unzClose(unzip_file);
        remove(archive_name);
        return result;
    }

    if (memcmp(filename, expected_prefix, sizeof(expected_prefix) - 1) != 0 ||
        filename[sizeof(filename) - 1] != '\0') {
        fprintf(stderr, "truncated filename was not NUL-terminated\n");
        unzClose(unzip_file);
        remove(archive_name);
        return result;
    }

    if (unzClose(unzip_file) != UNZ_OK) {
        fprintf(stderr, "unzClose failed\n");
        remove(archive_name);
        return result;
    }

    remove(archive_name);
    result = 0;
    return result;
}
