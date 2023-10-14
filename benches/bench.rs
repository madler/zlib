#![feature(test)]

extern crate test;
use cloudflare_zlib_sys::*;

#[bench]
fn compress_9(b: &mut test::Bencher) {
    let source: Vec<_> = (0..50000).map(|n| {
        (((n * 11111111) >> 13) ^ (n * 1321)) as u8
    }).collect();
    let mut gzbuf = vec![0u8; 50000];
    b.iter(|| unsafe {
        let mut len = gzbuf.len() as uLong;
        assert_eq!(0, compress2(gzbuf.as_mut_ptr(), &mut len,
                     source.as_ptr(), source.len() as uLong,
                     Z_BEST_COMPRESSION));
        assert!(len > 10000 && len <= 32556);
        len
    })
}

#[bench]
fn compress_def(b: &mut test::Bencher) {
    let source: Vec<_> = (0..50000).map(|n| {
        (((n * 11111117) >> 9) ^ (n * 1311)) as u8
    }).collect();
    let mut gzbuf = vec![0u8; 50000];
    b.iter(|| unsafe {
        let mut len = gzbuf.len() as uLong;
        assert_eq!(0, compress2(gzbuf.as_mut_ptr(), &mut len,
                     source.as_ptr(), source.len() as uLong,
                     Z_DEFAULT_COMPRESSION));
        assert!(len > 10000 && len <= 31486);
        len
    })
}

#[bench]
fn compress_1(b: &mut test::Bencher) {
    let source: Vec<_> = (0..50000).map(|n| {
        (((n * 11110031) >> 12) ^ (n * 1327)) as u8
    }).collect();
    let mut gzbuf = vec![0u8; 50000];
    b.iter(|| unsafe {
        let mut len = gzbuf.len() as uLong;
        assert_eq!(0, compress2(gzbuf.as_mut_ptr(), &mut len,
                     source.as_ptr(), source.len() as uLong,
                     Z_BEST_SPEED));
        assert!(len > 10000 && len <= 43967);
        len
    })
}
