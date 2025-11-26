#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "zlib.h"

#define CHUNK 16384  /* 한 번에 읽고 쓰는 버퍼 크기 */

/* build_extra: gzip 헤더의 extra 필드(FEXTRA 영역)에 AU(Author), DT(Date) 메타데이터를 작성하는 함수 */

static size_t build_extra(unsigned char *buf, size_t max,
                          const char *author, const char *date)
{
    unsigned char *p = buf;
    uint16_t xlen = 0;

    if (max < 2) {
        return 0;
    }

    // 나중에 XLEN을 채우기 위해 2바이트 비워둠
    p += 2;

    // 1) AU 필드: 작성자 정보
    if (author && author[0] != '\0') {
        size_t len = strlen(author);
        if (max - (size_t)(p - buf) >= 4 + len) {
            *p++ = 'A';
            *p++ = 'U';
            uint16_t L = (uint16_t)len;
            *p++ = (unsigned char)(L & 0xff);      // 길이 low byte
            *p++ = (unsigned char)((L >> 8) & 0xff); // 길이 high byte
            memcpy(p, author, len);
            p += len;
            xlen += (uint16_t)(4 + len);
        }
    }

    // 2) DT 필드: 날짜 정보
    if (date && date[0] != '\0') {
        size_t len = strlen(date);
        if (max - (size_t)(p - buf) >= 4 + len) {
            *p++ = 'D';
            *p++ = 'T';
            uint16_t L = (uint16_t)len;
            *p++ = (unsigned char)(L & 0xff);
            *p++ = (unsigned char)((L >> 8) & 0xff);
            memcpy(p, date, len);
            p += len;
            xlen += (uint16_t)(4 + len);
        }
    }

    // 맨 앞 2바이트에 XLEN 값 채워 넣기 (little endian)
    buf[0] = (unsigned char)(xlen & 0xff);
    buf[1] = (unsigned char)((xlen >> 8) & 0xff);

    return (size_t)(p - buf);  // extra 전체 길이 (XLEN 포함)
}


/* deflate_with_meta: source 파일을 gzip 포맷으로 압축하면서 gzip 헤더에 meta(author, date)를 넣는 함수 */
static int deflate_with_meta(FILE *source, FILE *dest,
                             const char *author, const char *date)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    // z_stream 초기화
    memset(&strm, 0, sizeof(strm));
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    // 15 + 16 : 15비트 윈도우 + gzip 사용 플래그(16) → gzip 포맷으로 압축
    ret = deflateInit2(&strm,
                       Z_DEFAULT_COMPRESSION,
                       Z_DEFLATED,
                       15 + 16,
                       8,
                       Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        return ret;
    }

    // gzip 헤더 설정
    gz_header header;
    memset(&header, 0, sizeof(header));

    header.os = 3;  // 운영체제 코드(3 = UNIX)

    // extra 버퍼에 메타데이터 채우기
    unsigned char extra[256];
    size_t extra_len = build_extra(extra, sizeof(extra), author, date);

    if (extra_len > 0) {
        header.extra     = extra;
        header.extra_len = (uInt)extra_len;
        header.extra_max = (uInt)extra_len;
    }

    // 헤더를 스트림에 세팅
    // 이렇게 하면 FEXTRA 비트가 켜지고 우리가 만든 extra field가 gzip 헤더에 들어감
    ret = deflateSetHeader(&strm, &header);
    if (ret != Z_OK) {
        deflateEnd(&strm);
        return ret;
    }

    // deflate 루프 
    int flush;
    do {
        strm.avail_in = (uInt)fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK;
            strm.next_out  = out;

            ret = deflate(&strm, flush);
            if (ret == Z_STREAM_ERROR) {
                deflateEnd(&strm);
                return ret;
            }

            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

    } while (flush != Z_FINISH);

    deflateEnd(&strm);
    return Z_OK;
}

/* main 사용 방법: minigzip_meta <input> <output.gz> [author] [date] */
int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr,
                "usage: %s <input> <output.gz> [author] [date]\n",
                argv[0]);
        return 1;
    }

    const char *in_name  = argv[1];
    const char *out_name = argv[2];
    const char *author   = (argc >= 4) ? argv[3] : "unknown";
    const char *date     = (argc >= 5) ? argv[4] : "unknown";

    FILE *in  = fopen(in_name, "rb");
    if (!in) {
        perror("open input");
        return 1;
    }
    FILE *out = fopen(out_name, "wb");
    if (!out) {
        perror("open output");
        fclose(in);
        return 1;
    }

    // 실제 압축 + 메타데이터 삽입 작업 호출
    int ret = deflate_with_meta(in, out, author, date);

    fclose(in);
    fclose(out);

    if (ret != Z_OK) {
        fprintf(stderr, "compression failed: %d\n", ret);
        return 1;
    }

    printf("created %s with author='%s', date='%s'\n",
           out_name, author, date);
    return 0;
}