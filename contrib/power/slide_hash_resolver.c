/* Copyright (C) 2019 Matheus Castanho <msc@linux.ibm.com>, IBM
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "../gcc/zifunc.h"
#include "power.h"

Z_IFUNC(slide_hash) {
#ifdef Z_POWER8
    if (__builtin_cpu_supports("arch_2_07"))
        return _slide_hash_power8;
#endif

    return slide_hash_default;
}
