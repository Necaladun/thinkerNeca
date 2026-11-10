@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 (
  echo ERROR: Unable to enter the SMAC/X game directory.
  echo.
  pause
  exit /b 1
)
rem The parent CMD window stays open after any completion, cancellation, or failure.
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0Run Automation.ps1"
set "result=%ERRORLEVEL%"
echo.
echo =====================================================
echo thinkerNeca Automation ended with exit code %result%
echo Diagnostics: "%~dp0automation\logs"
echo Closing this console does not terminate a running game.
echo =====================================================
echo.
pause
popd
exit /b %result%
