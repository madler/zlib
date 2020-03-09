/* adler32_test.c -- unit test for adler32 in the zlib compression library
 * Copyright (C) 2019 Rogerio Alves <rcardoso@linux.ibm.com>, IBM
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zlib.h"
#include <stdio.h>

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#endif

void test_adler32  OF((uLong adler, Byte* buf, z_size_t len, uLong chk, int line));
int main           OF((void));

typedef struct {
    int line;
    uLong adler;
    char* buf;
    int len;
    uLong expect;
} adler32_test;

void test_adler32(adler, buf, len, chk, line)
    uLong adler;
    Byte *buf;
    z_size_t len;
    uLong chk;
    int line;
{
    uLong res = adler32(adler, buf, len);
    if (res != chk) {
        fprintf(stderr, "FAIL [%d]: adler32 returned 0x%08X expected 0x%08X\n",
                line, (unsigned int)res, (unsigned int)chk);
        exit(1);
    }
}

static const adler32_test tests[] = {
   {__LINE__,0x1, 0x0, 0, 0x1},
   {__LINE__,0x1, "", 1, 0x10001},
   {__LINE__,0x1, "a", 1, 0x620062},
   {__LINE__,0x1, "abacus", 6, 0x8400270},
   {__LINE__,0x1, "backlog", 7, 0xb1f02d4},
   {__LINE__,0x1, "campfire", 8, 0xea10348},
   {__LINE__,0x1, "delta", 5, 0x61a020b},
   {__LINE__,0x1, "executable", 10, 0x16fa0423},
   {__LINE__,0x1, "file", 4, 0x41401a1},
   {__LINE__,0x1, "greatest", 8, 0xefa0360},
   {__LINE__,0x1, "inverter", 8, 0xf6f0370},
   {__LINE__,0x1, "jigsaw", 6, 0x8bd0286},
   {__LINE__,0x1, "karate", 6, 0x8a50279},
   {__LINE__,0x1, "landscape", 9, 0x126a03ac},
   {__LINE__,0x1, "machine", 7, 0xb5302d6},
   {__LINE__,0x1, "nanometer", 9, 0x12d803ca},
   {__LINE__,0x1, "oblivion", 8, 0xf220363},
   {__LINE__,0x1, "panama", 6, 0x8a1026f},
   {__LINE__,0x1, "quest", 5, 0x6970233},
   {__LINE__,0x1, "resource", 8, 0xf8d0369},
   {__LINE__,0x1, "secret", 6, 0x8d10287},
   {__LINE__,0x1, "ultimate", 8, 0xf8d0366},
   {__LINE__,0x1, "vector", 6, 0x8fb0294},
   {__LINE__,0x1, "walrus", 6, 0x918029f},
   {__LINE__,0x1, "xeno", 4, 0x45e01bb},
   {__LINE__,0x1, "yelling", 7, 0xbfe02f5},
   {__LINE__,0x1, "zero", 4, 0x46e01c1},
   {__LINE__,0x1, "4BJD7PocN1VqX0jXVpWB", 20, 0x3eef064d},
   {__LINE__,0x1, "F1rPWI7XvDs6nAIRx41l", 20, 0x425d065f},
   {__LINE__,0x1, "ldhKlsVkPFOveXgkGtC2", 20, 0x4f1a073e},
   {__LINE__,0x1, "5KKnGOOrs8BvJ35iKTOS", 20, 0x42290650},
   {__LINE__,0x1, "0l1tw7GOcem06Ddu7yn4", 20, 0x43fd0690},
   {__LINE__,0x1, "MCr47CjPIn9R1IvE1Tm5", 20, 0x3f770609},
   {__LINE__,0x1, "UcixbzPKTIv0SvILHVdO", 20, 0x4c7c0703},
   {__LINE__,0x1, "dGnAyAhRQDsWw0ESou24", 20, 0x48ac06b7},
   {__LINE__,0x1, "di0nvmY9UYMYDh0r45XT", 20, 0x489a0698},
   {__LINE__,0x1, "2XKDwHfAhFsV0RhbqtvH", 20, 0x44a906e6},
   {__LINE__,0x1, "ZhrANFIiIvRnqClIVyeD", 20, 0x4a29071c},
   {__LINE__,0x1, "v7Q9ehzioTOVeDIZioT1", 20, 0x4a7706f9},
   {__LINE__,0x1, "Yod5hEeKcYqyhfXbhxj2", 20, 0x4ce60769},
   {__LINE__,0x1, "GehSWY2ay4uUKhehXYb0", 20, 0x48ae06e5},
   {__LINE__,0x1, "kwytJmq6UqpflV8Y8GoE", 20, 0x51d60750},
   {__LINE__,0x1, "70684206568419061514", 20, 0x2b100414},
   {__LINE__,0x1, "42015093765128581010", 20, 0x2a550405},
   {__LINE__,0x1, "88214814356148806939", 20, 0x2b450423},
   {__LINE__,0x1, "43472694284527343838", 20, 0x2b460421},
   {__LINE__,0x1, "49769333513942933689", 20, 0x2bc1042b},
   {__LINE__,0x1, "54979784887993251199", 20, 0x2ccd043d},
   {__LINE__,0x1, "58360544869206793220", 20, 0x2b68041a},
   {__LINE__,0x1, "27347953487840714234", 20, 0x2b84041d},
   {__LINE__,0x1, "07650690295365319082", 20, 0x2afa0417},
   {__LINE__,0x1, "42655507906821911703", 20, 0x2aff0412},
   {__LINE__,0x1, "29977409200786225655", 20, 0x2b8d0420},
   {__LINE__,0x1, "85181542907229116674", 20, 0x2b140419},
   {__LINE__,0x1, "87963594337989416799", 20, 0x2c8e043f},
   {__LINE__,0x1, "21395988329504168551", 20, 0x2b68041f},
   {__LINE__,0x1, "51991013580943379423", 20, 0x2af10417},
   {__LINE__,0x1, "*]+@!);({_$;}[_},?{?;(_?,=-][@", 30, 0x7c9d0841},
   {__LINE__,0x1, "_@:_).&(#.[:[{[:)$++-($_;@[)}+", 30, 0x71060751},
   {__LINE__,0x1, "&[!,[$_==}+.]@!;*(+},[;:)$;)-@", 30, 0x7095070a},
   {__LINE__,0x1, "]{.[.+?+[[=;[?}_#&;[=)__$$:+=_", 30, 0x82530815},
   {__LINE__,0x1, "-%.)=/[@].:.(:,()$;=%@-$?]{%+%", 30, 0x61250661},
   {__LINE__,0x1, "+]#$(@&.=:,*];/.!]%/{:){:@(;)$", 30, 0x642006a3},
   {__LINE__,0x1, ")-._.:?[&:.=+}(*$/=!.${;(=$@!}", 30, 0x674206cb},
   {__LINE__,0x1, ":(_*&%/[[}+,?#$&*+#[([*-/#;%(]", 30, 0x67670680},
   {__LINE__,0x1, "{[#-;:$/{)(+[}#]/{&!%(@)%:@-$:", 30, 0x7547070f},
   {__LINE__,0x1, "_{$*,}(&,@.)):=!/%(&(,,-?$}}}!", 30, 0x69ea06ee},
   {__LINE__,0x1, "e$98KNzqaV)Y:2X?]77].{gKRD4G5{mHZk,Z)SpU%L3FSgv!Wb8MLAFdi{+fp)c,@8m6v)yXg@]HBDFk?.4&}g5_udE*JHCiH=aL", 100, 0x1b01e92},
   {__LINE__,0x1, "r*Fd}ef+5RJQ;+W=4jTR9)R*p!B;]Ed7tkrLi;88U7g@3v!5pk2X6D)vt,.@N8c]@yyEcKi[vwUu@.Ppm@C6%Mv*3Nw}Y,58_aH)", 100, 0xfbdb1e96},
   {__LINE__,0x1, "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&", 100, 0x47a61ec8},
   {__LINE__,0x1, "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"
    "byRKqAu3J", 5552, 0x8b81718f},
   {__LINE__,0x7a30360d, 0x0, 0, 0x1},
   {__LINE__,0x6fd767ee, "", 1, 0xd7c567ee},
   {__LINE__,0xefeb7589, "a", 1, 0x65e475ea},
   {__LINE__,0x61cf7e6b, "abacus", 6, 0x60b880da},
   {__LINE__,0xdc712e2, "backlog", 7, 0x9d0d15b5},
   {__LINE__,0xad23c7fd, "campfire", 8, 0xfbfecb44},
   {__LINE__,0x85cb2317, "delta", 5, 0x3b622521},
   {__LINE__,0x9eed31b0, "executable", 10, 0xa6db35d2},
   {__LINE__,0xb94f34ca, "file", 4, 0x9096366a},
   {__LINE__,0xab058a2, "greatest", 8, 0xded05c01},
   {__LINE__,0x5bff2b7a, "inverter", 8, 0xc7452ee9},
   {__LINE__,0x605c9a5f, "jigsaw", 6, 0x7899ce4},
   {__LINE__,0x51bdeea5, "karate", 6, 0xf285f11d},
   {__LINE__,0x85c21c79, "landscape", 9, 0x98732024},
   {__LINE__,0x97216f56, "machine", 7, 0xadf4722b},
   {__LINE__,0x18444af2, "nanometer", 9, 0xcdb34ebb},
   {__LINE__,0xbe6ce359, "oblivion", 8, 0xe8b7e6bb},
   {__LINE__,0x843071f1, "panama", 6, 0x389e745f},
   {__LINE__,0xf2480c60, "quest", 5, 0x36c90e92},
   {__LINE__,0x2d2feb3d, "resource", 8, 0x9705eea5},
   {__LINE__,0x7490310a, "secret", 6, 0xa3a63390},
   {__LINE__,0x97d247d4, "ultimate", 8, 0xe6154b39},
   {__LINE__,0x93cf7599, "vector", 6, 0x5e87782c},
   {__LINE__,0x73c84278, "walrus", 6, 0xbc84516},
   {__LINE__,0x228a87d1, "xeno", 4, 0x4646898b},
   {__LINE__,0xa7a048d0, "yelling", 7, 0xb1654bc4},
   {__LINE__,0x1f0ded40, "zero", 4, 0xd8a4ef00},
   {__LINE__,0xa804a62f, "4BJD7PocN1VqX0jXVpWB", 20, 0xe34eac7b},
   {__LINE__,0x508fae6a, "F1rPWI7XvDs6nAIRx41l", 20, 0x33f2b4c8},
   {__LINE__,0xe5adaf4f, "ldhKlsVkPFOveXgkGtC2", 20, 0xe7b1b68c},
   {__LINE__,0x67136a40, "5KKnGOOrs8BvJ35iKTOS", 20, 0xf6a0708f},
   {__LINE__,0xb00c4a10, "0l1tw7GOcem06Ddu7yn4", 20, 0xbd8f509f},
   {__LINE__,0x2e0c84b5, "MCr47CjPIn9R1IvE1Tm5", 20, 0xcc298abd},
   {__LINE__,0x81238d44, "UcixbzPKTIv0SvILHVdO", 20, 0xd7809446},
   {__LINE__,0xf853aa92, "dGnAyAhRQDsWw0ESou24", 20, 0x9525b148},
   {__LINE__,0x5a692325, "di0nvmY9UYMYDh0r45XT", 20, 0x620029bc},
   {__LINE__,0x3275b9f, "2XKDwHfAhFsV0RhbqtvH", 20, 0x70916284},
   {__LINE__,0x38371feb, "ZhrANFIiIvRnqClIVyeD", 20, 0xd52706},
   {__LINE__,0xafc8bf62, "v7Q9ehzioTOVeDIZioT1", 20, 0xeeb4c65a},
   {__LINE__,0x9b07db73, "Yod5hEeKcYqyhfXbhxj2", 20, 0xde3e2db},
   {__LINE__,0xe75b214, "GehSWY2ay4uUKhehXYb0", 20, 0x4171b8f8},
   {__LINE__,0x72d0fe6f, "kwytJmq6UqpflV8Y8GoE", 20, 0xa66a05cd},
   {__LINE__,0xf857a4b1, "70684206568419061514", 20, 0x1f9a8c4},
   {__LINE__,0x54b8e14, "42015093765128581010", 20, 0x49c19218},
   {__LINE__,0xd6aa5616, "88214814356148806939", 20, 0xbbfc5a38},
   {__LINE__,0x11e63098, "43472694284527343838", 20, 0x93434b8},
   {__LINE__,0xbe92385, "49769333513942933689", 20, 0xfe1827af},
   {__LINE__,0x49511de0, "54979784887993251199", 20, 0xcba8221c},
   {__LINE__,0x3db13bc1, "58360544869206793220", 20, 0x14643fda},
   {__LINE__,0xbb899bea, "27347953487840714234", 20, 0x1604a006},
   {__LINE__,0xf6cd9436, "07650690295365319082", 20, 0xb69f984c},
   {__LINE__,0x9109e6c3, "42655507906821911703", 20, 0xc43eead4},
   {__LINE__,0x75770fc, "29977409200786225655", 20, 0x707751b},
   {__LINE__,0x69b1d19b, "85181542907229116674", 20, 0xf5bdd5b3},
   {__LINE__,0xc6132975, "87963594337989416799", 20, 0x2fed2db3},
   {__LINE__,0xd58cb00c, "21395988329504168551", 20, 0xc2a2b42a},
   {__LINE__,0xb63b8caa, "51991013580943379423", 20, 0xdf0590c0},
   {__LINE__,0x8a45a2b8, "*]+@!);({_$;}[_},?{?;(_?,=-][@", 30, 0x1980aaf8},
   {__LINE__,0xcbe95b78, "_@:_).&(#.[:[{[:)$++-($_;@[)}+", 30, 0xf58662c8},
   {__LINE__,0x4ef8a54b, "&[!,[$_==}+.]@!;*(+},[;:)$;)-@", 30, 0x1f65ac54},
   {__LINE__,0x76ad267a, "]{.[.+?+[[=;[?}_#&;[=)__$$:+=_", 30, 0x7b792e8e},
   {__LINE__,0x569e613c, "-%.)=/[@].:.(:,()$;=%@-$?]{%+%", 30, 0x1d61679c},
   {__LINE__,0x36aa61da, "+]#$(@&.=:,*];/.!]%/{:){:@(;)$", 30, 0x12ec687c},
   {__LINE__,0xf67222df, ")-._.:?[&:.=+}(*$/=!.${;(=$@!}", 30, 0x740329a9},
   {__LINE__,0x74b34fd3, ":(_*&%/[[}+,?#$&*+#[([*-/#;%(]", 30, 0x374c5652},
   {__LINE__,0x351fd770, "{[#-;:$/{)(+[}#]/{&!%(@)%:@-$:", 30, 0xeadfde7e},
   {__LINE__,0xc45aef77, "_{$*,}(&,@.)):=!/%(&(,,-?$}}}!", 30, 0x3fcbf664},
   {__LINE__,0xd034ea71, "e$98KNzqaV)Y:2X?]77].{gKRD4G5{mHZk,Z)SpU%L3FSgv!Wb8MLAFdi{+fp)c,@8m6v)yXg@]HBDFk?.4&}g5_udE*JHCiH=aL", 100, 0x6b080911},
   {__LINE__,0xdeadc0de, "r*Fd}ef+5RJQ;+W=4jTR9)R*p!B;]Ed7tkrLi;88U7g@3v!5pk2X6D)vt,.@N8c]@yyEcKi[vwUu@.Ppm@C6%Mv*3Nw}Y,58_aH)", 100, 0x355fdf73},
   {__LINE__,0xba5eba11, "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&", 100, 0xb48bd8d8},
   {__LINE__,0x7712aa45, "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"
    "byRKqAu3J", 5552, 0x7dc51be2},
};

static const int test_size = sizeof(tests) / sizeof(tests[0]);

int main(void)
{
    int i;
    for (i = 0; i < test_size; i++) {
        test_adler32(tests[i].adler, (Byte*) tests[i].buf, tests[i].len,
                   tests[i].expect, tests[i].line);
    }
    return 0;
}
