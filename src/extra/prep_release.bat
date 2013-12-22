@ECHO OFF

IF "%1" == "" GOTO :EMPTY_VERSION

ECHO Verifying prerequisites

IF DEFINED ProgramFiles(x86) (
  SET DEVENV_EXE_PATH="%ProgramFiles(x86)%\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe"
) ELSE (
  SET DEVENV_EXE_PATH="%ProgramFiles%\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe"
)

IF EXIST %DEVENV_EXE_PATH% GOTO VS_FOUND
ECHO [ERROR] MS Visual Studio 2010 is not found. Exiting.
PAUSE
EXIT 1

:VS_FOUND
ECHO Using Visual Studio 2010 from
ECHO %DEVENV_EXE_PATH%
ECHO.

ECHO Building version for Far 1 x86
%DEVENV_EXE_PATH% /Rebuild "Release|Win32" "..\Observer.VS2010.sln"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Recoding lang files
pushd ..\..\bin\Release
@iconv --from-code=UTF-8 --to-code=866 -c ObserverRus.hlf > ObserverRus.tmp
@move /Y ObserverRus.tmp ObserverRus.hlf > nul
popd

ECHO Packing release
@rar.exe a -y -r -ep1 -apObserver -x*.metagen -- Observer_Far1_x86_%1.rar "..\..\bin\Release\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 2 x86
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode|Win32" "..\Observer.VS2010.sln"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
@rar.exe a -y -r -ep1 -apObserver -x*.metagen -- Observer_Far2_x86_%1.rar "..\..\bin\Release-Unicode\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 2 x64
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode|x64" "..\Observer.VS2010.sln"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
@rar.exe a -y -r -ep1 -apObserver -x*.metagen -- Observer_Far2_x64_%1.rar "..\..\bin\Release-Unicode-x64\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 3 x86
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode-3|Win32" "..\Observer.VS2010.sln"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
@rar.exe a -y -r -ep1 -apObserver -x*.metagen -- Observer_Far3_x86_%1.rar "..\..\bin\Release-Unicode-3\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 3 x64
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode-3|x64" "..\Observer.VS2010.sln"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
@rar.exe a -y -r -ep1 -apObserver -x*.metagen -- Observer_Far3_x64_%1.rar "..\..\bin\Release-Unicode-3-x64\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO [SUCCESS] Global build complete !!!
EXIT 0

:BUILD_ERROR
ECHO Unable to build project with Visual Studio. Exit code = %ERRORLEVEL%
EXIT 2

:PACK_ERROR
ECHO Unable to pack release into archive
EXIT 3

:EMPTY_VERSION
ECHO Please set release version
EXIT 4
