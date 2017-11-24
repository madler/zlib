::
:: Environment Variables
::
:: In:
::   ADDPATH contains path directories that are prepended to the path (optional)
::   CMAKE_BUILD_TYPE sets the build configuration (required)
::   CMAKE_EXTRA is additional parameters to pass to cmake (optional)
::   GENERATOR is the cmake generator to use (optional)
::   WRAPPER is the build type wrapper to use (optional) [mingw, cygwin, cygwin64]
::
:: Out:
::   BUILDDIR is the directory to do the build in
::   INSTDIR  is the directory to put the install target files into
::   SRCDIR   is the directory containing the project
::
::   CYGDIR     is the home of cygwin, whichever flavor is being used
::   CYGSETUP   is the path to the setup executable for installing things
::   CYGINSTDIR is INSTDIR translated to cygwin's /cygdrive/... path syntax
::   CYGSRCDIR  is SRCDIR  translated to cygwin's /cygdrive/... path syntax
::
::   DASHG    is the cmake -G"<generator>" argument content
:: 

@ECHO OFF

SET BUILDDIR=C:\temp\build
SET INSTDIR=C:\temp\install
SET SRCDIR=%CD%

IF "%WRAPPER%" == "cygwin" (
  SET CYGDIR=C:\cygwin
  SET CYGSETUP=C:\cygwin\setup-x86.exe
) ELSE IF "%WRAPPER%" == "cygwin64" (
  SET CYGDIR=C:\cygwin64
  SET CYGSETUP=C:\cygwin64\setup-x86_64.exe
)

IF "%WRAPPER:~0,6%" == "cygwin" (
  :: was using cygpath but it isn't available on all cygwin versions, so...
  SET CYGSRCDIR=/cygdrive/c/projects/zlib
  SET CYGINSTDIR=/cygdrive/c/temp/install
)

IF DEFINED GENERATOR (
  SET DASHG="-G%GENERATOR%"
)

SET PATH=%ADDPATH%%PATH%

@ECHO/
@ECHO --------------------------------------------------------------------------------
@ECHO -- Environment Variables
@ECHO --------------------------------------------------------------------------------
@ECHO/
@ECHO Inputs:
@ECHO/
@ECHO ADDPATH          %ADDPATH%
@ECHO CMAKE_BUILD_TYPE %CMAKE_BUILD_TYPE%
@ECHO CMAKE_EXTRA      %CMAKE_EXTRA%
@ECHO GENERATOR        %GENERATOR%
@ECHO WRAPPER          %WRAPPER%
@ECHO/
@ECHO Outputs:
@ECHO/
@ECHO BUILDDIR         %BUILDDIR%
@ECHO INSTDIR          %INSTDIR%
@ECHO SRCDIR           %SRCDIR%
@ECHO/
@ECHO DASHG            %DASHG%
IF "%WRAPPER:~0,6%" == "cygwin" (
@ECHO/
@ECHO CYGDIR           %CYGDIR%
@ECHO CYGSETUP         %CYGSETUP%
@ECHO CYGSRCDIR        %CYGSRCDIR%
@ECHO CYGINSTDIR       %CYGINSTDIR%
)
@ECHO/
@ECHO PATH:
@ECHO/
@ECHO %PATH%
@ECHO/
@ECHO --------------------------------------------------------------------------------
