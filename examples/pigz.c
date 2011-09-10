/* pigz.c -- parallel implementation of gzip
 * Copyright (C) 2007 Mark Adler
 * Version 1.1  28 January 2007  Mark Adler
 */

/* Version history:
   1.0  17 Jan 2007  First version
   1.1  28 Jan 2007  Avoid void * arithmetic (some compilers don't get that)
                     Add note about requiring zlib 1.2.3
                     Allow compression level 0 (no compression)
                     Completely rewrite parallelism -- add a write thread
                     Use deflateSetDictionary() to make use of history
                     Tune argument defaults to best performance on four cores
 */

/*
   pigz compresses from stdin to stdout using threads to make use of multiple
   processors and cores.  The input is broken up into 128 KB chunks, and each
   is compressed separately.  The CRC for each chunk is also calculated
   separately.  The compressed chunks are written in order to the output,
   and the overall CRC is calculated from the CRC's of the chunks.

   The compressed data format generated is the gzip format using the deflate
   compression method.  First a gzip header is written, followed by raw deflate
   partial streams.  They are partial, in that they do not have a terminating
   block.  At the end, the deflate stream is terminated with a final empty
   static block, and lastly a gzip trailer is written with the CRC and the
   number of input bytes.

   Each raw deflate partial stream is terminated by an empty stored block
   (using the Z_SYNC_FLUSH option of zlib), in order to end that partial
   bit stream at a byte boundary.  That allows the partial streams to be
   concantenated simply as sequences of bytes.  This adds a very small four
   or five byte overhead to the output for each input chunk.

   zlib's crc32_combine() routine allows the calcuation of the CRC of the
   entire input using the independent CRC's of the chunks.  pigz requires zlib
   version 1.2.3 or later, since that is the first version that provides the
   crc32_combine() function.

   pigz uses the POSIX pthread library for thread control and communication.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include "zlib.h"

#define local static

/* exit with error */
local void bail(char *msg)
{
    fprintf(stderr, "pigz abort: %s\n", msg);
    exit(1);
}

/* read up to len bytes into buf, repeating read() calls as needed */
local size_t readn(int desc, unsigned char *buf, size_t len)
{
    ssize_t ret;
    size_t got;

    got = 0;
    while (len) {
        ret = read(desc, buf, len);
        if (ret < 0)
            bail("read error");
        if (ret == 0)
            break;
        buf += ret;
        len -= ret;
        got += ret;
    }
    return got;
}

/* write len bytes, repeating write() calls as needed */
local void writen(int desc, unsigned char *buf, size_t len)
{
    ssize_t ret;

    while (len) {
        ret = write(desc, buf, len);
        if (ret < 1)
            bail("write error");
        buf += ret;
        len -= ret;
    }
}

/* a flag variable for communication between two threads */
struct flag {
    int value;              /* value of flag */
    pthread_mutex_t lock;   /* lock for checking and changing flag */
    pthread_cond_t cond;    /* condition for signaling on flag change */
};

/* initialize a flag for use, starting with value val */
local void flag_init(struct flag *me, int val)
{
    me->value = val;
    pthread_mutex_init(&(me->lock), NULL);
    pthread_cond_init(&(me->cond), NULL);
}

/* set the flag to val, signal another process that may be waiting for it */
local void flag_set(struct flag *me, int val)
{
    pthread_mutex_lock(&(me->lock));
    me->value = val;
    pthread_cond_signal(&(me->cond));
    pthread_mutex_unlock(&(me->lock));
}

/* if it isn't already, wait for some other thread to set the flag to val */
local void flag_wait(struct flag *me, int val)
{
    pthread_mutex_lock(&(me->lock));
    while (me->value != val)
        pthread_cond_wait(&(me->cond), &(me->lock));
    pthread_mutex_unlock(&(me->lock));
}

/* if flag is equal to val, wait for some other thread to change it */
local void flag_wait_not(struct flag *me, int val)
{
    pthread_mutex_lock(&(me->lock));
    while (me->value == val)
        pthread_cond_wait(&(me->cond), &(me->lock));
    pthread_mutex_unlock(&(me->lock));
}

/* clean up the flag when done with it */
local void flag_done(struct flag *me)
{
    pthread_cond_destroy(&(me->cond));
    pthread_mutex_destroy(&(me->lock));
}

/* a unit of work to feed to compress_thread() -- it is assumed that the out
   buffer is large enough to hold the maximum size len bytes could deflate to,
   plus five bytes for the final sync marker */
struct work {
    size_t len;                 /* length of input */
    unsigned long crc;          /* crc of input */
    unsigned char *buf;         /* input */
    unsigned char *out;         /* space for output (guaranteed big enough) */
    z_stream strm;              /* pre-initialized z_stream */
    struct flag busy;           /* busy flag indicating work unit in use */
    pthread_t comp;             /* this compression thread */
};

/* busy flag values */
#define IDLE 0          /* compress and writing done -- can start compress */
#define COMP 1          /* compress -- input and output buffers in use */
#define WRITE 2         /* compress done, writing output -- can read input */

/* read-only globals (set by main/read thread before others started) */
local int ind;              /* input file descriptor */
local int outd;             /* output file descriptor */
local int level;            /* compression level */
local int procs;            /* number of compression threads (>= 2) */
local size_t size;          /* uncompressed input size per thread (>= 32K) */
local struct work *jobs;    /* work units: jobs[0..procs-1] */

/* next and previous jobs[] indices */
#define NEXT(n) ((n) == procs - 1 ? 0 : (n) + 1)
#define PREV(n) ((n) == 0 ? procs - 1 : (n) - 1)

/* sliding dictionary size for deflate */
#define DICT 32768U

/* largest power of 2 that fits in an unsigned int -- used to limit requests
   to zlib functions that use unsigned int lengths */
#define MAX ((((unsigned)-1) >> 1) + 1)

/* compress thread: compress the input in the provided work unit and compute
   its crc -- assume that the amount of space at job->out is guaranteed to be
   enough for the compressed output, as determined by the maximum expansion
   of deflate compression -- use the input in the previous work unit (if there
   is one) to set the deflate dictionary for better compression */
local void *compress_thread(void *arg)
{
    size_t len;                     /* input length for this work unit */
    unsigned long crc;              /* crc of input data */
    struct work *prev;              /* previous work unit */
    struct work *job = arg;         /* work unit for this thread */
    z_stream *strm = &(job->strm);  /* zlib stream for this work unit */

    /* reset state for a new compressed stream */
    (void)deflateReset(strm);

    /* initialize input, output, and crc */
    strm->next_in = job->buf;
    strm->next_out = job->out;
    len = job->len;
    crc = crc32(0L, Z_NULL, 0);

    /* set dictionary if this isn't the first work unit, and if we will be
       compressing something (the read thread assures that the dictionary
       data in the previous work unit is still there) */
    prev = jobs + PREV(job - jobs);
    if (prev->buf != NULL && len != 0)
        deflateSetDictionary(strm, prev->buf + (size - DICT), DICT);

    /* run MAX-sized amounts of input through deflate and crc32 -- this loop
       is needed for those cases where the integer type is smaller than the
       size_t type, or when len is close to the limit of the size_t type */
    while (len > MAX) {
        strm->avail_in = MAX;
        strm->avail_out = (unsigned)-1;
        crc = crc32(crc, strm->next_in, strm->avail_in);
        (void)deflate(strm, Z_NO_FLUSH);
        len -= MAX;
    }

    /* run last piece through deflate and crc32, follow with a sync marker */
    if (len) {
        strm->avail_in = len;
        strm->avail_out = (unsigned)-1;
        crc = crc32(crc, strm->next_in, strm->avail_in);
        (void)deflate(strm, Z_SYNC_FLUSH);
    }

    /* don't need to Z_FINISH, since we'd delete the last two bytes anyway */

    /* return result */
    job->crc = crc;
    return NULL;
}

/* put a 4-byte integer into a byte array in LSB order */
#define PUT4(a,b) (*(a)=(b),(a)[1]=(b)>>8,(a)[2]=(b)>>16,(a)[3]=(b)>>24)

/* write thread: wait for compression threads to complete, write output in
   order, also write gzip header and trailer around the compressed data */
local void *write_thread(void *arg)
{
    int n;                          /* compress thread index */
    size_t len;                     /* length of input processed */
    unsigned long tot;              /* total uncompressed size (overflow ok) */
    unsigned long crc;              /* CRC-32 of uncompressed data */
    unsigned char wrap[10];         /* gzip header or trailer */

    /* write simple gzip header */
    memcpy(wrap, "\037\213\10\0\0\0\0\0\0\3", 10);
    wrap[8] = level == 9 ? 2 : (level == 1 ? 4 : 0);
    writen(outd, wrap, 10);

    /* process output of compress threads until end of input */    
    tot = 0;
    crc = crc32(0L, Z_NULL, 0);
    n = 0;
    do {
        /* wait for compress thread to start, then wait to complete */
        flag_wait(&(jobs[n].busy), COMP);
        pthread_join(jobs[n].comp, NULL);

        /* now that compress is done, allow read thread to use input buffer */
        flag_set(&(jobs[n].busy), WRITE);

        /* write compressed data and update length and crc */
        writen(outd, jobs[n].out, jobs[n].strm.next_out - jobs[n].out);
        len = jobs[n].len;
        tot += len;
        crc = crc32_combine(crc, jobs[n].crc, len);

        /* release this work unit and go to the next work unit */
        flag_set(&(jobs[n].busy), IDLE);
        n = NEXT(n);

        /* an input buffer less than size in length indicates end of input */
    } while (len == size);

    /* write final static block and gzip trailer (crc and len mod 2^32) */
    wrap[0] = 3;  wrap[1] = 0;
    PUT4(wrap + 2, crc);
    PUT4(wrap + 6, tot);
    writen(outd, wrap, 10);
    return NULL;
}

/* one-time initialization of a work unit -- this is where we set the deflate
   compression level and request raw deflate, and also where we set the size
   of the output buffer to guarantee enough space for a worst-case deflate
   ending with a Z_SYNC_FLUSH */
local void job_init(struct work *job)
{
    int ret;                        /* deflateInit2() return value */

    job->buf = malloc(size);
    job->out = malloc(size + (size >> 11) + 10);
    job->strm.zfree = Z_NULL;
    job->strm.zalloc = Z_NULL;
    job->strm.opaque = Z_NULL;
    ret = deflateInit2(&(job->strm), level, Z_DEFLATED, -15, 8,
                       Z_DEFAULT_STRATEGY);
    if (job->buf == NULL || job->out == NULL || ret != Z_OK)
        bail("not enough memory");
}

/* compress ind to outd in the gzip format, using multiple threads for the
   compression and crc calculation and another thread for writing the output --
   the read thread is the main thread */
local void read_thread(void)
{
    int n;                          /* general index */
    size_t got;                     /* amount read */
    pthread_attr_t attr;            /* thread attributes (left at defaults) */
    pthread_t write;                /* write thread */

    /* set defaults (not all pthread implementations default to joinable) */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    /* allocate and set up work list (individual work units will be initialized
       as needed, in case the input is short), assure that allocation size
       arithmetic does not overflow */
    if (size + (size >> 11) + 10 < (size >> 11) + 10 ||
        (ssize_t)(size + (size >> 11) + 10) < 0 ||
        ((size_t)0 - 1) / procs <= sizeof(struct work) ||
        (jobs = malloc(procs * sizeof(struct work))) == NULL)
        bail("not enough memory");
    for (n = 0; n < procs; n++) {
        jobs[n].buf = NULL;
        flag_init(&(jobs[n].busy), IDLE);
    }

    /* start write thread */
    pthread_create(&write, &attr, write_thread, NULL);

    /* read from input and start compress threads (write thread will pick up
       the output of the compress threads) */
    n = 0;
    do {
        /* initialize this work unit if it's the first time it's used */
        if (jobs[n].buf == NULL)
            job_init(jobs + n);

        /* read input data, but wait for last compress on this work unit to be
           done, and wait for the dictionary to be used by the last compress on
           the next work unit */
        flag_wait_not(&(jobs[n].busy), COMP);
        flag_wait_not(&(jobs[NEXT(n)].busy), COMP);
        got = readn(ind, jobs[n].buf, size);

        /* start compress thread, but wait for write to be done first */
        flag_wait(&(jobs[n].busy), IDLE);
        jobs[n].len = got;
        pthread_create(&(jobs[n].comp), &attr, compress_thread, jobs + n);

        /* mark work unit so write thread knows compress was started */
        flag_set(&(jobs[n].busy), COMP);

        /* go to the next work unit */
        n = NEXT(n);

        /* do until end of input, indicated by a read less than size */
    } while (got == size);

    /* wait for the write thread to complete -- the write thread will join with
       all of the compress threads, so this waits for all of the threads to
       complete */
    pthread_join(write, NULL);

    /* free up all requested resources and return */
    for (n = procs - 1; n >= 0; n--) {
        flag_done(&(jobs[n].busy));
        (void)deflateEnd(&(jobs[n].strm));
        free(jobs[n].out);
        free(jobs[n].buf);
    }
    free(jobs);
    pthread_attr_destroy(&attr);
}

/* Process arguments for level, size, and procs, compress from stdin to
   stdout in the gzip format.  Note that procs must be at least two in
   order to provide a dictionary in one work unit for the other work
   unit, and that size must be at least 32K to store a full dictionary. */
int main(int argc, char **argv)
{
    int n;                          /* general index */
    int get;                        /* command line parameters to get */
    char *arg;                      /* command line argument */

    /* set defaults -- 32 processes and 128K buffers was found to provide
       good utilization of four cores (about 97%) and balanced the overall
       execution time impact of more threads against more dictionary
       processing for a fixed amount of memory -- the memory usage for these
       settings and full use of all work units (at least 4 MB of input) is
       16.2 MB
       */
    level = Z_DEFAULT_COMPRESSION;
    procs = 32;
    size = 131072UL;

    /* process command-line arguments */
    get = 0;
    for (n = 1; n < argc; n++) {
        arg = argv[n];
        if (*arg == '-') {
            while (*++arg)
                if (*arg >= '0' && *arg <= '9')     /* compression level */
                    level = *arg - '0';
                else if (*arg == 'b')               /* chunk size in K */
                    get |= 1;
                else if (*arg == 'p')               /* number of processes */
                    get |= 2;
                else if (*arg == 'h') {             /* help */
                    fputs("usage: pigz [-0..9] [-b blocksizeinK]", stderr);
                    fputs(" [-p processes] < foo > foo.gz\n", stderr);
                    return 0;
                }
                else
                    bail("invalid option");
        }
        else if (get & 1) {
            if (get & 2)
                bail("you need to separate the -b and -p options");
            size = (size_t)(atol(arg)) << 10;       /* chunk size */
            if (size < DICT)
                bail("invalid option");
            get = 0;
        }
        else if (get & 2) {
            procs = atoi(arg);                      /* processes */
            if (procs < 2)
                bail("invalid option");
            get = 0;
        }
        else
            bail("invalid option (you need to pipe input and output)");
    }
    if (get)
        bail("missing option argument");

    /* do parallel compression from stdin to stdout (the read thread starts up
       the write thread and the compression threads, and they all join before
       the read thread returns) */
    ind = 0;
    outd = 1;
    read_thread();

    /* done */
    return 0;
}
