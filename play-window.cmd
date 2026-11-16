@echo off
setlocal
cd /d "%~dp0"
if not exist terminal_velocity_sdl.exe (
  echo The window build is missing. Extract the complete window edition ZIP first.
  pause
  exit /b 1
)
terminal_velocity_sdl.exe --terminal-view %*
if errorlevel 1 pause
