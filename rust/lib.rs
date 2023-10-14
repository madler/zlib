#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
//! It requires x86-64 CPU with SSE 4.2 or ARM64 with NEON & CRC.

use std::os::raw::*;

pub type alloc_func = unsafe extern "C" fn(voidpf, uInt, uInt) -> voidpf;
pub type Bytef = u8;
pub type free_func = unsafe extern "C" fn(voidpf, voidpf);
pub type gzFile = *mut gzFile_s;
pub type in_func = unsafe extern "C" fn(*mut c_void, *mut *const c_uchar) -> c_uint;
pub type out_func = unsafe extern "C" fn(*mut c_void, *mut c_uchar, c_uint) -> c_int;
pub type uInt = u32; // zconf.h says so
pub type uLong = u64; // zconf.h says so
pub type uLongf = uLong;
pub type voidp = *mut c_void;
pub type voidpc = *const c_void;
pub type voidpf = *mut c_void;

pub enum gzFile_s {}
pub enum internal_state {}

pub type z_off_t = isize;

pub const ZLIB_VERSION: &[u8] = b"1.2.8";

pub unsafe fn deflateInit2(strm: z_streamp, level: c_int, method: c_int, window_bits: c_int, mem_level: c_int, strategy: c_int) -> c_int {
    deflateInit2_(strm, level, method, window_bits, mem_level, strategy, ZLIB_VERSION.as_ptr() as *const _, ::std::mem::size_of::<z_stream>() as c_int)
}

pub unsafe fn deflateInit(strm: z_streamp, level: c_int) -> c_int {
    deflateInit_(strm, level, ZLIB_VERSION.as_ptr() as *const _, ::std::mem::size_of::<z_stream>() as c_int)
}

pub unsafe fn inflateInit(strm: z_streamp) -> c_int {
    inflateInit_(strm, ZLIB_VERSION.as_ptr() as *const _, ::std::mem::size_of::<z_stream>() as c_int)
}

pub unsafe fn inflateInit2(strm: z_streamp, window_bits: c_int) -> c_int {
    inflateInit2_(strm, window_bits, ZLIB_VERSION.as_ptr() as *const _, ::std::mem::size_of::<z_stream>() as c_int)
}

pub unsafe fn inflateBackInit(strm: z_streamp, window_bits: c_int, window: *mut c_uchar) -> c_int {
    inflateBackInit_(strm, window_bits, window, ZLIB_VERSION.as_ptr() as *const _, ::std::mem::size_of::<z_stream>() as c_int)
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct gz_header {
    pub text: c_int,
    pub time: uLong,
    pub xflags: c_int,
    pub os: c_int,
    pub extra: *mut Bytef,
    pub extra_len: uInt,
    pub extra_max: uInt,
    pub name: *mut Bytef,
    pub name_max: uInt,
    pub comment: *mut Bytef,
    pub comm_max: uInt,
    pub hcrc: c_int,
    pub done: c_int,
}
pub type gz_headerp = *mut gz_header;

#[repr(C)]
#[derive(Copy, Clone)]
pub struct z_stream {
    pub next_in: *mut Bytef,
    pub avail_in: uInt,
    pub total_in: uLong,
    pub next_out: *mut Bytef,
    pub avail_out: uInt,
    pub total_out: uLong,
    pub msg: *mut c_char,
    pub state: *mut internal_state,
    pub zalloc: Option<alloc_func>,
    pub zfree: Option<free_func>,
    pub opaque: voidpf,
    pub data_type: c_int,
    pub adler: uLong,
    pub reserved: uLong,
}
pub type z_streamp = *mut z_stream;

extern "C" {
    pub fn adler32(adler: uLong, buf: *const Bytef, len: uInt) -> uLong;
    pub fn adler32_combine(adler1: uLong, adler2: uLong, len2: z_off_t) -> uLong;
    pub fn compress(dest: *mut Bytef, destLen: *mut uLongf, source: *const Bytef, sourceLen: uLong) -> c_int;
    pub fn compress2(dest: *mut Bytef, destLen: *mut uLongf, source: *const Bytef, sourceLen: uLong, level: c_int) -> c_int;
    pub fn compressBound(sourceLen: uLong) -> uLong;
    pub fn crc32(crc: uLong, buf: *const Bytef, len: uInt) -> uLong;
    pub fn crc32_combine(crc1: uLong, crc2: uLong, len2: z_off_t) -> uLong;
    pub fn deflate(strm: z_streamp, flush: c_int) -> c_int;
    pub fn deflateBound(strm: z_streamp, sourceLen: uLong) -> uLong;
    pub fn deflateCopy(dest: z_streamp, source: z_streamp) -> c_int;
    pub fn deflateEnd(strm: z_streamp) -> c_int;
    pub fn deflateInit2_(strm: z_streamp, level: c_int, method: c_int, windowBits: c_int, memLevel: c_int, strategy: c_int, version: *const c_char, stream_size: c_int) -> c_int;
    pub fn deflateInit_(strm: z_streamp, level: c_int, version: *const c_char, stream_size: c_int) -> c_int;
    pub fn deflateParams(strm: z_streamp, level: c_int, strategy: c_int) -> c_int;
    pub fn deflatePending(strm: z_streamp, pending: *mut c_uint, bits: *mut c_int) -> c_int;
    pub fn deflatePrime(strm: z_streamp, bits: c_int, value: c_int) -> c_int;
    pub fn deflateReset(strm: z_streamp) -> c_int;
    pub fn deflateResetKeep(arg1: z_streamp) -> c_int;
    pub fn deflateSetDictionary(strm: z_streamp, dictionary: *const Bytef, dictLength: uInt) -> c_int;
    pub fn deflateSetHeader(strm: z_streamp, head: gz_headerp) -> c_int;
    pub fn deflateTune(strm: z_streamp, good_length: c_int, max_lazy: c_int, nice_length: c_int, max_chain: c_int) -> c_int;
    pub fn gzclearerr(file: gzFile);
    pub fn gzclose(file: gzFile) -> c_int;
    pub fn gzdirect(file: gzFile) -> c_int;
    pub fn gzdopen(fd: c_int, mode: *const c_char) -> gzFile;
    pub fn gzeof(file: gzFile) -> c_int;
    pub fn gzerror(file: gzFile, errnum: *mut c_int) -> *const c_char;
    pub fn gzflush(file: gzFile, flush: c_int) -> c_int;
    pub fn gzgetc(file: gzFile) -> c_int;
    pub fn gzgets(file: gzFile, buf: *mut c_char, len: c_int) -> *mut c_char;
    pub fn gzopen(path: *const c_char, mode: *const c_char) -> gzFile;
    pub fn gzoffset(arg1: gzFile) -> c_long;
    pub fn gzputc(file: gzFile, c: c_int) -> c_int;
    pub fn gzputs(file: gzFile, s: *const c_char) -> c_int;
    pub fn gzread(file: gzFile, buf: voidp, len: c_uint) -> c_int;
    pub fn gzrewind(file: gzFile) -> c_int;
    pub fn gzseek(file: gzFile, offset: z_off_t, whence: c_int) -> z_off_t;
    pub fn gzsetparams(file: gzFile, level: c_int, strategy: c_int) -> c_int;
    pub fn gztell(file: gzFile) -> z_off_t;
    pub fn gzungetc(c: c_int, file: gzFile) -> c_int;
    pub fn gzwrite(file: gzFile, buf: voidpc, len: c_uint) -> c_int;
    pub fn inflate(strm: z_streamp, flush: c_int) -> c_int;
    pub fn inflateBack(strm: z_streamp, _in: in_func, in_desc: *mut c_void, out: out_func, out_desc: *mut c_void) -> c_int;
    pub fn inflateBackEnd(strm: z_streamp) -> c_int;
    pub fn inflateBackInit_(strm: z_streamp, windowBits: c_int, window: *mut c_uchar, version: *const c_char, stream_size: c_int) -> c_int;
    pub fn inflateCopy(dest: z_streamp, source: z_streamp) -> c_int;
    pub fn inflateEnd(strm: z_streamp) -> c_int;
    pub fn inflateGetDictionary(strm: z_streamp, dictionary: *mut Bytef, dictLength: *mut uInt) -> c_int;
    pub fn inflateGetHeader(strm: z_streamp, head: gz_headerp) -> c_int;
    pub fn inflateInit2_(strm: z_streamp, windowBits: c_int, version: *const c_char, stream_size: c_int) -> c_int;
    pub fn inflateInit_(strm: z_streamp, version: *const c_char, stream_size: c_int) -> c_int;
    pub fn inflateMark(strm: z_streamp) -> c_long;
    pub fn inflatePrime(strm: z_streamp, bits: c_int, value: c_int) -> c_int;
    pub fn inflateReset(strm: z_streamp) -> c_int;
    pub fn inflateReset2(strm: z_streamp, windowBits: c_int) -> c_int;
    pub fn inflateResetKeep(arg1: z_streamp) -> c_int;
    pub fn inflateSetDictionary(strm: z_streamp, dictionary: *const Bytef, dictLength: uInt) -> c_int;
    pub fn inflateSync(strm: z_streamp) -> c_int;
    pub fn inflateSyncPoint(arg1: z_streamp) -> c_int;
    pub fn inflateUndermine(arg1: z_streamp, arg2: c_int) -> c_int;
    pub fn inflateValidate(arg1: z_streamp, arg2: c_int) -> c_int;
    pub fn uncompress(dest: *mut Bytef, destLen: *mut uLongf, source: *const Bytef, sourceLen: uLong) -> c_int;
    pub fn uncompress2(dest: *mut Bytef, destLen: *mut uLongf, source: *const Bytef, sourceLen: *mut uLong) -> c_int;
    pub fn zError(arg1: c_int) -> *const c_char;
    pub fn zlibCompileFlags() -> uLong;
    pub fn zlibVersion() -> *const c_char;
}

pub const Z_NO_FLUSH: c_int = 0;
pub const Z_PARTIAL_FLUSH: c_int = 1;
pub const Z_SYNC_FLUSH: c_int = 2;
pub const Z_FULL_FLUSH: c_int = 3;
pub const Z_FINISH: c_int = 4;
pub const Z_BLOCK: c_int = 5;
pub const Z_TREES: c_int = 6;

pub const Z_OK: c_int = 0;
pub const Z_STREAM_END: c_int = 1;
pub const Z_NEED_DICT: c_int = 2;
pub const Z_ERRNO: c_int = -1;
pub const Z_STREAM_ERROR: c_int = -2;
pub const Z_DATA_ERROR: c_int = -3;
pub const Z_MEM_ERROR: c_int = -4;
pub const Z_BUF_ERROR: c_int = -5;
pub const Z_VERSION_ERROR: c_int = -6;

pub const Z_NO_COMPRESSION: c_int = 0;
pub const Z_BEST_SPEED: c_int = 1;
pub const Z_BEST_COMPRESSION: c_int = 9;
pub const Z_DEFAULT_COMPRESSION: c_int = -1;

pub const Z_FILTERED: c_int = 1;
pub const Z_HUFFMAN_ONLY: c_int = 2;
pub const Z_RLE: c_int = 3;
pub const Z_FIXED: c_int = 4;
pub const Z_DEFAULT_STRATEGY: c_int = 0;

pub const Z_BINARY: c_int = 0;
pub const Z_TEXT: c_int = 1;
pub const Z_ASCII: c_int = Z_TEXT;
pub const Z_UNKNOWN: c_int = 2;

pub const Z_DEFLATED: c_int = 8;
pub const MAX_MEM_LEVEL: c_int = 8;
pub const DEF_MEM_LEVEL: c_int = 8;

#[test]
fn test_compress() {
    unsafe {
        let source = "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod
tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse
cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non
proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
        let mut gzbuf = [0u8; 500];

        let mut len = gzbuf.len() as uLong;
        assert_eq!(0, compress2(gzbuf.as_mut_ptr(), &mut len, source.as_bytes().as_ptr(), source.as_bytes().len() as uLong, Z_BEST_COMPRESSION));
        assert_eq!(446, source.len());
        assert_eq!(271, len);

        let mut unbuf = [0u8; 500];
        let mut len = unbuf.len() as uLong;
        assert_eq!(0, uncompress(unbuf.as_mut_ptr(), &mut len, gzbuf.as_ptr(), gzbuf.len() as uLong));
        assert_eq!(&unbuf[0..len as usize], source.as_bytes());
    }
}
