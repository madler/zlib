# Rust bindings for Zlib with Cloudflare's performance improvements

This crate builds zlib C implementation for 64-bit ARM and Intel CPUs. It requires x86-64 CPU with SSE 4.2 or ARM64 with NEON & CRC. 32-bit architectures are not supported at all.

The API/ABI of the library is the same as original Zlib's.

