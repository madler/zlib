/* crc32_tes.c -- unit test for crc32 in the zlib compression library
 * Copyright (C) 1995-2006, 2010, 2011, 2016, 2019 Rogerio Alves
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zlib.h"
#include <stdio.h>

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#endif

void test_crc32  OF((uLong crc, Byte* buf, z_size_t len, uLong chk, int line));
int main         OF((void));

typedef struct {
    int line;
	uLong crc;
    Byte* buf;
	int len;
    uLong expect;
} crc32_test;

void test_crc32(crc, buf, len, chk, line)
    uLong crc;
    Byte *buf;
    z_size_t len;
    uLong chk;
    int line;
{
    uLong res = crc32(crc, buf, len);
	if (res != chk) {
        fprintf(stderr, "FAIL [%d]: crc32 returned 0x%08X expected 0x%08X\n",
                line, (unsigned int)res, (unsigned int)chk);
        exit(1);
    }
}

static const crc32_test tests[] = {
  {__LINE__, 0x0, 0x0, 0, 0x0},
  {__LINE__, 0xffffffff, 0x0, 0, 0x0},
  {__LINE__, 0x0, 0x0, 255, 0x0}, /*  BZ 174799.  */
  {__LINE__, 0x0, 0x0, 256, 0x0},
  {__LINE__, 0x0, 0x0, 257, 0x0},
  {__LINE__, 0x0, 0x0, 32767, 0x0},
  {__LINE__, 0x0, 0x0, 32768, 0x0},
  {__LINE__, 0x0, 0x0, 32769, 0x0},
  {__LINE__, 0x0, "", 0, 0x0},
  {__LINE__, 0xffffffff, "", 0, 0xffffffff},
  {__LINE__, 0x0, "abacus", 6, 0xc3d7115b},
  {__LINE__, 0x0, "backlog", 7, 0x269205},
  {__LINE__, 0x0, "campfire", 8, 0x22a515f8},
  {__LINE__, 0x0, "delta", 5, 0x9643fed9},
  {__LINE__, 0x0, "executable", 10, 0xd68eda01},
  {__LINE__, 0x0, "file", 4, 0x8c9f3610},
  {__LINE__, 0x0, "greatest", 8, 0xc1abd6cd},
  {__LINE__, 0x0, "hello", 5, 0x3610a686},
  {__LINE__, 0x0, "inverter", 8, 0xc9e962c9},
  {__LINE__, 0x0, "jigsaw", 6, 0xce4e3f69},
  {__LINE__, 0x0, "karate", 6, 0x890be0e2},
  {__LINE__, 0x0, "landscape", 9, 0xc4e0330b},
  {__LINE__, 0x0, "machine", 7, 0x1505df84},
  {__LINE__, 0x0, "nanometer", 9, 0xd4e19f39},
  {__LINE__, 0x0, "oblivion", 8, 0xdae9de77},
  {__LINE__, 0x0, "panama", 6, 0x66b8979c},
  {__LINE__, 0x0, "quest", 5, 0x4317f817},
  {__LINE__, 0x0, "resource", 8, 0xbc91f416},
  {__LINE__, 0x0, "secret", 6, 0x5ca2e8e5},
  {__LINE__, 0x0, "test", 4, 0xd87f7e0c},
  {__LINE__, 0x0, "ultimate", 8, 0x3fc79b0b},
  {__LINE__, 0x0, "vector", 6, 0x1b6e485b},
  {__LINE__, 0x0, "walrus", 6, 0xbe769b97},
  {__LINE__, 0x0, "xeno", 4, 0xe7a06444},
  {__LINE__, 0x0, "yelling", 7, 0xfe3944e5},
  {__LINE__, 0x0, "zlib", 4, 0x73887d3a},
  {__LINE__, 0x0, "4BJD7PocN1VqX0jXVpWB", 20, 0xd487a5a1},
  {__LINE__, 0x0, "F1rPWI7XvDs6nAIRx41l", 20, 0x61a0132e},
  {__LINE__, 0x0, "ldhKlsVkPFOveXgkGtC2", 20, 0xdf02f76},
  {__LINE__, 0x0, "5KKnGOOrs8BvJ35iKTOS", 20, 0x579b2b0a},
  {__LINE__, 0x0, "0l1tw7GOcem06Ddu7yn4", 20, 0xf7d16e2d},
  {__LINE__, 0x0, "MCr47CjPIn9R1IvE1Tm5", 20, 0x731788f5},
  {__LINE__, 0x0, "UcixbzPKTIv0SvILHVdO", 20, 0x7112bb11},
  {__LINE__, 0x0, "dGnAyAhRQDsWw0ESou24", 20, 0xf32a0dac},
  {__LINE__, 0x0, "di0nvmY9UYMYDh0r45XT", 20, 0x625437bb},
  {__LINE__, 0x0, "2XKDwHfAhFsV0RhbqtvH", 20, 0x896930f9},
  {__LINE__, 0x0, "ZhrANFIiIvRnqClIVyeD", 20, 0x8579a37},
  {__LINE__, 0x0, "v7Q9ehzioTOVeDIZioT1", 20, 0x632aa8e0},
  {__LINE__, 0x0, "Yod5hEeKcYqyhfXbhxj2", 20, 0xc829af29},
  {__LINE__, 0x0, "GehSWY2ay4uUKhehXYb0", 20, 0x1b08b7e8},
  {__LINE__, 0x0, "kwytJmq6UqpflV8Y8GoE", 20, 0x4e33b192},
  {__LINE__, 0x0, "70684206568419061514", 20, 0x59a179f0},
  {__LINE__, 0x0, "42015093765128581010", 20, 0xcd1013d7},
  {__LINE__, 0x0, "88214814356148806939", 20, 0xab927546},
  {__LINE__, 0x0, "43472694284527343838", 20, 0x11f3b20c},
  {__LINE__, 0x0, "49769333513942933689", 20, 0xd562d4ca},
  {__LINE__, 0x0, "54979784887993251199", 20, 0x233395f7},
  {__LINE__, 0x0, "58360544869206793220", 20, 0x2d167fd5},
  {__LINE__, 0x0, "27347953487840714234", 20, 0x8b5108ba},
  {__LINE__, 0x0, "07650690295365319082", 20, 0xc46b3cd8},
  {__LINE__, 0x0, "42655507906821911703", 20, 0xc10b2662},
  {__LINE__, 0x0, "29977409200786225655", 20, 0xc9a0f9d2},
  {__LINE__, 0x0, "85181542907229116674", 20, 0x9341357b},
  {__LINE__, 0x0, "87963594337989416799", 20, 0xf0424937},
  {__LINE__, 0x0, "21395988329504168551", 20, 0xd7c4c31f},
  {__LINE__, 0x0, "51991013580943379423", 20, 0xf11edcc4},
  {__LINE__, 0x0, "*]+@!);({_$;}[_},?{?;(_?,=-][@", 30, 0x40795df4},
  {__LINE__, 0x0, "_@:_).&(#.[:[{[:)$++-($_;@[)}+", 30, 0xdd61a631},
  {__LINE__, 0x0, "&[!,[$_==}+.]@!;*(+},[;:)$;)-@", 30, 0xca907a99},
  {__LINE__, 0x0, "]{.[.+?+[[=;[?}_#&;[=)__$$:+=_", 30, 0xf652deac},
  {__LINE__, 0x0, "-%.)=/[@].:.(:,()$;=%@-$?]{%+%", 30, 0xaf39a5a9},
  {__LINE__, 0x0, "+]#$(@&.=:,*];/.!]%/{:){:@(;)$", 30, 0x6bebb4cf},
  {__LINE__, 0x0, ")-._.:?[&:.=+}(*$/=!.${;(=$@!}", 30, 0x76430bac},
  {__LINE__, 0x0, ":(_*&%/[[}+,?#$&*+#[([*-/#;%(]", 30, 0x6c80c388},
  {__LINE__, 0x0, "{[#-;:$/{)(+[}#]/{&!%(@)%:@-$:", 30, 0xd54d977d},
  {__LINE__, 0x0, "_{$*,}(&,@.)):=!/%(&(,,-?$}}}!", 30, 0xe3966ad5},
  {__LINE__, 0x0, "e$98KNzqaV)Y:2X?]77].{gKRD4G5{mHZk,Z)SpU%L3FSgv!Wb8MLAFdi{+fp)c,@8m6v)yXg@]HBDFk?.4&}g5_udE*JHCiH=aL", 100, 0xe7c71db9},
  {__LINE__, 0x0, "r*Fd}ef+5RJQ;+W=4jTR9)R*p!B;]Ed7tkrLi;88U7g@3v!5pk2X6D)vt,.@N8c]@yyEcKi[vwUu@.Ppm@C6%Mv*3Nw}Y,58_aH)", 100, 0xeaa52777},
  {__LINE__, 0x0, "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&", 100, 0xcd472048},
  {__LINE__, 0x7a30360d, "abacus", 6, 0xf8655a84},
  {__LINE__, 0x6fd767ee, "backlog", 7, 0x1ed834b1},
  {__LINE__, 0xefeb7589, "campfire", 8, 0x686cfca},
  {__LINE__, 0x61cf7e6b, "delta", 5, 0x1554e4b1},
  {__LINE__, 0xdc712e2, "executable", 10, 0x761b4254},
  {__LINE__, 0xad23c7fd, "file", 4, 0x7abdd09b},
  {__LINE__, 0x85cb2317, "greatest", 8, 0x4ba91c6b},
  {__LINE__, 0x9eed31b0, "inverter", 8, 0xd5e78ba5},
  {__LINE__, 0xb94f34ca, "jigsaw", 6, 0x23649109},
  {__LINE__, 0xab058a2, "karate", 6, 0xc5591f41},
  {__LINE__, 0x5bff2b7a, "landscape", 9, 0xf10eb644},
  {__LINE__, 0x605c9a5f, "machine", 7, 0xbaa0a636},
  {__LINE__, 0x51bdeea5, "nanometer", 9, 0x6af89afb},
  {__LINE__, 0x85c21c79, "oblivion", 8, 0xecae222b},
  {__LINE__, 0x97216f56, "panama", 6, 0x47dffac4},
  {__LINE__, 0x18444af2, "quest", 5, 0x70c2fe36},
  {__LINE__, 0xbe6ce359, "resource", 8, 0x1471d925},
  {__LINE__, 0x843071f1, "secret", 6, 0x50c9a0db},
  {__LINE__, 0xf2480c60, "ultimate", 8, 0xf973daf8},
  {__LINE__, 0x2d2feb3d, "vector", 6, 0x344ac03d},
  {__LINE__, 0x7490310a, "walrus", 6, 0x6d1408ef},
  {__LINE__, 0x97d247d4, "xeno", 4, 0xe62670b5},
  {__LINE__, 0x93cf7599, "yelling", 7, 0x1b36da38},
  {__LINE__, 0x73c84278, "zlib", 4, 0x6432d127},
  {__LINE__, 0x228a87d1, "4BJD7PocN1VqX0jXVpWB", 20, 0x997107d0},
  {__LINE__, 0xa7a048d0, "F1rPWI7XvDs6nAIRx41l", 20, 0xdc567274},
  {__LINE__, 0x1f0ded40, "ldhKlsVkPFOveXgkGtC2", 20, 0xdcc63870},
  {__LINE__, 0xa804a62f, "5KKnGOOrs8BvJ35iKTOS", 20, 0x6926cffd},
  {__LINE__, 0x508fae6a, "0l1tw7GOcem06Ddu7yn4", 20, 0xb52b38bc},
  {__LINE__, 0xe5adaf4f, "MCr47CjPIn9R1IvE1Tm5", 20, 0xf83b8178},
  {__LINE__, 0x67136a40, "UcixbzPKTIv0SvILHVdO", 20, 0xc5213070},
  {__LINE__, 0xb00c4a10, "dGnAyAhRQDsWw0ESou24", 20, 0xbc7648b0},
  {__LINE__, 0x2e0c84b5, "di0nvmY9UYMYDh0r45XT", 20, 0xd8123a72},
  {__LINE__, 0x81238d44, "2XKDwHfAhFsV0RhbqtvH", 20, 0xd5ac5620},
  {__LINE__, 0xf853aa92, "ZhrANFIiIvRnqClIVyeD", 20, 0xceae099d},
  {__LINE__, 0x5a692325, "v7Q9ehzioTOVeDIZioT1", 20, 0xb07d2b24},
  {__LINE__, 0x3275b9f, "Yod5hEeKcYqyhfXbhxj2", 20, 0x24ce91df},
  {__LINE__, 0x38371feb, "GehSWY2ay4uUKhehXYb0", 20, 0x707b3b30},
  {__LINE__, 0xafc8bf62, "kwytJmq6UqpflV8Y8GoE", 20, 0x16abc6a9},
  {__LINE__, 0x9b07db73, "70684206568419061514", 20, 0xae1fb7b7},
  {__LINE__, 0xe75b214, "42015093765128581010", 20, 0xd4eecd2d},
  {__LINE__, 0x72d0fe6f, "88214814356148806939", 20, 0x4660ec7},
  {__LINE__, 0xf857a4b1, "43472694284527343838", 20, 0xfd8afdf7},
  {__LINE__, 0x54b8e14, "49769333513942933689", 20, 0xc6d1b5f2},
  {__LINE__, 0xd6aa5616, "54979784887993251199", 20, 0x32476461},
  {__LINE__, 0x11e63098, "58360544869206793220", 20, 0xd917cf1a},
  {__LINE__, 0xbe92385, "27347953487840714234", 20, 0x4ad14a12},
  {__LINE__, 0x49511de0, "07650690295365319082", 20, 0xe37b5c6c},
  {__LINE__, 0x3db13bc1, "42655507906821911703", 20, 0x7cc497f1},
  {__LINE__, 0xbb899bea, "29977409200786225655", 20, 0x99781bb2},
  {__LINE__, 0xf6cd9436, "85181542907229116674", 20, 0x132256a1},
  {__LINE__, 0x9109e6c3, "87963594337989416799", 20, 0xbfdb2c83},
  {__LINE__, 0x75770fc, "21395988329504168551", 20, 0x8d9d1e81},
  {__LINE__, 0x69b1d19b, "51991013580943379423", 20, 0x7b6d4404},
  {__LINE__, 0xc6132975, "*]+@!);({_$;}[_},?{?;(_?,=-][@", 30, 0x8619f010},
  {__LINE__, 0xd58cb00c, "_@:_).&(#.[:[{[:)$++-($_;@[)}+", 30, 0x15746ac3},
  {__LINE__, 0xb63b8caa, "&[!,[$_==}+.]@!;*(+},[;:)$;)-@", 30, 0xaccf812f},
  {__LINE__, 0x8a45a2b8, "]{.[.+?+[[=;[?}_#&;[=)__$$:+=_", 30, 0x78af45de},
  {__LINE__, 0xcbe95b78, "-%.)=/[@].:.(:,()$;=%@-$?]{%+%", 30, 0x25b06b59},
  {__LINE__, 0x4ef8a54b, "+]#$(@&.=:,*];/.!]%/{:){:@(;)$", 30, 0x4ba0d08f},
  {__LINE__, 0x76ad267a, ")-._.:?[&:.=+}(*$/=!.${;(=$@!}", 30, 0xe26b6aac},
  {__LINE__, 0x569e613c, ":(_*&%/[[}+,?#$&*+#[([*-/#;%(]", 30, 0x7e2b0a66},
  {__LINE__, 0x36aa61da, "{[#-;:$/{)(+[}#]/{&!%(@)%:@-$:", 30, 0xb3430dc7},
  {__LINE__, 0xf67222df, "_{$*,}(&,@.)):=!/%(&(,,-?$}}}!", 30, 0x626c17a},
  {__LINE__, 0x74b34fd3, "e$98KNzqaV)Y:2X?]77].{gKRD4G5{mHZk,Z)SpU%L3FSgv!Wb8MLAFdi{+fp)c,@8m6v)yXg@]HBDFk?.4&}g5_udE*JHCiH=aL", 100, 0xccf98060},
  {__LINE__, 0x351fd770, "r*Fd}ef+5RJQ;+W=4jTR9)R*p!B;]Ed7tkrLi;88U7g@3v!5pk2X6D)vt,.@N8c]@yyEcKi[vwUu@.Ppm@C6%Mv*3Nw}Y,58_aH)", 100, 0xd8b95312},
  {__LINE__, 0xc45aef77, "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&", 100, 0xbb1c9912},
  {__LINE__, 0xc45aef77, "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&"
                         "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&"
                         "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&"
                         "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&"
                         "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&"
                         "h{bcmdC+a;t+Cf{6Y_dFq-{X4Yu&7uNfVDh?q&_u.UWJU],-GiH7ADzb7-V.Q%4=+v!$L9W+T=bP]$_:]Vyg}A.ygD.r;h-D]m%&", 600, 0x888AFA5B}
};

static const int test_size = sizeof(tests) / sizeof(tests[0]);

int main(void)
{
    int i;
    for (i = 0; i < test_size; i++) {
        test_crc32(tests[i].crc, tests[i].buf, tests[i].len,
                   tests[i].expect, tests[i].line);
    }
    return 0;
}
