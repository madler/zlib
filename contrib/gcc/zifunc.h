/* Copyright (C) 2019 Matheus Castanho <msc@linux.ibm.com>, IBM
 *               2019 Rogerio Alves    <rogerio.alves@ibm.com>, IBM
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef Z_IFUNC_H_
#define Z_IFUNC_H_

/* Helpers for arch optimizations */

#if defined(__clang__)
#if __has_feature(coverage_sanitizer)
#define Z_IFUNC_NO_SANCOV __attribute__((no_sanitize("coverage")))
#else /* __has_feature(coverage_sanitizer) */
#define Z_IFUNC_NO_SANCOV
#endif /* __has_feature(coverage_sanitizer) */
#else /* __clang__ */
#define Z_IFUNC_NO_SANCOV
#endif /* __clang__ */

#ifdef __s390__
#define Z_IFUNC_PARAMS unsigned long hwcap
#define Z_IFUNC_ATTRS Z_IFUNC_NO_SANCOV
#else /* __s390__ */
#define Z_IFUNC_PARAMS void
#define Z_IFUNC_ATTRS
#endif /* __s390__ */

#define Z_IFUNC(fname) \
    typeof(fname) fname __attribute__ ((ifunc (#fname "_resolver"))); \
    Z_IFUNC_ATTRS \
    local typeof(fname) *fname##_resolver(Z_IFUNC_PARAMS)
/* This is a helper macro to declare a resolver for an indirect function
 * (ifunc). Let's say you have function
 *
 *    int foo (int a);
 *
 * for which you want to provide different implementations, for example:
 *
 *    int foo_clever (int a) {
 *      ... clever things ...
 *    }
 *
 *    int foo_smart (int a) {
 *      ... smart things ...
 *    }
 *
 * You will have to declare foo() as an indirect function and also provide a
 * resolver for it, to choose between foo_clever() and foo_smart() based on
 * some criteria you define (e.g. processor features).
 *
 * Since most likely foo() has a default implementation somewhere in zlib, you
 * may have to rename it so the 'foo' symbol can be used by the ifunc without
 * conflicts.
 *
 *    #define foo foo_default
 *    int foo (int a) {
 *      ...
 *    }
 *    #undef foo
 *
 * Now you just have to provide a resolver function to choose which function
 * should be used (decided at runtime on the first call to foo()):
 *
 *    Z_IFUNC(foo) {
 *        if (... some condition ...)
 *          return foo_clever;
 *
 *        if (... other condition ...)
 *          return foo_smart;
 *
 *        return foo_default;
 *    }
 *
 * All calls to foo() throughout the code can remain untouched, all the magic
 * will be done by the linker using the resolver function.
 */

#endif /* Z_IFUNC_H_ */
