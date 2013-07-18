:: =================================================================================================
:: Copyright 2013 Adobe Systems Incorporated
:: All Rights Reserved.
::
:: NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
:: of the Adobe license agreement accompanying it.
:: =================================================================================================

echo  ON
cls

REM list of all projects to build

echo "Update path environment that cmake can be found"
REM set PATH=%PATH%;%CD%\..\..\resources\tools\CMakeApp\win\bin
set PATH=%PATH%;%CD%\..\tools\cmake\bin


echo "================= Generate project for XMP build ================="
call cmake.bat %1 %2 %3 %4 %5
if errorlevel 1 goto error


goto ok


:error
echo Failed.
echo "Exiting cmake_all.bat"
exit /B 1

:ok
echo Success.
echo "Exiting cmake_all.bat"
exit /B 0
