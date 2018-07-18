IBM Z mainframes starting from version z15 provide DFLTCC instruction,
which implements deflate algorithm in hardware with estimated
compression and decompression performance orders of magnitude faster
than the current zlib and ratio comparable with that of level 1.

This directory adds DFLTCC support. In order to enable it, the following
build commands should be used:

    $ ./configure --dfltcc
    $ make

When built like this, zlib would compress in hardware on level 1, and in
software on all other levels. Decompression will always happen in
hardware. In order to enable DFLTCC compression for levels 1-6 (i.e. to
make it used by default) one could either configure with
--dfltcc-level-mask=0x7e or set the environment variable
DFLTCC_LEVEL_MASK to 0x7e at run time.
