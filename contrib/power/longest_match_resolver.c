/* Copyright (C) 2019 Matheus Castanho <msc@linux.ibm.com>, IBM
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "../gcc/zifunc.h"
#include "power.h"

Z_IFUNC(longest_match) {
#ifdef Z_POWER9
    if (__builtin_cpu_supports("arch_3_00"))
        return _longest_match_power9;
#endif

    return longest_match_default;
}
