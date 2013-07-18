:: =================================================================================================
:: Copyright 2013 Adobe Systems Incorporated
:: All Rights Reserved.
::
:: NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
:: of the Adobe license agreement accompanying it.
:: =================================================================================================

echo OFF
cls

ECHO Enter your choice:
ECHO 1. Clean All
ECHO 2. Generate XMPSDKToolkit Dynamic Win32
ECHO 3. Generate XMPSDKToolkit Static  Win32
ECHO 4. Generate XMPSDKToolkit Dynamic   x64
ECHO 5. Generate XMPSDKToolkit Static    x64
ECHO 6. Generate All


ECHO
set /P choice=Enter your choice:

ECHO your choice is %choice%

set GENERATE_ALL=Off
set NEXT_LABEL=ok

IF "%choice%"=="1" GOTO CLEANALL
IF "%choice%"=="2" GOTO 32DLL
IF "%choice%"=="3" GOTO 32LIB
IF "%choice%"=="4" GOTO 64DLL
IF "%choice%"=="5" GOTO 64LIB
IF "%choice%"=="6" GOTO GENALL

ECHO Invalid Choice, Exiting
exit /B 0

:GENALL
set GENERATE_ALL=On

:32DLL
echo "Generating XMPSDKToolkit Dynamic Win32"
set VS_VERSION=2010
set BUILD_TYPE=Dynamic
set BITS=32
IF "%GENERATE_ALL%"=="On" (
	set NEXT_LABEL=32LIB
)
GOTO GenerateNow


:32LIB
echo "Generating XMPSDKToolkit Static Win32"
set VS_VERSION=2010
set BUILD_TYPE=Static
set BITS=32
IF "%GENERATE_ALL%"=="On" (
	set NEXT_LABEL=64DLL
)
GOTO GenerateNow

:64DLL
echo "Generating XMPSDKToolkit Dynamic x64"
set VS_VERSION=2010
set BUILD_TYPE=Dynamic
set BITS=64
IF "%GENERATE_ALL%"=="On" (
	set NEXT_LABEL=64LIB
)
GOTO GenerateNow

:64LIB
echo "Generating XMPSDKToolkit Static x64"
set VS_VERSION=2010
set BUILD_TYPE=Static
set BITS=64
IF "%GENERATE_ALL%"=="On" (
	set NEXT_LABEL=ok
)
GOTO GenerateNow

:GenerateNow
call cmake_all.bat %BITS% %VS_VERSION% WarningAsError %BUILD_TYPE%
if errorlevel 1 goto error
goto %NEXT_LABEL%

:error
echo CMake Build Failed.
pause
exit /B 1


:ok
echo CMake Build Success.
pause
exit /B 0

:CLEANALL
echo "Cleaning..."
if exist vc10 rmdir /S /Q vc10
if exist ..\XMPCore\build\vc10 rmdir /S /Q ..\XMPCore\build\vc10
if exist ..\XMPFiles\build\vc10 rmdir /S /Q ..\XMPFiles\build\vc10
if exist ..\public\libraries\windows rmdir /S /Q ..\public\libraries\windows
if exist ..\public\libraries\windows_x64 rmdir /S /Q ..\public\libraries\windows_x64
echo "Done"
pause
exit /B 0
