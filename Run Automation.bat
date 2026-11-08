@echo off
setlocal
pushd "%~dp0" || exit /b 1
rem Windows PowerShell is included with Windows; no second launcher EXE.
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0Run Automation.ps1"
set "result=%ERRORLEVEL%"
popd
exit /b %result%
