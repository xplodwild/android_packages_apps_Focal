:: =================================================================================================
:: Copyright 2013 Adobe Systems Incorporated
:: All Rights Reserved.
::
:: NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
:: of the Adobe license agreement accompanying it.
:: =================================================================================================

REM Available Arguments:
REM [64|32] Bit Architecture (optional, 64 is default)
REM [2008|2010|2012] VS Version(optional, 2010 is default)
REM [Dynamic | Static] optional, Dynamic by default
REM [WarningAsError] optional
REM [Clean] optional

echo OFF

SETLOCAL
:SetDefaultArguments
set Project=XMP
set CleanCMake=OFF
set XMPROOT=%buildSharedLoc%/../..

:: Default Generator
set GeneratorVersion=Visual Studio 10
set GeneratorArchitecture=Win64
set CMake_Arch64Bit=ON
set CMake_ARCH=x64

::Defualt Build Settings
set CMake_BuildStatic=Off
set CMake_Build_Warning_As_Error=Off

:: Folder arguments
set CMake64_Folder_Suffix=_x64
set CMakeGenVersion_FolderSuffix=
set CMake_LibTypeFolderName=dynamic

:: Parse over argumets using loop and shift
:Loop
	if /I "%1"=="" GOTO EndLoop
	
	
	if /I "%1"=="Clean" (
	echo "Clean make specified"
	set CleanCMake=ON
	)

	:: Static/Dynamic
	if /I "%1"=="Static" (
	echo "Static build on"
	set CMake_BuildStatic=On
	set CMake_LibTypeFolderName=static
	)
	if /I "%1"=="Dynamic" (
	echo "Static build off"
	set CMake_BuildStatic=Off
	set CMake_LibTypeFolderName=dynamic
	)

	:: Visual Studio Version
	if /I "%1"=="2010" (
	echo "Generator VS 2010 specified"
	set GeneratorVersion=Visual Studio 10
	set CMakeGenVersion_FolderSuffix=
	)

	if /I "%1"=="WarningAsError" (
	echo "sensible warnings activated"
	set CMake_Build_Warning_As_Error=On
	)

	if /I "%1"=="32" (
	set CMake_ARCH=x86
	set GeneratorArchitecture=Win32
	set CMake_Arch64Bit=OFF
	)

	if /I "%1"=="64" (
	echo 64 bit specified
	set CMake_Arch64Bit=ON
	set CMake_ARCH=x64
	)
	
	shift
	goto Loop

:EndLoop

if "%CMake_Arch64Bit%"=="ON" (
set CMake64_Folder_Suffix=_x64
) else (
set CMake64_Folder_Suffix=
)

:: CMake Folder specified:
set CMakeFolder="vc10/%CMake_LibTypeFolderName%/windows%CMake64_Folder_Suffix%"
echo CMakeFolder: %CMakeFolder%

:: Create generator type from VS version and architecture
if "%GeneratorArchitecture%"=="Win32" (
::32 Bit has no generator argument
set GENERATOR=%GeneratorVersion%
) else (
:: Is ARM or Win64
set GENERATOR=%GeneratorVersion% %GeneratorArchitecture%
)
echo Generator used: %GENERATOR%

:: Delete old cmake folder on "clean"
if "%CleanCMake%"=="ON" (
echo deleting folder %CleanCMake%
if exist %CMakeFolder% rmdir /S /Q %CMakeFolder%
)

mkdir %CMakeFolder%
cd %CMakeFolder%
echo cmake ../../../. -G"%GENERATOR%" -DXMP_CMAKEFOLDER_NAME="%CMakeFolder%" -DCMAKE_CL_64=%CMake_Arch64Bit% -DCMAKE_ARCH=%CMake_ARCH% -DXMP_BUILD_WARNING_AS_ERROR=%CMake_Build_Warning_As_Error% -DXMP_BUILD_STATIC="%CMake_BuildStatic%" 
cmake ../../../. -G"%GENERATOR%" -DXMP_CMAKEFOLDER_NAME="%CMakeFolder%" -DCMAKE_CL_64=%CMake_Arch64Bit% -DCMAKE_ARCH=%CMake_ARCH% -DXMP_BUILD_WARNING_AS_ERROR=%CMake_Build_Warning_As_Error% -DXMP_BUILD_STATIC="%CMake_BuildStatic%" 

if errorlevel 1 goto error
goto ok

:error
cd ../../..
echo Failed %Project% build cmake.
echo "Exiting CMakeUtils.bat"
exit /B 1

:ok
echo Success %Project% build cmake.
cd ../../..
echo "Exiting CMakeUtils.bat"
exit /B 0