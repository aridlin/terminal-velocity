@echo off
setlocal
cd /d "%~dp0"
if not exist terminal_velocity.exe (
  echo terminal_velocity.exe is missing. Extract the complete Windows ZIP first.
  pause
  exit /b 1
)
terminal_velocity.exe %*
if errorlevel 1 pause
