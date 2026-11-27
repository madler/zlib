/*
   miniunz.c
   sample part of the MiniZip project - ( https://www.winimage.com/zLibDll/minizip.html )

         Copyright (C) 1998-2026 Gilles Vollant (minizip) ( https://www.winimage.com/zLibDll/minizip.html )

         Modifications of Unzip for Zip64
         Copyright (C) 2007-2008 Even Rouault

         Modifications for Zip64 support on both zip and unzip
         Copyright (C) 2009-2010 Mathias Svensson ( https://result42.com )
*/

#ifndef _WIN32
#  ifndef _POSIX_C_SOURCE
#    define _POSIX_C_SOURCE 200809L
#  endif
#endif

#if (!defined(_WIN32)) && (!defined(WIN32)) && (!defined(__APPLE__))
        #ifndef __USE_FILE_OFFSET64
                #define __USE_FILE_OFFSET64
        #endif
        #ifndef __USE_LARGEFILE64
                #define __USE_LARGEFILE64
        #endif
        #ifndef _LARGEFILE64_SOURCE
                #define _LARGEFILE64_SOURCE
        #endif
        #ifndef _FILE_OFFSET_BIT
                #define _FILE_OFFSET_BIT 64
        #endif
#endif

#if defined(__APPLE__) || defined(__HAIKU__) || defined(MINIZIP_FOPEN_NO_64)
// In darwin and perhaps other BSD variants off_t is a 64 bit value, hence no need for specific 64 bit functions
#define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#define FTELLO_FUNC(stream) ftello(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko(stream, offset, origin)
#else
#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#endif


#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
# include <direct.h>
# include <io.h>
# include <windows.h>
# include <winternl.h>
#else
# include <unistd.h>
# include <utime.h>
#endif

#if !defined(_WIN32) && defined(AT_FDCWD) && defined(O_NOFOLLOW) && defined(O_DIRECTORY)
# define SAFE_POSIX_EXTRACTION
#endif


#include "unzip.h"

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

#ifdef _WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif
/*
  mini unzip, demo of unzip package

  usage :
  Usage : miniunz [-exvlo] file.zip [file_to_extract] [-d extractdir]

  list the file in the zipfile, and print the content of FILE_ID.ZIP or README.TXT
    if it exists
*/


/* change_file_date : change the date/time of a file
    filename : the filename of the file where date/time must be modified
    dosdate : the new date at the MSDOS format (4 bytes)
    tmu_date : the SAME new date at the tm_unz format */
static void change_file_date(FILE *file, const char *filename, uLong dosdate, tm_unz tmu_date) {
#if !defined(_WIN32) && !defined(SAFE_POSIX_EXTRACTION)
  (void)file;
#endif
#ifdef _WIN32
  (void)filename;
  (void)tmu_date;
  HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
  FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;

  if (hFile == INVALID_HANDLE_VALUE)
    return;
  if (!GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite))
    return;
  DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
  LocalFileTimeToFileTime(&ftLocal,&ftm);
  SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
#elif defined(__unix__) || defined(__unix) || defined(__APPLE__)
  (void)dosdate;
  struct utimbuf ut;
  struct tm newdate;
  newdate.tm_sec = tmu_date.tm_sec;
  newdate.tm_min=tmu_date.tm_min;
  newdate.tm_hour=tmu_date.tm_hour;
  newdate.tm_mday=tmu_date.tm_mday;
  newdate.tm_mon=tmu_date.tm_mon;
  if (tmu_date.tm_year > 1900)
      newdate.tm_year=tmu_date.tm_year - 1900;
  else
      newdate.tm_year=tmu_date.tm_year ;
  newdate.tm_isdst=-1;

  ut.actime=ut.modtime=mktime(&newdate);
#ifdef SAFE_POSIX_EXTRACTION
  {
    struct timespec times[2];
    (void)filename;
    times[0].tv_sec = times[1].tv_sec = ut.modtime;
    times[0].tv_nsec = times[1].tv_nsec = 0;
    futimens(fileno(file), times);
  }
#else
  utime(filename,&ut);
#endif
#else
  (void)filename;
  (void)dosdate;
  (void)tmu_date;
#endif
}

#ifdef _WIN32
# ifndef FILE_OPEN_REPARSE_POINT
#  define FILE_OPEN_REPARSE_POINT 0x00200000
# endif
# ifndef FILE_DIRECTORY_FILE
#  define FILE_DIRECTORY_FILE 0x00000001
# endif
# ifndef FILE_NON_DIRECTORY_FILE
#  define FILE_NON_DIRECTORY_FILE 0x00000040
# endif
# ifndef FILE_SYNCHRONOUS_IO_NONALERT
#  define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
# endif
# ifndef FILE_OPEN
#  define FILE_OPEN 0x00000001
# endif
# ifndef FILE_CREATE
#  define FILE_CREATE 0x00000002
# endif

typedef NTSTATUS (NTAPI *minizip_nt_create_file_fn)(
    PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
    PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);

/* NtCreateFile supports relative opens from a directory handle. Use that to
   inspect each component without letting path resolution follow reparse points. */
static NTSTATUS minizip_nt_create_file(
    PHANDLE file, ACCESS_MASK access, POBJECT_ATTRIBUTES attributes,
    PIO_STATUS_BLOCK io_status, ULONG file_attributes, ULONG share_access,
    ULONG disposition, ULONG options) {
    static minizip_nt_create_file_fn create_file = NULL;
    if (create_file == NULL) {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll != NULL)
            create_file = (minizip_nt_create_file_fn)GetProcAddress(
                ntdll, "NtCreateFile");
        if (create_file == NULL) {
            errno = ENOSYS;
            return (NTSTATUS)0xC0000001L; /* STATUS_UNSUCCESSFUL */
        }
    }
    return create_file(file, access, attributes, io_status, NULL,
                       file_attributes, share_access, disposition, options,
                       NULL, 0);
}

static void minizip_set_nt_errno(NTSTATUS status) {
    if (status == (NTSTATUS)0xC0000034L || /* STATUS_OBJECT_NAME_NOT_FOUND */
        status == (NTSTATUS)0xC000003AL)   /* STATUS_OBJECT_PATH_NOT_FOUND */
        errno = ENOENT;
    else if (status == (NTSTATUS)0xC0000035L) /* STATUS_OBJECT_NAME_COLLISION */
        errno = EEXIST;
    else
        errno = EACCES;
}

static int minizip_windows_component(const char *source, size_t length,
                                    WCHAR **wide, UNICODE_STRING *name) {
    int chars;
    size_t bytes;
    WCHAR *buffer;
    if (length == 0 || length > INT_MAX) {
        errno = EINVAL;
        return -1;
    }
    chars = MultiByteToWideChar(CP_ACP, 0, source, (int)length, NULL, 0);
    if (chars <= 0) {
        errno = EINVAL;
        return -1;
    }
    if ((size_t)chars > (USHRT_MAX - sizeof(WCHAR)) / sizeof(WCHAR)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    bytes = (size_t)chars * sizeof(WCHAR);
    buffer = (WCHAR *)malloc(bytes + sizeof(WCHAR));
    if (buffer == NULL) {
        errno = ENOMEM;
        return -1;
    }
    if (MultiByteToWideChar(CP_ACP, 0, source, (int)length, buffer, chars) != chars) {
        free(buffer);
        errno = EINVAL;
        return -1;
    }
    buffer[chars] = L'\0';
    if ((chars == 1 && buffer[0] == L'.') ||
        (chars == 2 && buffer[0] == L'.' && buffer[1] == L'.') ||
        wcschr(buffer, L':') != NULL ||
        buffer[chars - 1] == L'.' || buffer[chars - 1] == L' ') {
        free(buffer);
        errno = EACCES;
        return -1;
    }
    name->Length = (USHORT)bytes;
    name->MaximumLength = (USHORT)bytes;
    name->Buffer = buffer;
    *wide = buffer;
    return 0;
}

static NTSTATUS minizip_create_relative(
    HANDLE parent, UNICODE_STRING *name, ACCESS_MASK access,
    ULONG attributes, ULONG disposition, ULONG options, PHANDLE file) {
    OBJECT_ATTRIBUTES object_attributes;
    IO_STATUS_BLOCK io_status;
    NTSTATUS status;
    ZeroMemory(&object_attributes, sizeof(object_attributes));
    object_attributes.Length = sizeof(object_attributes);
    object_attributes.RootDirectory = parent;
    object_attributes.ObjectName = name;
    object_attributes.Attributes = OBJ_CASE_INSENSITIVE;
    ZeroMemory(&io_status, sizeof(io_status));
    status = minizip_nt_create_file(file, access, &object_attributes,
                                   &io_status, attributes,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE |
                                       FILE_SHARE_DELETE,
                                   disposition, options);
    return status;
}

static int minizip_is_safe_directory(HANDLE directory) {
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(directory, &info)) {
        errno = EACCES;
        return 0;
    }
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        errno = EACCES;
        return 0;
    }
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        errno = ENOTDIR;
        return 0;
    }
    return 1;
}

static HANDLE minizip_open_directory_component(HANDLE parent,
                                               UNICODE_STRING *name) {
    const ACCESS_MASK access = FILE_LIST_DIRECTORY | FILE_ADD_FILE |
                               FILE_ADD_SUBDIRECTORY | FILE_READ_ATTRIBUTES |
                               SYNCHRONIZE;
    const ULONG options = FILE_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT |
                          FILE_SYNCHRONOUS_IO_NONALERT;
    HANDLE directory = INVALID_HANDLE_VALUE;
    NTSTATUS status = minizip_create_relative(parent, name, access, 0,
                                              FILE_OPEN, options, &directory);
    if (status == (NTSTATUS)0xC0000034L || status == (NTSTATUS)0xC000003AL) {
        status = minizip_create_relative(parent, name, access,
                                         FILE_ATTRIBUTE_DIRECTORY,
                                         FILE_CREATE, options, &directory);
        if (status == (NTSTATUS)0xC0000035L)
            status = minizip_create_relative(parent, name, access, 0,
                                             FILE_OPEN, options, &directory);
    }
    if (status < 0) {
        minizip_set_nt_errno(status);
        return INVALID_HANDLE_VALUE;
    }
    if (!minizip_is_safe_directory(directory)) {
        CloseHandle(directory);
        return INVALID_HANDLE_VALUE;
    }
    return directory;
}

static HANDLE minizip_open_output_directory(const char *path) {
    const DWORD access = FILE_LIST_DIRECTORY | FILE_ADD_FILE |
                         FILE_ADD_SUBDIRECTORY | FILE_READ_ATTRIBUTES |
                         SYNCHRONIZE;
    HANDLE directory = CreateFileW(L".", access,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE |
                                       FILE_SHARE_DELETE,
                                   NULL, OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS, NULL);
    const char *part = path;
    if (directory == INVALID_HANDLE_VALUE) {
        errno = EACCES;
        return INVALID_HANDLE_VALUE;
    }
    while (*part != '\0') {
        const char *start;
        size_t length;
        WCHAR *wide;
        UNICODE_STRING name;
        HANDLE next;
        while (*part == '/' || *part == '\\')
            part++;
        if (*part == '\0')
            break;
        start = part;
        while (*part != '\0' && *part != '/' && *part != '\\')
            part++;
        length = (size_t)(part - start);
        if (minizip_windows_component(start, length, &wide, &name) != 0) {
            CloseHandle(directory);
            return INVALID_HANDLE_VALUE;
        }
        next = minizip_open_directory_component(directory, &name);
        free(wide);
        CloseHandle(directory);
        if (next == INVALID_HANDLE_VALUE)
            return INVALID_HANDLE_VALUE;
        directory = next;
    }
    return directory;
}

static HANDLE minizip_open_output_file_handle(HANDLE directory,
                                              UNICODE_STRING *name) {
    const ACCESS_MASK access = FILE_WRITE_DATA | FILE_READ_ATTRIBUTES |
                               FILE_WRITE_ATTRIBUTES | SYNCHRONIZE;
    const ULONG options = FILE_NON_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT |
                          FILE_SYNCHRONOUS_IO_NONALERT;
    HANDLE file = INVALID_HANDLE_VALUE;
    NTSTATUS status = minizip_create_relative(directory, name, access, 0,
                                              FILE_OPEN, options, &file);
    int created = 0;
    BY_HANDLE_FILE_INFORMATION info;
    LARGE_INTEGER beginning;
    if (status == (NTSTATUS)0xC0000034L || status == (NTSTATUS)0xC000003AL) {
        status = minizip_create_relative(directory, name, access,
                                         FILE_ATTRIBUTE_NORMAL,
                                         FILE_CREATE, options, &file);
        if (status >= 0)
            created = 1;
        else if (status == (NTSTATUS)0xC0000035L)
            status = minizip_create_relative(directory, name, access, 0,
                                             FILE_OPEN, options, &file);
    }
    if (status < 0) {
        minizip_set_nt_errno(status);
        return INVALID_HANDLE_VALUE;
    }
    if (!GetFileInformationByHandle(file, &info) ||
        (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT |
                                  FILE_ATTRIBUTE_DIRECTORY |
                                  FILE_ATTRIBUTE_DEVICE)) != 0) {
        CloseHandle(file);
        errno = EACCES;
        return INVALID_HANDLE_VALUE;
    }
    if (!created) {
        beginning.QuadPart = 0;
        if (!SetFilePointerEx(file, beginning, NULL, FILE_BEGIN) ||
            !SetEndOfFile(file)) {
            CloseHandle(file);
            errno = EACCES;
            return INVALID_HANDLE_VALUE;
        }
    }
    return file;
}

static FILE *minizip_open_output_file_windows(const char *path) {
    const char *last_separator = NULL;
    const char *scan;
    size_t directory_length, name_length;
    char *directory_path;
    WCHAR *wide_name;
    UNICODE_STRING name;
    HANDLE directory, file;
    intptr_t descriptor;
    FILE *stream;
    for (scan = path; *scan != '\0'; scan++)
        if (*scan == '/' || *scan == '\\')
            last_separator = scan;
    directory_length = last_separator == NULL ? 0 : (size_t)(last_separator - path);
    name_length = strlen(last_separator == NULL ? path : last_separator + 1);
    if (name_length == 0) {
        errno = EINVAL;
        return NULL;
    }
    directory_path = (char *)malloc(directory_length + 1);
    if (directory_path == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    memcpy(directory_path, path, directory_length);
    directory_path[directory_length] = '\0';
    directory = minizip_open_output_directory(directory_path);
    free(directory_path);
    if (directory == INVALID_HANDLE_VALUE)
        return NULL;
    if (minizip_windows_component(last_separator == NULL ? path : last_separator + 1,
                                  name_length, &wide_name, &name) != 0) {
        CloseHandle(directory);
        return NULL;
    }
    file = minizip_open_output_file_handle(directory, &name);
    free(wide_name);
    CloseHandle(directory);
    if (file == INVALID_HANDLE_VALUE)
        return NULL;
    descriptor = _open_osfhandle((intptr_t)file, _O_BINARY | _O_WRONLY);
    if (descriptor == -1) {
        CloseHandle(file);
        return NULL;
    }
    stream = _fdopen((int)descriptor, "wb");
    if (stream == NULL)
        _close((int)descriptor);
    return stream;
}

#elif defined(SAFE_POSIX_EXTRACTION)
/* Resolve each directory through an open descriptor without following symlinks.
   Checking a path with lstat() before fopen() would leave a race. */
static int open_output_directory(char *path) {
    int dir = open(".", O_RDONLY | O_DIRECTORY);
    char *part = path;
    if (dir < 0)
        return -1;
    while (*part != '\0') {
        char *sep = strchr(part, '/');
        int next;
        if (sep != NULL)
            *sep = '\0';
        if (*part != '\0') {
            if (strcmp(part, "..") == 0) {
                close(dir);
                errno = EINVAL;
                return -1;
            }
            next = openat(dir, part, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
            if (next < 0 && errno == ENOENT) {
                if (mkdirat(dir, part, 0775) != 0 && errno != EEXIST) {
                    close(dir);
                    return -1;
                }
                next = openat(dir, part, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
            }
            close(dir);
            if (next < 0)
                return -1;
            dir = next;
        }
        if (sep == NULL)
            break;
        part = sep + 1;
    }
    return dir;
}

static FILE *open_output_file(const char *path) {
    char *copy = (char *)malloc(strlen(path) + 1);
    char *sep;
    int dir, fd;
    FILE *out;
    if (copy == NULL)
        return NULL;
    memcpy(copy, path, strlen(path) + 1);
    sep = strrchr(copy, '/');
    if (sep != NULL) {
        *sep = '\0';
        dir = open_output_directory(copy);
        sep++;
    }
    else {
        char empty[] = "";
        dir = open_output_directory(empty);
        sep = copy;
    }
    fd = dir < 0 ? -1 : openat(dir, sep,
                              O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0666);
    if (dir >= 0)
        close(dir);
    free(copy);
    if (fd < 0)
        return NULL;
    out = fdopen(fd, "wb");
    if (out == NULL)
        close(fd);
    return out;
}
#else
# define open_output_file(path) FOPEN_FUNC(path, "wb")
#endif

#ifdef _WIN32
# define open_output_file(path) minizip_open_output_file_windows(path)
#endif


/* mymkdir and change_file_date are not 100 % portable
   As I don't know well Unix, I wait feedback for the unix portion */

static int mymkdir(const char* dirname) {
    int ret=0;
#ifdef _WIN32
    HANDLE dir;
    if (dirname[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    dir = minizip_open_output_directory(dirname);
    if (dir == INVALID_HANDLE_VALUE)
        return -1;
    CloseHandle(dir);
    ret = 0;
#elif defined(__unix__) || defined(__unix) || defined(__APPLE__)
#ifdef SAFE_POSIX_EXTRACTION
    char *copy = (char *)malloc(strlen(dirname) + 1);
    if (copy == NULL)
        return -1;
    memcpy(copy, dirname, strlen(dirname) + 1);
    ret = open_output_directory(copy);
    free(copy);
    if (ret < 0)
        return -1;
    close(ret);
    ret = 0;
#else
    ret = mkdir (dirname,0775);
#endif
#else
    (void)dirname;
#endif
    return ret;
}

static int makedir(const char *newdir) {
  char *buffer ;
  char *p;
  size_t len = strlen(newdir);

  if (len == 0)
    return 0;

  buffer = (char*)malloc(len+1);
        if (buffer==NULL)
        {
                printf("Error allocating memory\n");
                return UNZ_INTERNALERROR;
        }
  memcpy(buffer,newdir,len+1);

  if (buffer[len-1] == '/') {
    buffer[len-1] = '\0';
  }
  if (mymkdir(buffer) == 0)
    {
      free(buffer);
      return 1;
    }

  p = buffer+1;
  while (1)
    {
      char hold;

      while(*p && *p != '\\' && *p != '/')
        p++;
      hold = *p;
      *p = 0;
      if ((mymkdir(buffer) == -1) && (errno == ENOENT))
        {
          printf("couldn't create directory %s\n",buffer);
          free(buffer);
          return 0;
        }
      if (hold == 0)
        break;
      *p++ = hold;
    }
  free(buffer);
  return 1;
}

/* Copy to out a lexically relative path name. Filesystem operations must also
   avoid following symlinks. Both '/' and '\\' are taken as separators. Empty and
   "." path components are removed, and a ".." component is removed along with
   the component before it, if any. On Windows, drive specifiers at the start
   and components of only dots and spaces are also removed. The result is
   always a relative path, and is no longer than name. out must have room for
   strlen(name) + 1 characters. */
static void safe_path(char* out, const char* name) {
    char* start = out;
    while (*name != '\0')
    {
        const char* comp = name;
        size_t len;
        while (*name != '\0' && *name != '/' && *name != '\\')
            name++;
        len = (size_t)(name - comp);
#ifdef _WIN32
        while (out == start && len >= 2 && comp[1] == ':')
        {
            comp += 2;          /* remove drive specifiers, e.g. "C:" */
            len -= 2;
        }
        {
            /* Windows drops trailing dots and spaces from a name, so remove
               any other component made of only those, such as "..." */
            size_t i = 0;
            while (i < len && (comp[i] == '.' || comp[i] == ' '))
                i++;
            if (i == len && !(len == 2 && comp[0] == '.' && comp[1] == '.'))
                len = 0;
        }
#endif
        if (len == 2 && comp[0] == '.' && comp[1] == '.')
        {
            /* remove the previous component and its separator, if any */
            if (out > start)
                out--;
            while (out > start && out[-1] != '/' && out[-1] != '\\')
                out--;
        }
        else if (len != 0 && !(len == 1 && comp[0] == '.'))
        {
            memcpy(out, comp, len);
            out += len;
            if (*name != '\0')
                *out++ = *name;     /* keep the separator */
        }
        if (*name != '\0')
            name++;
    }
    *out = '\0';
}

static void do_banner(void) {
    printf("MiniUnz 1.1, demo of zLib + Unz package written by Gilles Vollant\n");
    printf("more info at https://www.winimage.com/zLibDll/unzip.html\n\n");
}

static void do_help(void) {
    printf("Usage : miniunz [-e] [-x] [-v] [-l] [-o] [-p password] file.zip [file_to_extr.] [-d extractdir]\n\n" \
           "  -e  Extract without pathname (junk paths)\n" \
           "  -x  Extract with pathname\n" \
           "  -v  list files\n" \
           "  -l  list files\n" \
           "  -d  directory to extract into\n" \
           "  -o  overwrite files without prompting\n" \
           "  -p  extract encrypted file using password\n\n");
}

static void Display64BitsSize(ZPOS64_T n, int size_char) {
  /* to avoid compatibility problem , we do here the conversion */
  char number[21];
  int offset=19;
  int pos_string = 19;
  number[20]=0;
  for (;;) {
      number[offset]=(char)((n%10)+'0');
      if (number[offset] != '0')
          pos_string=offset;
      n/=10;
      if (offset==0)
          break;
      offset--;
  }
  {
      int size_display_string = 19-pos_string;
      while (size_char > size_display_string)
      {
          size_char--;
          printf(" ");
      }
  }

  printf("%s",&number[pos_string]);
}

static int do_list(unzFile uf) {
    uLong i;
    unz_global_info64 gi;
    int err;

    err = unzGetGlobalInfo64(uf,&gi);
    if (err!=UNZ_OK)
        printf("error %d with zipfile in unzGetGlobalInfo\n",err);
    printf("  Length  Method     Size Ratio   Date    Time   CRC-32     Name\n");
    printf("  ------  ------     ---- -----   ----    ----   ------     ----\n");
    for (i=0;i<gi.number_entry;i++)
    {
        char filename_inzip[65536+1];
        unz_file_info64 file_info;
        uLong ratio=0;
        const char *string_method = "";
        char charCrypt=' ';
        err = unzGetCurrentFileInfo64(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
        if (err!=UNZ_OK)
        {
            printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
            break;
        }
        if (file_info.uncompressed_size>0)
            ratio = (uLong)((file_info.compressed_size*100)/file_info.uncompressed_size);

        /* display a '*' if the file is encrypted */
        if ((file_info.flag & 1) != 0)
            charCrypt='*';

        if (file_info.compression_method==0)
            string_method="Stored";
        else
        if (file_info.compression_method==Z_DEFLATED)
        {
            uInt iLevel=(uInt)((file_info.flag & 0x6)/2);
            if (iLevel==0)
              string_method="Defl:N";
            else if (iLevel==1)
              string_method="Defl:X";
            else        /* iLevel==2 || iLevel==3 */
              string_method="Defl:F"; /* 2:fast , 3 : extra fast*/
        }
        else
        if (file_info.compression_method==Z_BZIP2ED)
        {
              string_method="BZip2 ";
        }
        else
            string_method="Unkn. ";

        Display64BitsSize(file_info.uncompressed_size,7);
        printf("  %6s%c",string_method,charCrypt);
        Display64BitsSize(file_info.compressed_size,7);
        printf(" %3lu%%  %2.2lu-%2.2lu-%2.2lu  %2.2lu:%2.2lu  %8.8lx   %s\n",
                ratio,
                (uLong)file_info.tmu_date.tm_mon + 1,
                (uLong)file_info.tmu_date.tm_mday,
                (uLong)file_info.tmu_date.tm_year % 100,
                (uLong)file_info.tmu_date.tm_hour,(uLong)file_info.tmu_date.tm_min,
                (uLong)file_info.crc,filename_inzip);
        if ((i+1)<gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err!=UNZ_OK)
            {
                printf("error %d with zipfile in unzGoToNextFile\n",err);
                break;
            }
        }
    }

    return 0;
}


static int do_extract_currentfile(unzFile uf, const int* popt_extract_without_path, int* popt_overwrite, const char* password) {
    char filename_inzip[65536+1];
    char write_name[65536+1];
    char* filename_withoutpath;
    char* p;
    int err=UNZ_OK;
    FILE *fout=NULL;
    void* buf;
    uInt size_buf;

    unz_file_info64 file_info;
    err = unzGetCurrentFileInfo64(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

    if (err!=UNZ_OK)
    {
        printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
        return err;
    }

    size_buf = WRITEBUFFERSIZE;
    buf = (void*)malloc(size_buf);
    if (buf==NULL)
    {
        printf("Error allocating memory\n");
        return UNZ_INTERNALERROR;
    }

    p = filename_withoutpath = filename_inzip;
    while ((*p) != '\0')
    {
        if (((*p)=='/') || ((*p)=='\\'))
            filename_withoutpath = p+1;
        p++;
    }

    if ((*filename_withoutpath)=='\0')
    {
        safe_path(write_name, filename_inzip);
        if (((*popt_extract_without_path)==0) && (write_name[0]!='\0'))
        {
            printf("creating directory: %s\n",write_name);
            if (mymkdir(write_name) != 0 && errno != EEXIST)
                err = UNZ_ERRNO;
        }
    }
    else
    {
        const char* write_filename = write_name;
        int skip=0;

        if ((*popt_extract_without_path)==0)
            safe_path(write_name, filename_inzip);
        else
            safe_path(write_name, filename_withoutpath);
        if (write_name[0]=='\0')
        {
            printf("skipping %s: no name left to extract to\n",filename_inzip);
            skip = 1;
        }

        err = unzOpenCurrentFilePassword(uf,password);
        if (err!=UNZ_OK)
        {
            printf("error %d with zipfile in unzOpenCurrentFilePassword\n",err);
        }

        if (((*popt_overwrite)==0) && (err==UNZ_OK))
        {
            char rep=0;
            FILE* ftestexist;
            ftestexist = FOPEN_FUNC(write_filename,"rb");
            if (ftestexist!=NULL)
            {
                fclose(ftestexist);
                do
                {
                    char answer[128];
                    int ret;

                    printf("The file %s exists. Overwrite ? [y]es, [n]o, [A]ll: ",write_filename);
                    ret = scanf("%1s",answer);
                    if (ret != 1)
                    {
                       exit(EXIT_FAILURE);
                    }
                    rep = answer[0] ;
                    if ((rep>='a') && (rep<='z'))
                        rep -= 0x20;
                }
                while ((rep!='Y') && (rep!='N') && (rep!='A'));
            }

            if (rep == 'N')
                skip = 1;

            if (rep == 'A')
                *popt_overwrite=1;
        }

        if ((skip==0) && (err==UNZ_OK))
        {
            fout=open_output_file(write_filename);
            /* some zipfile don't contain directory alone before file */
            if ((fout==NULL) && ((*popt_extract_without_path)==0))
            {
                /* make the directories leading up to the file */
                char* sep = NULL;
                char* q;
                for (q = write_name; *q != '\0'; q++)
                    if (*q == '/' || *q == '\\')
                        sep = q;
                if (sep != NULL)
                {
                    char c = *sep;
                    *sep = '\0';
                    makedir(write_name);
                    *sep = c;
                    fout=open_output_file(write_filename);
                }
            }

            if (fout==NULL)
            {
                printf("error opening %s\n",write_filename);
                err=UNZ_ERRNO;
            }
        }

        if (fout!=NULL)
        {
            printf(" extracting: %s\n",write_filename);

            do
            {
                err = unzReadCurrentFile(uf,buf,size_buf);
                if (err<0)
                {
                    printf("error %d with zipfile in unzReadCurrentFile\n",err);
                    break;
                }
                if (err>0)
                    if (fwrite(buf,(unsigned)err,1,fout)!=1)
                    {
                        printf("error in writing extracted file\n");
                        err=UNZ_ERRNO;
                        break;
                    }
            }
            while (err>0);
            if (err==0 && fflush(fout) != 0)
                err=UNZ_ERRNO;
#ifdef SAFE_POSIX_EXTRACTION
            if (err==0)
                change_file_date(fout,write_filename,file_info.dosDate,
                                 file_info.tmu_date);
#elif defined(_WIN32)
            if (err==0)
                change_file_date(fout,write_filename,file_info.dosDate,
                                 file_info.tmu_date);
#endif
            if (fclose(fout) != 0 && err == UNZ_OK)
                err = UNZ_ERRNO;
#if !defined(SAFE_POSIX_EXTRACTION) && !defined(_WIN32)
            if (err==0)
                change_file_date(NULL,write_filename,file_info.dosDate,
                                 file_info.tmu_date);
#endif
        }

        if (err==UNZ_OK)
        {
            err = unzCloseCurrentFile (uf);
            if (err!=UNZ_OK)
            {
                printf("error %d with zipfile in unzCloseCurrentFile\n",err);
            }
        }
        else
            unzCloseCurrentFile(uf); /* don't lose the error */
    }

    free(buf);
    return err;
}


static int do_extract(unzFile uf, int opt_extract_without_path, int opt_overwrite, const char* password) {
    uLong i;
    unz_global_info64 gi;
    int err;

    err = unzGetGlobalInfo64(uf,&gi);
    if (err!=UNZ_OK)
    {
        printf("error %d with zipfile in unzGetGlobalInfo\n",err);
        return err;
    }

    for (i=0;i<gi.number_entry;i++)
    {
        err = do_extract_currentfile(uf,&opt_extract_without_path,
                                      &opt_overwrite,
                                      password);
        if (err != UNZ_OK)
            break;

        if ((i+1)<gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err!=UNZ_OK)
            {
                printf("error %d with zipfile in unzGoToNextFile\n",err);
                break;
            }
        }
    }

    return err;
}

static int do_extract_onefile(unzFile uf, const char* filename, int opt_extract_without_path, int opt_overwrite, const char* password) {
    if (unzLocateFile(uf,filename,CASESENSITIVITY)!=UNZ_OK)
    {
        printf("file %s not found in the zipfile\n",filename);
        return 2;
    }

    return do_extract_currentfile(uf,&opt_extract_without_path,
                                      &opt_overwrite,
                                      password);
}


int main(int argc, char *argv[]) {
    const char *zipfilename=NULL;
    const char *filename_to_extract=NULL;
    const char *password=NULL;
    char filename_try[MAXFILENAME+16] = "";
    int i;
    int ret_value=0;
    int opt_do_list=0;
    int opt_do_extract=1;
    int opt_do_extract_withoutpath=0;
    int opt_overwrite=0;
    int opt_extractdir=0;
    const char *dirname=NULL;
    unzFile uf=NULL;

    do_banner();
    if (argc==1)
    {
        do_help();
        return 0;
    }
    else
    {
        for (i=1;i<argc;i++)
        {
            if ((*argv[i])=='-')
            {
                const char *p=argv[i]+1;

                while ((*p)!='\0')
                {
                    char c=*(p++);
                    if ((c=='l') || (c=='L'))
                        opt_do_list = 1;
                    if ((c=='v') || (c=='V'))
                        opt_do_list = 1;
                    if ((c=='x') || (c=='X'))
                        opt_do_extract = 1;
                    if ((c=='e') || (c=='E'))
                        opt_do_extract = opt_do_extract_withoutpath = 1;
                    if ((c=='o') || (c=='O'))
                        opt_overwrite=1;
                    if ((c=='d') || (c=='D'))
                    {
                        opt_extractdir=1;
                        dirname=argv[i+1];
                    }

                    if (((c=='p') || (c=='P')) && (i+1<argc))
                    {
                        password=argv[i+1];
                        i++;
                    }
                }
            }
            else
            {
                if (zipfilename == NULL)
                    zipfilename = argv[i];
                else if ((filename_to_extract==NULL) && (!opt_extractdir))
                        filename_to_extract = argv[i] ;
            }
        }
    }

    if (zipfilename!=NULL)
    {

#        ifdef USEWIN32IOAPI
        zlib_filefunc64_def ffunc;
#        endif

        strncpy(filename_try, zipfilename,MAXFILENAME-1);
        /* strncpy doesn't append the trailing NULL, of the string is too long. */
        filename_try[ MAXFILENAME ] = '\0';

#        ifdef USEWIN32IOAPI
        fill_win32_filefunc64A(&ffunc);
        uf = unzOpen2_64(zipfilename,&ffunc);
#        else
        uf = unzOpen64(zipfilename);
#        endif
        if (uf==NULL)
        {
            strcat(filename_try,".zip");
#            ifdef USEWIN32IOAPI
            uf = unzOpen2_64(filename_try,&ffunc);
#            else
            uf = unzOpen64(filename_try);
#            endif
        }
    }

    if (uf==NULL)
    {
        printf("Cannot open %s or %s.zip\n",zipfilename,zipfilename);
        return 1;
    }
    printf("%s opened\n",filename_try);

    if (opt_do_list==1)
        ret_value = do_list(uf);
    else if (opt_do_extract==1)
    {
#ifdef _WIN32
        if (opt_extractdir && _chdir(dirname))
#else
        if (opt_extractdir && chdir(dirname))
#endif
        {
          printf("Error changing into %s, aborting\n", dirname);
          exit(-1);
        }

        if (filename_to_extract == NULL)
            ret_value = do_extract(uf, opt_do_extract_withoutpath, opt_overwrite, password);
        else
            ret_value = do_extract_onefile(uf, filename_to_extract, opt_do_extract_withoutpath, opt_overwrite, password);
    }

    unzClose(uf);

    return ret_value;
}
