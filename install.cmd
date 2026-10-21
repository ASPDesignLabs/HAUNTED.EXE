@echo off
rem Installs HAUNTED.scr via Windows' own screensaver installer
rem (asks for administrator rights, copies it to System32, opens Screen Saver Settings).
cd /d "%~dp0"
if not exist HAUNTED.scr (
  echo HAUNTED.scr not found next to this script.
  pause
  exit /b 1
)
rundll32.exe desk.cpl,InstallScreenSaver "%~dp0HAUNTED.scr"
