#include "zlib.h"
#include <stdio.h> // snprintf 사용
#include <string.h> // strlen 사용
#include <errno.h>  // Z_ERRNO 처리 - C 표준 라이브러리의 errno를 사용하여 OS 오류 메시지 추가

// 정적 버퍼를 사용하여 반환 문자열을 저장.
static char zlib_error_buffer[520]; 

/*
 * Zlib 오류 코드와 z_stream 정보를 통합, 오류 메시지 문자열을 반환.
 */
ZEXTERN const char * ZEXPORT zlib_get_full_error(int err_code, z_streamp strm) {
    // 버퍼 초기화
    zlib_error_buffer[0] = '\0';
    
    // 기본 오류 유형 메시지 (zError의 역할)
    const char *base_msg = ZEXPORT zError(err_code);
    
    // 템플릿에 맞게 기본 정보 삽입: "[Zlib Error Code -2] stream error"
    int written = snprintf(zlib_error_buffer, sizeof(zlib_error_buffer), 
                           "[Zlib Error Code %d] - %s.\n", 
                           err_code, base_msg);
    
    // 상세 메시지 (strm->msg의 역할) 추가
    // strm 포인터 자체가 유효한 메모리 주소를 가리키고 있는가? & strm 구조체의 msg 멤버가 실제로 오류 메시지 문자열을 가리키고 있는가?
    // 이 부분으로 CHECK_ERR에서 양식에 맞춘 문자열 fprint문 작성의 번거로움을 제거.
    if (strm != Z_NULL && strm->msg != Z_NULL) {
        written += snprintf(zlib_error_buffer + written, sizeof(zlib_error_buffer) - written, 
                           "Details: %s.\n", strm->msg);
    }
    
    // 파일 시스템 오류 (Z_ERRNO) 특별 처리 추가 (gzerror의 Z_ERRNO 로직 확장)
    if (err_code == Z_ERRNO) {
        written += snprintf(zlib_error_buffer + written, sizeof(zlib_error_buffer) - written, 
                           "OS Error: %s.\n", strerror(errno)); 
    }

    // 만약 버퍼에 아무것도 쓰이지 않았다면 (불가능한 오류 코드), 기본 메시지 반환
    // 이 함수는 ret이 Z_OK아닐 때만 호출이 된다고 가정하므로 버퍼에 아무것도 쓰이지 않았다면 다른 오류라는 것이 확정. 
    // (이 함수를 쓰려면 약간의 사용자의 전처리가 필요하긴 하다는 말...)
    if (zlib_error_buffer[0] == '\0') {
        return "Unknown Zlib Error";
    }
    
    //통합된 문자열 반환
    return zlib_error_buffer;
}