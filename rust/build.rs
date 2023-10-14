use std::env;
use std::path::PathBuf;

fn main() {
    let mut cc = cc::Build::new();
    let comp = cc.get_compiler();
    let msvc_bugs = comp.is_like_msvc();
    cc.warnings(false);
    cc.pic(true);

    let target_pointer_width = env::var("CARGO_CFG_TARGET_POINTER_WIDTH").expect("CARGO_CFG_TARGET_POINTER_WIDTH var");
    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").expect("CARGO_CFG_TARGET_ARCH var");

    if target_pointer_width != "64" {
        println!("cargo:warning=cloudflare-zlib does not support 32-bit architectures");
    }

    if let Ok(target_cpu) = env::var("TARGET_CPU") {
        cc.flag_if_supported(&format!("-march={}", target_cpu));
    }

    let vendor = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    println!("cargo:include={}", vendor.display());
    cc.include(&vendor);

    if target_arch == "aarch64" {
        cc.define("INFLATE_CHUNK_SIMD_NEON", Some("1"));
        cc.define("ADLER32_SIMD_NEON", Some("1"));
    } else if target_arch == "x86_64" {
        cc.define("INFLATE_CHUNK_SIMD_SSE2", Some("1"));
        if msvc_bugs {
            cc.define("__x86_64__", Some("1"));
        }
    }

    cc.define("INFLATE_CHUNK_READ_64LE", Some("1"));

    if msvc_bugs {
        cc.define("_CRT_SECURE_NO_DEPRECATE", Some("1"));
        cc.define("_CRT_NONSTDC_NO_DEPRECATE", Some("1"));
        cc.flag_if_supported("/arch:AVX");
    } else {
        cc.define("HAVE_UNISTD_H", Some("1"));
        cc.define("HAVE_HIDDEN", Some("1"));

        if "32" != target_pointer_width {
            cc.define("HAVE_OFF64_T", Some("1"));
            cc.define("_LARGEFILE64_SOURCE", Some("1"));
        }
        if target_arch == "aarch64" {
            cc.flag_if_supported("-march=armv8-a+crc");
        } else {
            if cc.is_flag_supported("-mssse3").is_ok() {
                cc.flag("-mssse3");
                cc.define("ADLER32_SIMD_SSSE3", Some("1"));
            }
            if cc.is_flag_supported("-mpclmul").is_ok() {
                cc.flag("-mpclmul");
                cc.define("HAS_PCLMUL", None);
                cc.file(vendor.join("crc32_simd.c"));
                cc.flag_if_supported("-msse4.2");
            }
        }
    }

    cc.file(vendor.join("adler32.c"));
    cc.file(vendor.join("adler32_simd.c"));
    cc.file(vendor.join("crc32.c"));
    cc.file(vendor.join("deflate.c"));
    cc.file(vendor.join("infback.c"));
    cc.file(vendor.join("inffast.c"));
    cc.file(vendor.join("inflate.c"));
    cc.file(vendor.join("inftrees.c"));
    cc.file(vendor.join("trees.c"));
    cc.file(vendor.join("zutil.c"));

    cc.file(vendor.join("compress.c"));
    cc.file(vendor.join("uncompr.c"));
    cc.file(vendor.join("gzclose.c"));
    cc.file(vendor.join("gzlib.c"));
    cc.file(vendor.join("gzread.c"));
    cc.file(vendor.join("gzwrite.c"));

    cc.file(vendor.join("inffast_chunk.c"));

    cc.compile("cloudflare_zlib");
}
