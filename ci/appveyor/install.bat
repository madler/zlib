@ECHO ON
SETLOCAL
::CALL ci\appveyor\env.bat

:: Move sh.exe out of the path for MinGW builds to work
IF "%WRAPPER:~0,5%" == "mingw" (
  REN "C:\Program Files\Git\usr\bin\sh.exe" "sh.exx"
)

:: As an added precaution against cmake locating MinGW items
:: when doing a MinGW-w64 build, we move MinGW out of the way
IF "%WRAPPER%" == "mingw64" (
  MOVE C:\MinGW C:\MinGW-Disabled
)

:: Install missing cygwin packages
IF "%WRAPPER:~0,6%" == "cygwin" (
  %CYGSETUP% -B -q -n -N -d -l %CYGDIR%/var/cache/setup -R %CYGDIR% -s http://mirror.rit.edu/cygwin -P cmake make
)

CALL ci\appveyor\build.bat
