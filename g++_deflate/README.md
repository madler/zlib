# Deflate

Deflate is a C++ implementation of the DEFLATE compression algorithm
as specified in RFC 1951. It has both a Compressor and a Decompressor,
which are standard iostream's.

Deflate is simply a wrapper over the zlib interface compression library
written by Jean-Loup Gailly and Mark Adler.

Primarily intended for compressing or decompressing HTML5 websockets data,
as described in RFC 7692, "Deflate" does "raw" deflate.

Deflate depends on "MemStream", an early implementation (1997) of what is today
known as "std::strstream" or "std::stringstream". The MemStream source
-- slightly enhanced -- is part of this distribution.
The original is available on the [jaxo/yaxx github](https://github.com/Jaxo/yaxx/tree/master/toolslib)

This C++ code has been tested on Linux and g++. It should be easy to port it
to other environments.

Here is simple example of use:

```cpp
#include "Deflate.h"

int main() {
   Deflate::Compressor compressor;
   Deflate::Decompressor decompressor;

   compressor << "Hello, Hello?" << std::flush;
   decompressor << compressor.rdbuf() << std::flush;

   std::cout << '|' << decompressor.rdbuf() << '|' << std::endl;
   return 0;
}
```

To compile and run:
```sh
g++ -Wno-write-strings -Wall -std=c++0x -O0 -g -D _DEBUG \
ZTest.cpp Deflate.cpp MemStream.cpp -o ZTest -lz && ZTest
```

This code is licensed under the terms of the MIT license.
--------

Pierre G. Richard

<pgr@jaxo.com> \
\
\
GreenHouseKeeper SAS

Engineering Department

Montpellier - France


