#include <stdio.h>
#include <string.h>

#include "unzip.h"

static int check_current_file(unzFile archive, const char *expected_name)
{
    unz_file_info64 info;
    char filename[128];

    memset(filename, 0, sizeof(filename));
    if (unzGetCurrentFileInfo64(archive, &info, filename, sizeof(filename),
                                NULL, 0, NULL, 0) != UNZ_OK)
        return 0;
    return strcmp(filename, expected_name) == 0;
}

int main(int argc, char **argv)
{
    /* The fixture embeds a false ZIP64 directory and EOCD in file data. It
       points to a hidden manifest with different contents than the real EOCD. */
    static const char expected_manifest[] = "{\"status\":\"safe\"}\n";
    char data[64];
    unz_global_info64 global_info;
    unzFile archive;
    int data_size;
    int result = 1;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s archive.zip\n", argv[0]);
        return 2;
    }

    archive = unzOpen64(argv[1]);
    if (archive == NULL)
    {
        fprintf(stderr, "Failed to open ZIP archive\n");
        return 1;
    }

    if (unzGetGlobalInfo64(archive, &global_info) != UNZ_OK ||
        global_info.number_entry != 2 ||
        unzGoToFirstFile(archive) != UNZ_OK ||
        !check_current_file(archive, "manifest.json") ||
        unzOpenCurrentFile(archive) != UNZ_OK)
        goto cleanup;

    data_size = unzReadCurrentFile(archive, data, sizeof(data));
    if (unzCloseCurrentFile(archive) != UNZ_OK ||
        data_size != (int)(sizeof(expected_manifest) - 1) ||
        memcmp(data, expected_manifest, sizeof(expected_manifest) - 1) != 0 ||
        unzGoToNextFile(archive) != UNZ_OK ||
        !check_current_file(archive, "decoy.bin") ||
        unzGoToNextFile(archive) != UNZ_END_OF_LIST_OF_FILE)
        goto cleanup;

    result = 0;

cleanup:
    if (unzClose(archive) != UNZ_OK)
        result = 1;
    if (result != 0)
        fprintf(stderr, "Unexpected ZIP64 directory selected\n");
    return result;
}
