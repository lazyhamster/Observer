@ECHO OFF

IF "%1" == "" GOTO :EMPTY_VERSION

ECHO Verifying prerequisites

IF DEFINED ProgramFiles(x86) (
  SET DEVENV_EXE_PATH="%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\Common7\IDE\devenv.exe"
) ELSE (
  SET DEVENV_EXE_PATH="%ProgramFiles%\Microsoft Visual Studio 12.0\Common7\IDE\devenv.exe"
)

IF EXIST %DEVENV_EXE_PATH% GOTO VS_FOUND
ECHO [ERROR] MS Visual Studio 2013 is not found. Exiting.
PAUSE
EXIT 1

:VS_FOUND
ECHO Using Visual Studio from
ECHO %DEVENV_EXE_PATH%
ECHO.

SET PACKER_CMD=@rar.exe a -y -r -ep1 -apObserver -x*.metagen
SET SOLUTION_FILE=Observer.sln

REM ECHO Building version for Far 1 x86
REM %DEVENV_EXE_PATH% /Rebuild "Release|Win32" "..\Observer.VS2010.sln"
REM if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

REM ECHO Recoding lang files
REM pushd ..\..\bin\Release
REM @iconv --from-code=UTF-8 --to-code=866 -c ObserverRus.hlf > ObserverRus.tmp
REM @move /Y ObserverRus.tmp ObserverRus.hlf > nul
REM @iconv --from-code=UTF-8 --to-code=866 -c ObserverRus.lng > ObserverRus.tmp
REM @move /Y ObserverRus.tmp ObserverRus.lng > nul
REM popd

REM ECHO Packing release
REM %PACKER_CMD% -- Observer_Far1_x86_%1.rar "..\..\bin\Release\*" > nul
REM if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 2 x86
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode|Win32" "..\%SOLUTION_FILE%"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
%PACKER_CMD% -- Observer_Far2_x86_%1.rar "..\..\bin\Release-Unicode\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 2 x64
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode|x64" "..\%SOLUTION_FILE%"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
%PACKER_CMD% -- Observer_Far2_x64_%1.rar "..\..\bin\Release-Unicode-x64\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 3 x86
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode-3|Win32" "..\%SOLUTION_FILE%"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
%PACKER_CMD% -- Observer_Far3_x86_%1.rar "..\..\bin\Release-Unicode-3\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 3 x64
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode-3|x64" "..\%SOLUTION_FILE%"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
%PACKER_CMD% -- Observer_Far3_x64_%1.rar "..\..\bin\Release-Unicode-3-x64\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Packing source code
%PACKER_CMD% -xipch "-x*\ipch" "-x*\ipch\*" -x*.suo -x*.sdf -x*.opensdf -xObserver*.rar -- Observer_%1_src.rar "..\..\src" "..\..\doc" > nul
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
