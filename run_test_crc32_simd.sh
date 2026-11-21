#!/usr/bin/env bash
# Bit-exact CRC32 validation for Chromium-style PCLMUL simd (zen5 / avx2).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
gcc -O2 -Wall -msse4.2 -mpclmul \
  -DCRC32_SIMD_SSE42_PCLMUL -I. \
  -o /tmp/test_crc32_simd \
  test_crc32_simd.c crc32.c crc32_simd.c zutil.c
/tmp/test_crc32_simd
