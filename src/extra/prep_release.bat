@ECHO OFF

ECHO Fetching version

for /f %%i in ('m4 -P version.m4') do set PVER=%%i

IF "%PVER%" == "" GOTO :EMPTY_VERSION
ECHO Version detected: %PVER%
ECHO.

ECHO Searching for Visual Studio

IF DEFINED ProgramFiles(x86) (
  SET VS_WHERE_PATH="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
) ELSE (
  SET VS_WHERE_PATH="%ProgramFiles%\Microsoft Visual Studio\Installer"
)

SET PATH=%PATH%;%VS_WHERE_PATH%
for /f "usebackq tokens=1* delims=: " %%i in (`vswhere.exe -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop`) do (
	if /i "%%i"=="productPath" set DEVENV_EXE_PATH="%%j"
)

IF EXIST %DEVENV_EXE_PATH% GOTO VS_FOUND
ECHO [ERROR] MS Visual Studio 2017 is not found. Exiting.
PAUSE
EXIT 1

:VS_FOUND
ECHO Using Visual Studio from
ECHO %DEVENV_EXE_PATH%
ECHO.

SET PACKER_CMD=@rar.exe a -y -r -ep1 -apObserver -x*.metagen -x*.iobj -x*.ipdb
SET SOLUTION_FILE=..\Observer.sln

ECHO Building version for Far 2 x86
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode|Win32" "%SOLUTION_FILE%"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
%PACKER_CMD% -- Observer_Far2_x86_%PVER%.rar "..\..\bin\Release-Unicode-Win32\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 2 x64
%DEVENV_EXE_PATH% /Rebuild "Release-Unicode|x64" "%SOLUTION_FILE%"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
%PACKER_CMD% -- Observer_Far2_x64_%PVER%.rar "..\..\bin\Release-Unicode-x64\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 3 x86
%DEVENV_EXE_PATH% /Rebuild "Release-Far3|Win32" "%SOLUTION_FILE%"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
%PACKER_CMD% -- Observer_Far3_x86_%PVER%.rar "..\..\bin\Release-Far3-Win32\*" > nul
if NOT ERRORLEVEL == 0 GOTO PACK_ERROR

ECHO Building version for Far 3 x64
%DEVENV_EXE_PATH% /Rebuild "Release-Far3|x64" "%SOLUTION_FILE%"
if NOT ERRORLEVEL == 0 GOTO BUILD_ERROR

ECHO Packing release
%PACKER_CMD% -- Observer_Far3_x64_%PVER%.rar "..\..\bin\Release-Far3-x64\*" > nul
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
ECHO Can not fetch release version from sources
EXIT 4
