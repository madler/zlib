::
:: Builds many different combinations of zlib on windows.
:: Provided as an example on how to build exactly what you want.
::

@ECHO ON
SETLOCAL EnableDelayedExpansion

::
:: Set these to your BUILDDIR, INSTDIR, and SRCDIR if needed - the
:: defaults should work fine if you run the batch script from win32.
:: The LOGFILE will receive directory listings from each build combo.
::

SET ZLIB_BUILD=C:\temp\zlib\zlib-build
SET ZLIB_INSTALL=C:\temp\zlib\zlib-install
SET ZLIB_HOME=%CD%\..
SET LOGFILE=C:\temp\zlib\zlib-matrix.log
@ECHO/ > %LOGFILE%

:: These are the stock builds for zlib (builds static and shared together)
CALL :BUILDONE Debug          stock-debug
CALL :BUILDONE RelWithDebInfo stock-relwithdebinfo
CALL :BUILDONE Release        stock-release

:: These are new build configurations targeting a single type and mode
CALL :BUILDONE Debug          shared-debug                      "-DBUILD_STATIC_AND_SHARED=OFF"
CALL :BUILDONE RelWithDebInfo shared-relwithdebinfo             "-DBUILD_STATIC_AND_SHARED=OFF"
CALL :BUILDONE Release        shared-release                    "-DBUILD_STATIC_AND_SHARED=OFF"

:: For static debug library builds put the debug information in the library with /Z7
CALL :BUILDONE Debug          static-debug                      "-DBUILD_STATIC_AND_SHARED=OFF" "-DBUILD_SHARED_LIBS=OFF" "-DCMAKE_C_FLAGS_DEBUG=/Z7"
CALL :BUILDONE RelWithDebInfo static-relwithdebinfo             "-DBUILD_STATIC_AND_SHARED=OFF" "-DBUILD_SHARED_LIBS=OFF" "-DCMAKE_C_FLAGS_RELWITHDEBINFO=/Z7"
CALL :BUILDONE Release        static-release                    "-DBUILD_STATIC_AND_SHARED=OFF" "-DBUILD_SHARED_LIBS=OFF"

:: Static library with a static MSVC runtime:
CALL :BUILDONE Debug          static-debug-static-rtl           "-DBUILD_STATIC_AND_SHARED=OFF" "-DBUILD_SHARED_LIBS=OFF" "-DCMAKE_C_FLAGS_DEBUG=/MTd /Z7"
CALL :BUILDONE RelWithDebInfo static-relwithdebinfo-static-rtl  "-DBUILD_STATIC_AND_SHARED=OFF" "-DBUILD_SHARED_LIBS=OFF" "-DCMAKE_C_FLAGS_RELWITHDEBINFO=/MT /Z7"
CALL :BUILDONE Release        static-release-static-rtl         "-DBUILD_STATIC_AND_SHARED=OFF" "-DBUILD_SHARED_LIBS=OFF" "-DCMAKE_C_FLAGS_RELEASE=/MT"

EXIT /B

:: \param[in]  %1    Build Config
:: \param[in]  %2    Build Name
:: \param[in]  %3..  Build Options
:BUILDONE
IF EXIST %ZLIB_BUILD%\%2 (RMDIR /S /Q %ZLIB_BUILD%\%2)
IF EXIST %ZLIB_INSTALL%\%2 (RMDIR /S /Q %ZLIB_INSTALL%\%2)
MKDIR    %ZLIB_BUILD%\%2
CD       %ZLIB_BUILD%\%2
cmake %ZLIB_HOME% -DCMAKE_INSTALL_PREFIX=%ZLIB_INSTALL%\%2 %3 %4 %5 %6 %7 || EXIT /B
cmake --build . --target install --config %1
@ECHO/ >> %LOGFILE%
@ECHO "-------------------------------------------------------------------------------" >> %LOGFILE%
@ECHO "-- %2" >> %LOGFILE%
@ECHO "-------------------------------------------------------------------------------" >> %LOGFILE%
DIR /S %ZLIB_INSTALL%\%2 >> %LOGFILE%
