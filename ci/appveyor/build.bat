@ECHO ON
SETLOCAL
::CALL ci\appveyor\env.bat

:: Create the build directory
MKDIR %BUILDDIR% || EXIT /B
CD %BUILDDIR%

cmake --version

IF "%WRAPPER:~0,6%" == "cygwin" (
  :: Run the cygwin cmake build in a cygwin bash shell
  bash.exe -c "cmake %CYGSRCDIR% %DASHG% -DCMAKE_INSTALL_PREFIX=%CYGINSTDIR%" || EXIT /B
  bash.exe -c "cmake --build . --target install --config %CMAKE_BUILD_TYPE%"  || EXIT /B
) ELSE (
  cmake "%SRCDIR%" %DASHG% -DCMAKE_INSTALL_PREFIX=%INSTDIR% %CMAKE_EXTRA%     || EXIT /B
  cmake --build . --target install --config %CMAKE_BUILD_TYPE%                || EXIT /B
)

DIR /S %INSTDIR%
