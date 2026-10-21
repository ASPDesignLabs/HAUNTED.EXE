@echo off
rem Removes the installed screensaver and its saved settings.
net session >nul 2>&1
if errorlevel 1 (
  echo Please run this as administrator ^(right-click - Run as administrator^).
  pause
  exit /b 1
)
del "%SystemRoot%\System32\HAUNTED.scr" 2>nul
rem the same screensaver under its old name, before 1.0.0b1
del "%SystemRoot%\System32\HalloweenSaver.scr" 2>nul
reg delete "HKCU\Software\HalloweenVGASaver" /f >nul 2>&1
echo Done. Pick another screensaver in Settings - Personalization - Lock screen - Screen saver settings.
pause
