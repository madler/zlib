#include <sys/auxv.h>
#include "../gcc/zifunc.h"

#define VX_MIN_LEN 64
#define VX_ALIGNMENT 16L
#define VX_ALIGN_MASK (VX_ALIGNMENT - 1)

unsigned int crc32_le_vgfm_16(unsigned int crc, const unsigned char FAR *buf, z_size_t len);

local unsigned long s390_crc32_vx(unsigned long crc, const unsigned char FAR *buf, z_size_t len)
{
    uintptr_t prealign, aligned, remaining;

    if (buf == Z_NULL) return 0UL;

    if (len < VX_MIN_LEN + VX_ALIGN_MASK)
        return crc32_z_default(crc, buf, len);

    if ((uintptr_t)buf & VX_ALIGN_MASK) {
        prealign = VX_ALIGNMENT - ((uintptr_t)buf & VX_ALIGN_MASK);
        len -= prealign;
        crc = crc32_z_default(crc, buf, prealign);
        buf += prealign;
    }
    aligned = len & ~VX_ALIGN_MASK;
    remaining = len & VX_ALIGN_MASK;

    crc = crc32_le_vgfm_16(crc ^ 0xffffffff, buf, (size_t)aligned) ^ 0xffffffff;

    if (remaining)
        crc = crc32_z_default(crc, buf + aligned, remaining);

    return crc;
}

Z_IFUNC(crc32_z)
{
    if (hwcap & HWCAP_S390_VX)
        return s390_crc32_vx;
    return crc32_z_default;
}
