:: =================================================================================================
:: Copyright 2013 Adobe Systems Incorporated
:: All Rights Reserved.
::
:: NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
:: of the Adobe license agreement accompanying it.
:: =================================================================================================

echo OFF

rem Change relPath to point to the folder with the shared configs,
set relPath=..

set workingDir=%~dp0%
set buildSharedLoc=%workingDir%%relPath%/build/shared
call "%buildSharedLoc%/CMakeUtils.bat" %*
if errorlevel 1 goto error
goto ok

:error
echo Failed %PROJECT% build cmake.
echo "Exiting cmake.bat"
exit /B 1

:ok
echo Success %PROJECT% build cmake.
echo "Exiting cmake.bat"
exit /B 0

