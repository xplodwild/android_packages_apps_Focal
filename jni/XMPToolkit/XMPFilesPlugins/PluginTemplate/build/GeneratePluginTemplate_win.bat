:: =================================================================================================
:: Copyright 2013 Adobe Systems Incorporated
:: All Rights Reserved.
::
:: NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
:: of the Adobe license agreement accompanying it.
:: =================================================================================================

echo OFF
cls
set CMAKE=..\..\..\tools\cmake\bin\cmake.exe
set workingDir=%~dp0%
set CMAKE=%workingDir%%relPath%%CMAKE%
set CMAKE_BUILDSTATIC=Off
set BITS64=ON

if NOT exist %CMAKE% ( ECHO Cmake tool not present at %CMAKE%, cannot proceed 
pause
exit /B 0)

ECHO Enter your choice:
ECHO 1. Clean All
ECHO 2. Generate PluginTemplate Dynamic Win32
ECHO 3. Generate PluginTemplate Dynamic   x64
ECHO 4. Generate Both


ECHO
set /P choice=Enter your choice:

ECHO your choice is %choice%

set GENERATE_ALL=Off
set NEXT_LABEL=ok

IF "%choice%"=="1" GOTO CLEANALL
IF "%choice%"=="2" GOTO 32DLL
IF "%choice%"=="3" GOTO 64DLL
IF "%choice%"=="4" GOTO GENALL

ECHO Invalid Choice, Exiting
pause
exit /B 0

:GENALL
set GENERATE_ALL=On

:32DLL
echo "Generating PluginTemplate Dynamic Win32"
set GENERATOR=Visual Studio 10
set BITS64=OFF
set CMakeFolder="vc10/windows"
set CMake_ARCH=x86
IF "%GENERATE_ALL%"=="On" (
	set NEXT_LABEL=64DLL
)
GOTO GenerateNow


:64DLL
echo "Generating PluginTemplate Dynamic x64"
set GENERATOR=Visual Studio 10 Win64
set BITS64=ON
set CMakeFolder="vc10/windows_x64"
set CMake_ARCH=x64
IF "%GENERATE_ALL%"=="On" (
	set NEXT_LABEL=ok
)
GOTO GenerateNow



:GenerateNow
echo CMakeFolder: %CMakeFolder%
mkdir %CMakeFolder%
cd %CMakeFolder%
echo %CMAKE% ../../. -G"%GENERATOR%" -DCMAKE_CL_64=%BITS64% -DXMP_CMAKEFOLDER_NAME=%CMakeFolder% -DCMAKE_ARCH=%CMake_ARCH% -DXMP_BUILD_STATIC="%CMAKE_BUILDSTATIC%"
%CMAKE% ../../. -G"%GENERATOR%" -DCMAKE_CL_64=%BITS64% -DXMP_CMAKEFOLDER_NAME=%CMakeFolder% -DCMAKE_ARCH=%CMake_ARCH% -DXMP_BUILD_STATIC="%CMAKE_BUILDSTATIC%"
cd ..\..
if errorlevel 1 goto error
goto %NEXT_LABEL%

:error
cd ..\..
echo Failed %PROJECT% build cmake.
pause
exit /B 1


:ok
echo Success %PROJECT% build cmake.
pause
exit /B 0

:CLEANALL
echo "Cleaning..."
if exist vc10 rmdir /S /Q vc10
echo "Done"
pause
exit /B 0
