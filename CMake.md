# Building zlib with CMake #

## Prerequisites ##

CMake version 2.4.4 or later is required.  [Download](https://cmake.org/download/) and install the latest CMake (recommended).

## Introduction ##

If you are not familiar with CMake, this project is a great way to learn how it works.  Let's define a couple directories we'll refer to throughout the document:

- `BUILDDIR` is the directory where the build takes place
- `BUILDTYPE` is one of the CMake standard build types: `Debug`, `RelWithDebInfo`, or `Release`.
- `INSTDIR` is the directory where the build result is installed
- `SRCDIR` is the directory that contains the zlib project files

CMake takes cmake script files (typically called CMakeLists.txt) from `SRCDIR` and generates an entire platform-specific build environment in `BUILDDIR`.  CMake supports output of many different build environments, such as make, ninja, and NMake to name a few.  CMake also supports generation of IDE projects for Eclipse, Visual Studio, and others.  

## Invocation ##

Generating an out-of-tree build directory with CMake is trivial:

1. Create the `BUILDDIR`.
2. cd into `BUILDDIR`.
3. cmake `SRCDIR` `-DCMAKE_INSTALL_PREFIX=INSTDIR` `<other-options>`

## Options ##

Options are defined when you generate the build environment.  Options are defined by passing them with a `-D` prefix on the cmake command line.  Here are the options that matter most when building zlib:

| Option                    | Origin | Default | Meaning |
| :-----                    | -----: | ------: | :------ |
| `ASM686`                  | zlib   | `OFF`   | Build with an i686 assembly implementation |
| `AMD64`                   | zlib   | `OFF`   | Build with an x86_64 assembly implementation |
| `BUILD_SHARED_LIBS`       | cmake  | `ON`    | Build a shared library |
| `BUILD_STATIC_AND_SHARED` | zlib   | `ON`    | Build both a shared and a static library* |
| `CMAKE_BUILD_TYPE`        | cmake  | `Debug` | Defines the build type at generation time** |
| `CMAKE_INSTALL_PREFIX`    | cmake  | depends | Build prefix for installation, platform specific|
| `SKIP_INSTALL_ALL`        | zlib   | `OFF`   | Skip installation of everything |
| `SKIP_INSTALL_FILES`      | zlib   | `OFF`   | Skip installation of manual pages and pkgconfig files |
| `SKIP_INSTALL_HEADERS`    | zlib   | `OFF`   | Skip installation of headers |
| `SKIP_INSTALL_LIRARIES`   | zlib   | `OFF`   | Skip installation of binaries, libraries, and symbol files |

\* The CMake build for zlib produces static and shared libraries in the same build by default.  This behavior is not always optimal for building on every platform, therefore a CMake option called `BUILD_STATIC_AND_SHARED` was introduced to allow the caller to disable this combined build and honor the more commonly used `BUILD_SHARED_LIBS` CMake option instead.

\** `CMAKE_BUILD_TYPE` is only used on Unix.  For Windows, the build type is defined at build time, not at generation time.

A default build without options will create a shared library, debug build, and attempt to install it into a system-wide visible location (`/usr/local` on Unix, and `C:\Program Files` on Windows).

## Unix ##

Unix does not suffer from as many runtime library choices as Windows, and is therefore quite simple.  Using a minimal set of options we end up with static and shared builds as output:

    $ mkdir <BUILDDIR>
    $ cd <BUILDDIR>
    $ cmake <SRCDIR> -DCMAKE_INSTALL_PREFIX=<INSTDIR> -DCMAKE_BUILD_TYPE=<BUILDTYPE>
    $ cmake --build . --target install

### Generating Eclipse Projects ###

CMake has the ability to generate Eclipse projects.  When invoking cmake in `BUILDDIR`, specify the generator:

    $ cmake `<SRCDIR>` -G"Eclipse CDT4 - Unix Makefiles"

This will produce an Eclipse solution targeting `make` builds.  A full list of generators can be obtained by running this command and scrolling to the end of the output:

    cmake --help

## Windows ##

Building on Windows, with the large number of library configuration options, is a little more complicated.  For example one can build a statically-linked zlib that uses a dynamic runtime library, or one can build a statically-linked zlib that uses a static MSVC runtime library.  The CMake install target will put only the files you need into `INSTDIR`.  If the build contains debugging information, that will be included.  Some examples for building on Windows are found below.

### DLL (shared) with MSVCRT DLL ###

    C:\BUILDDIR> cmake C:\SRCDIR -DCMAKE_INSTALL_PREFIX=<INSTDIR> -DBUILD_STATIC_AND_SHARED=OFF
    C:\BUILDDIR> cmake --build . --target install --config <BUILDTYPE>

### LIB (static) with MSVCRT LIB (statically linked runtime) ###

    C:\BUILDDIR> cmake C:\SRCDIR -DCMAKE_INSTALL_PREFIX=<INSTDIR> -DBUILD_STATIC_AND_SHARED=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS_<BUILDTYPE>="/MT /Z7"
    C:\BUILDDIR> cmake --build . --target install --config <BUILDTYPE>

Note the use of `CMAKE_C_FLAGS_<BUILDTYPE>` here, which is equivalent to using `CFLAGS` in a make build for a given `BUILDTYPE`.  Options typically used here are:

- `/MT` to force a static release runtime link
- `/MTd` to force a static debug runtime link
- `/Z7` to put symbols in objects and libraries instead of a PDB file

### Generating Visual Studio Projects ###

CMake has the ability to generate MSVC projects.  When invoking cmake in `BUILDDIR`, specify the generator:

    C:\TEMP\zlib-build> cmake C:\SRC\zlib -G"Visual Studio 15 2017 Win64"

This will produce a Visual Studio 2017 solution.  A full list of generators can be obtained by running this command and scrolling to the end of the output:

    cmake --help

### Additional Resources ###

A full suite of example builds for Windows can be found in the `win32\cmake-matrix.bat` file.