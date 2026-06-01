/* 
   Makes a dynamic huffman table given the symbol counts.
   Based on zlib/examples/zpipe.c in zlib 1.2.8

   cd to zlib root directory
   ./configure
   make
   cd examples
   cc -O -I.. -o makedht  makedht.c ../libz.a

   deflate_make_dht( (z_stream *)strm, (int *)lhist, (int *)dhist, (int *)valid_bits ); 
   Caller provides lhist and dhist int arrays.  A dynamic huffman
   table (DHT) formatted in the manner of Deflate Type 2 block is
   returned in strm.  Number of valid bits in the last byte is
   returned in valid_bits.

   format_cpb(char *cpbtxt, char *zbuf, int have, int valid_bits )
   Pretty formats the DHT.
*/

/* 
   From command line, supply the Literal/Length/Distance symbols and
   their counts in the *lzcount file.  makedht then calls zlib to make
   the dynamic huffman table (DHT).  Makedht then writes human
   readable DHT to stdout and binary DHT to <fname>.

   [abali@hahn examples]$ ./makedht jabber1.lzcount jabber1.dht
   bytes: 19  invalid bits: 4
   --------------------------------
   00000000000000000000000000000094
   203826000000220058c5a6900244f0c3
   d7770700000000000000000000000000
   --------------------------------

   Hex dump of the same:
   [abali@hahn examples]$ xxd jabber1.dht
   0000000: 2038 2600 0000 2200 58c5 a690 0244 f0c3   8&...".X....D..
   0000010: d777 07                                  .w.

   Notes: 
   Invalid bit count is the number of unused **left-most** bits in the
   last byte.  Bit endianness is due to the Deflate specification.

   When DEBUG is enabled in zlib, Huffman codes assigned to each
   symbol are also printed to stderr.

   The -f flag asks zlib to produce a Huffman code for all the Lit/Len
   (0-285) and Dist (0-29) symbols. The -f flag overrides the symbol
   counts of 0 to 1, and therefore forces the code to be generated for
   all the symbols.  In the example below, you can see that the
   result is larger when compared to the previous example.

   [abali@hahn examples]$ ./makedht -f jabber1.lzcount jabber1.dht
   bytes: 56  invalid bits: 4
   --------------------------------
   000000000000000000000000000001bc
   bde300040208da443232b3f7cedeca48
   56943d92ec952dbbec19d9ab4284ca4e
   43c8deca56b2f7cc2a65454564af9292
   f0fbff7e8ffbfd0f0000000000000000
   --------------------------------

   Sample *.lzcount file
   [abali@hahn examples]$ cat jabber1.lzcount
 32 :          2
 39 :          1
 84 :          1
 97 :          1
 98 :          1
114 :          1
115 :          1
119 :          1
256 :          1
  0 :          0
  1 :          0
 29 :          0
*/


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "zlib.h"

#define CHUNK 16384

/*
  Cpb stands for compression parameter block.  format_cpb converts the
  zlib produced DHT in zbuf to ASCII text and writes to cpbtxt.  Have
  is number of bytes in zbuf.  Valid_bits is the value returned from
  deflateMakeDHT(). */

void format_cpb(char *cpbtxt, char *zbuf, int have, int valid_bits )
{
    int i, b;
    char *ptr;
    char tmp[CHUNK];
    char hex[] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
    int invalid_bits;

    /* last byte */
    invalid_bits = ( valid_bits ) ? 8 - valid_bits : 0 ;
    fprintf(stderr,"bytes: %d  invalid bits: %d\n", have, invalid_bits );
    /* format-clear the cpbparm file 1st line */
    memset( cpbtxt, '0', 32 );
    /* format the cpb bit count */
    sprintf( tmp, "%x", 8 * have - invalid_bits );
    b = strlen( tmp );
    /* write the bit count to cpb parm 1st line */
    strncpy( cpbtxt+(32-b), tmp, b );
    /* continue from the next line */
    ptr = cpbtxt+32;
    for(i=0; i<have; i++) {
	    unsigned char byte;
	    if ( i % 16 == 0 ) *(ptr++) = '\n'; /* write 16 bytes per line */
	    byte = (unsigned char) zbuf[i];
	    *(ptr++) = hex[ (byte>>4)&0xf ]; /* convert hex to ASCII */
	    *(ptr++) = hex[ (byte   )&0xf ];
    }
    /* padding for the last line */
    for(i=have; i< (16*((have+15)/16)); i++) {
	    *(ptr++) = '0'; *(ptr++) = '0';
    }
    *(ptr++) = '\n'; *(ptr++) = 0;
}



int makedht(char *fname, int *lhist, int *dhist)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    char in[CHUNK];
    char zbuf[CHUNK];
    char cpbtxt[CHUNK];
    FILE *cpbbin;
    int i;
    int valid_bits; 
    int b;

    /* file for the CPB binary output */
    if( NULL == ( cpbbin = fopen( fname, "w" )) ) {
	    fprintf( stderr, "error: cannot open %s\n", fname );
	    return 1;
    }

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
	    return ret;

    strm.avail_out = CHUNK;
    strm.next_out = zbuf;

    ret = deflate_make_dht( &strm, lhist, dhist, &valid_bits );
    assert(ret != Z_STREAM_ERROR);

    have = CHUNK - strm.avail_out;
    if (fwrite(zbuf, 1, have, cpbbin) != have || ferror(cpbbin)) {
	    (void)deflateEnd(&strm);
	    return Z_ERRNO;
    }
    fclose( cpbbin );

    format_cpb( cpbtxt, zbuf, have, valid_bits );

    fflush(stdout);
    fputs("--------------------------------\n", stdout );
    fputs( cpbtxt, stdout );
    fputs("--------------------------------\n", stdout );
    fflush(stdout);

    (void)deflateEnd(&strm);

    return Z_OK;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
	    if (ferror(stdin))
		    fputs("error reading stdin\n", stderr);
	    if (ferror(stdout))
		    fputs("error writing stdout\n", stderr);
	    break;
    case Z_STREAM_ERROR:
	    fputs("invalid compression level\n", stderr);
	    break;
    case Z_DATA_ERROR:
	    fputs("invalid or incomplete deflate data\n", stderr);
	    break;
    case Z_MEM_ERROR:
	    fputs("out of memory\n", stderr);
	    break;
    case Z_VERSION_ERROR:
	    fputs("zlib version mismatch!\n", stderr);
    }
}

/* Initialize zero lzcounts to a val.  If the same DHT will be used
   repeatedly by different input data, the DHT must contain a symbol
   for all possible input symbols.  Changing zero counts to a nonzero
   count ensures that in the DHT there is a code for every symbol. Of
   course this comes at the expense of DHTs being larger  */

void fill_zero_lzcounts(int *llhist, int *dhist, int val)
{
    int i;
    for(i=0; i<286; i++) 
	    if( ! llhist[i] ) 
		    llhist[i] = val;
    for(i=0; i<30; i++)  
	    if( ! dhist[i] )  
		    dhist[i] = val;
}

/* read lzcounts from file fname and write them to the int arrays
   llhist and dhist for Lit/Len and Distance respectively */

int get_lzcounts(char *fname, int *llhist, int *dhist)
{
    int i, lz, prev_lz, count, doll;
    FILE *lzf;
    char buf[1024];
    if( NULL == ( lzf = fopen( fname, "r" )) ) {
	    fprintf( stderr, "error: cannot open %s\n", fname );
	    return 1;
    }
    for(i=0; i<286; i++) 
	    llhist[i] = 0;
    for(i=0; i<30; i++)  
	    dhist[i] = 0;
    prev_lz=0;
    doll=1;

    while( NULL != fgets( buf, 1023, lzf ) ) {
	    sscanf( buf, "%d : %d", &lz, &count );
	    if( prev_lz > lz ) /* detect LL to D transition */
		    doll = 0;
	    assert( (doll==1 && lz >= 0 && lz <= 285) || (doll==0 && lz >= 0 && lz <= 29 ) );
	    prev_lz = lz;
	    if( doll )
		    llhist[ lz ] = count;
	    else
		    dhist[ lz ] = count;
    }
    llhist[256] = 1; /* The EOB symbol is always present */
    fclose( lzf );
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int lhist[286];
    int dhist[30];

    /* when -f argument is present */
    if (argc == 4 && strcmp(argv[1], "-f") == 0) {

	    /* read LZ counts from file */
	    if( get_lzcounts( argv[2], lhist, dhist ) )
		    return 1;

	    /* change zero counts to one */
	    fill_zero_lzcounts( lhist, dhist, 1 );

	    /* make the dht */
	    ret = makedht( argv[3], lhist, dhist );
	    if (ret != Z_OK)
		    zerr(ret);
	    fflush(stderr);

	    return ret;
    }
    /* no -f argument */
    else if( argc == 3 ) {

	    /* read LZ counts from file */
	    if( get_lzcounts( argv[1], lhist, dhist ) )
		    return 1;

	    /* make the dht */
	    ret = makedht( argv[2], lhist, dhist );
	    if (ret != Z_OK)
		    zerr(ret);
	    fflush(stderr);

	    return ret;
    }
    /* when argument count is wrong, report usage */
    else {
	    fprintf( stderr, "usage:\n");
	    fprintf( stderr, "%s [-f] <lzcount> <dht.bin>\n", argv[0]);
	    fprintf( stderr, "   <Lzcount> contains a symbol : count pair per line of input.\n");
	    fprintf( stderr, "   Lit/Len symbols 0..285 must be followed by Distance symbols 0..29.\n");
	    fprintf( stderr, "   Missing symbols have a count of 0 by default.\n");
	    fprintf( stderr, "   The optional -f changes 0 counts to 1.\n");
	    fprintf( stderr, "   Human readable output is printed to stdout.\n");
	    fprintf( stderr, "   Number of bits in the DHT is printed in the first 16 bytes.\n");
	    fprintf( stderr, "   Number of unused bits in the DHT tail byte is also printed.\n");
	    fprintf( stderr, "   Binary output is dumped to dht.bin.\n");	    
	    return 1;
    }

}
