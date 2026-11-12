@echo off
setlocal
set "ROADMAKER_REPO=%~dp0.."
call "%~dp0..\agent\roadctl.cmd" %*
exit /b %ERRORLEVEL%
