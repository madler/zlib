#ifndef X86_HASH_INC
#define X86_HASH_INC

#ifndef HAS_SSE42
#error need SSE 4.2 hw support
#endif

#include <x86intrin.h>

__attribute__ ((always_inline)) local uInt
x86_compute_hash(deflate_state *s, uInt h, const unsigned char *str) {
    uInt v = 0xffffff & *(unsigned int*)str;
    uInt hash = _mm_crc32_u32(0, v);
    return hash & s->hash_mask;
}

#define UPDATE_HASH(s,h,str) (h = x86_compute_hash(s, h, str - 2))
#define INIT_HASH

#endif
