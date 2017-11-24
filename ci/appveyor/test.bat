@ECHO ON
SETLOCAL
::CALL ci\appveyor\env.bat

CD %BUILDDIR%                            || EXIT /B
ctest -D ExperimentalTest                || EXIT /B

IF EXIST %CMAKE_BUILD_TYPE% (
  CD %CMAKE_BUILD_TYPE%                  || EXIT /B
)
COPY example.exe example.exe.orig        || EXIT /B
minigzip -3 example.exe                  || EXIT /B
minigzip -9 example.exe.gz               || EXIT /B
minigzip -d example.exe.gz.gz            || EXIT /B
minigzip -d example.exe.gz               || EXIT /B
fc /B example.exe example.exe.orig > nul
