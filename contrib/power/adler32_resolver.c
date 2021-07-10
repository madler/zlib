/* Copyright (C) 2019 Rogerio Alves <rcardoso@linux.ibm.com>, IBM
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "../gcc/zifunc.h"
#include "power.h"

Z_IFUNC(adler32) {
#ifdef Z_POWER8
    if (__builtin_cpu_supports("arch_2_07"))
        return _adler32_power8;
#endif

    return adler32_default;
}
