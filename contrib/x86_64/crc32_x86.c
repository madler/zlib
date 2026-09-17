#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define Z_ONCE
#include "../../zutil.h"
#include "crc32_x86_hooks.h"

#ifdef __x86_64__

static inline int has_osxsave(void)
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(0x01)
            : "memory"
    );

    return ((ecx >> 27) & 0x01);
}

// check XCR0 Bits
static inline uint64_t xgetbv(uint32_t ecx)
{
    uint32_t eax, edx;
    __asm__ volatile (
            "xgetbv"
            : "=a"(eax), "=d"(edx)
            : "c"(ecx)
            : "memory"
    );

    return ((uint64_t)edx << 32) | eax;
}

static inline int xcr0_ymm_enabled(void)
{
    uint64_t xcr0 = xgetbv(0);

    // Check XCR0[2:1] == '11'(YMM state enabled)
    return ((xcr0 >> 1) & 0x03) == 0x03;
}


static inline int xcr0_zmm_enabled(void) {

    uint64_t xcr0 = xgetbv(0);

    // Check XCR0[7:5] == '111'(ZMM + OPMASK state enabled)
    return ((xcr0 >> 5) & 0x07) == 0x07;
}

static inline int has_pclmul(void) {
    uint32_t eax, ebx, ecx, edx;

    __asm__ volatile (
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(0x01), "b"(0x00)
            : "memory"
    );

    return ((ecx >> 28) & 0x01) && ((ecx >> 1) & 0x01);//support avx && pclmulqdq instruction
}

static inline int has_vpclmulqdq(void)
{
    uint32_t eax, ebx, ecx, edx;
    if (!has_osxsave())
         return 0;
    if (!xcr0_ymm_enabled())
         return 0;

    if (!xcr0_zmm_enabled())
        return 0;

    __asm__ volatile (
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(0x07), "b"(0x00)
            : "memory"
    );

    return ((ebx >> 16) & 0x01) && ((ebx >> 31) & 0x01) && ((ecx >> 10) & 0x01);
}


/* SIMD optimized variants of crc32_z */
unsigned long crc32_z_impl_x86_avx(unsigned long crc, const unsigned char FAR* buf, z_size_t len); // pclmul
#if defined(__GNUC__) && (__GNUC__ >= 8)
unsigned long crc32_z_impl_x86_avx512(unsigned long crc, const unsigned char FAR* buf, z_size_t len); // vpclmulqdq
#endif

local z_once_t x86_crc32_made = Z_ONCE_INIT;
local void x86_crc32_setup() {
#if defined(__GNUC__) && (__GNUC__ >= 8)
    if (has_vpclmulqdq()) {
	crc32_z_hook = crc32_z_impl_x86_avx512;
    } else
#endif
    if (has_pclmul())
	crc32_z_hook = crc32_z_impl_x86_avx;
    else
        crc32_z_hook = crc32_z;
}

local unsigned long x86_crc32_init(unsigned long crc, const unsigned char FAR *buf, z_size_t len)
{
    z_once(&x86_crc32_made,x86_crc32_setup);
    return crc32_z_hook(crc, buf, len);
}

ZLIB_INTERNAL unsigned long (*crc32_z_hook)(unsigned long crc, const unsigned char FAR *buf, z_size_t len) = x86_crc32_init;

#endif /* end of __x86_64__ */
