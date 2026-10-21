@echo off
rem Runs the screensaver as an ordinary borderless full-screen window that screen recorders can see.
rem The mouse does nothing to it. Esc quits; F11 switches between full screen and a window.
rem
rem   OBS Studio: add a "Window Capture" (or "Game Capture") source and pick
rem               "HAUNTED.EXE (capture ...)".
rem   Xbox Game Bar: click the window, then press Win+Alt+R to start and stop recording.
cd /d "%~dp0"
start "" HAUNTED.scr /f
