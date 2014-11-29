#ifndef X86_HASH_INC
#define X86_HASH_INC

#ifndef HAS_SSE42
#error need SSE 4.2 hw support
#endif

#include <x86intrin.h>

/* Perform a match on at least four bytes and therefore the hash should be for 4 bytes */
#define UPDATE_HASH(s,h,str) (h = _mm_crc32_u32(0,  *(uInt*)(str - 2)) & s->hash_mask)
#define INIT_HASH

#endif
