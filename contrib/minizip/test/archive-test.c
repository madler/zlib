#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zip.h"
#include "unzip.h"

static void put_le(unsigned char *buf, ZPOS64_T value, unsigned size) {
    while (size--) {
        *buf++ = (unsigned char)value;
        value >>= 8;
    }
}

static ZPOS64_T get_le32(const unsigned char *buf) {
    return (ZPOS64_T)buf[0] | ((ZPOS64_T)buf[1] << 8) |
           ((ZPOS64_T)buf[2] << 16) | ((ZPOS64_T)buf[3] << 24);
}

static int add_entry(zipFile file, const char *name) {
    int err = zipOpenNewFileInZip(file, name, NULL, NULL, 0, NULL, 0, NULL, 0, 0);
    return err == ZIP_OK ? zipCloseFileInZip(file) : err;
}

static int create_archive(const char *path, const char *comment, int zip64,
                          int prefix) {
    zipFile file;
    FILE *stream;
    unsigned char eocd[22], records[76] = {0};
    long pos;
    int err;

    if (prefix) {
        stream = fopen(path, "wb");
        if (stream == NULL)
            return 1;
        err = fputs("stub", stream) == EOF;
        err |= fclose(stream) != 0;
        if (err)
            return 1;
    }
    file = zipOpen64(path, prefix ? APPEND_STATUS_CREATEAFTER : APPEND_STATUS_CREATE);
    if (file == NULL)
        return 1;
    err = add_entry(file, "original");
    err |= zipClose(file, comment);
    if (err != ZIP_OK || !zip64)
        return err != ZIP_OK;

    /* Insert a ZIP64 end record and adjacent locator without a huge archive. */
    stream = fopen(path, "r+b");
    if (stream == NULL)
        return 1;
    err = fseek(stream, -(long)(sizeof(eocd) + strlen(comment)), SEEK_END) != 0;
    pos = ftell(stream);
    if (err || pos < 0 || fread(eocd, 1, sizeof(eocd), stream) != sizeof(eocd)) {
        fclose(stream);
        return 1;
    }
    put_le(records, 0x06064b50, 4);
    put_le(records + 4, 44, 8);
    put_le(records + 12, 45, 2);
    put_le(records + 14, 45, 2);
    put_le(records + 24, 1, 8);
    put_le(records + 32, 1, 8);
    put_le(records + 40, get_le32(eocd + 12), 8);
    put_le(records + 48, get_le32(eocd + 16), 8);
    put_le(records + 56, 0x07064b50, 4);
    put_le(records + 64, (ZPOS64_T)pos, 8);
    put_le(records + 72, 1, 4);
    memset(eocd + 8, 0xff, 12);
    err = fseek(stream, pos, SEEK_SET) != 0;
    err |= fwrite(records, 1, sizeof(records), stream) != sizeof(records);
    err |= fwrite(eocd, 1, sizeof(eocd), stream) != sizeof(eocd);
    err |= fwrite(comment, 1, strlen(comment), stream) != strlen(comment);
    err |= fclose(stream) != 0;
    return err;
}

static int check_archive(const char *path, ZPOS64_T entries) {
    unzFile file = unzOpen64(path);
    unz_global_info64 info;
    char byte;
    int err;

    if (file == NULL)
        return 1;
    err = unzGetGlobalInfo64(file, &info) != UNZ_OK;
    if (!err)
        err = info.number_entry != entries;
    if (!err)
        err = unzLocateFile(file, "original", 1) != UNZ_OK;
    if (!err) {
        err = unzOpenCurrentFile(file) != UNZ_OK;
        if (!err) {
            err = unzReadCurrentFile(file, &byte, 1) != 0;
            err |= unzCloseCurrentFile(file) != UNZ_OK;
        }
    }
    if (!err && entries == 2)
        err = unzLocateFile(file, "added", 1) != UNZ_OK;
    err |= unzClose(file) != UNZ_OK;
    return err;
}

static int check_large_seek(const char *path) {
    zlib_filefunc64_def funcs;
    voidpf stream;
    ZPOS64_T offset = ((ZPOS64_T)1 << 32) + 17;
    int err;

    fill_fopen64_filefunc(&funcs);
    stream = funcs.zopen64_file(funcs.opaque, path,
                               ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_CREATE);
    if (stream == NULL)
        return 1;
    err = funcs.zseek64_file(funcs.opaque, stream, offset, ZLIB_FILEFUNC_SEEK_SET) != 0;
    err |= funcs.ztell64_file(funcs.opaque, stream) != offset;
    err |= funcs.zclose_file(funcs.opaque, stream) != 0;
    return err;
}

int main(int argc, char **argv) {
    char mismatch[281], boundary[2001], maximum[65536];
    const char *comments[5];
    int which, zip64, prefix, failures = 0;

    if (argc != 2)
        return 1;
    memset(mismatch, 'x', sizeof(mismatch) - 1);
    memcpy(mismatch, "PK\005\006", 4);
    mismatch[20] = mismatch[21] = 1;
    mismatch[sizeof(mismatch) - 1] = 0;
    memset(boundary, 'x', sizeof(boundary) - 1);
    memcpy(boundary + 2000 - 1029, "PK\005\006", 4);
    boundary[sizeof(boundary) - 1] = 0;
    memset(maximum, 'x', sizeof(maximum) - 1);
    memcpy(maximum + 65535 - 4, "PK\005\006", 4);
    maximum[sizeof(maximum) - 1] = 0;
    comments[0] = "";
    comments[1] = "PK\005\006";
    comments[2] = mismatch;
    comments[3] = boundary;
    comments[4] = maximum;

    for (zip64 = 0; zip64 != 2; zip64++)
        for (prefix = 0; prefix != 2; prefix++)
            for (which = 0; which != 5; which++) {
                zipFile file;
                int err;

                if (create_archive(argv[1], comments[which], zip64, prefix)) {
                    fprintf(stderr, "create failed: zip64=%d prefix=%d comment=%d\n",
                            zip64, prefix, which);
                    failures++;
                    continue;
                }
                if (check_archive(argv[1], 1)) {
                    fprintf(stderr, "read failed: zip64=%d prefix=%d comment=%d\n",
                            zip64, prefix, which);
                    failures++;
                }
                file = zipOpen64(argv[1], APPEND_STATUS_ADDINZIP);
                err = file == NULL;
                if (!err) {
                    err = add_entry(file, "added") != ZIP_OK;
                    err |= zipClose(file, comments[which]) != ZIP_OK;
                    err |= check_archive(argv[1], 2);
                }
                if (err) {
                    fprintf(stderr, "append failed: zip64=%d prefix=%d comment=%d\n",
                            zip64, prefix, which);
                    failures++;
                }
            }
    if (check_large_seek(argv[1])) {
        fprintf(stderr, "64-bit file seek failed\n");
        failures++;
    }
    if (remove(argv[1]) != 0)
        failures++;
    return failures ? 1 : 0;
}
