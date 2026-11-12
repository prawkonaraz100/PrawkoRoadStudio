@echo off
setlocal
set "ROADMAKER_REPO=F:\PrawkoRoadStudio"
call "F:\RoadMakerAgent\roadctl.cmd" %*
exit /b %ERRORLEVEL%
