@echo off
setlocal EnableExtensions
rem One entry point to the canonical PrawkoRoadStudio project, never an archive worktree.
set "ROOT=%~dp0"
set "EXE=%ROOT%build\dev-windows\editor\roadmaker-editor.exe"
set "SCENE=%ROOT%projects\Prawko-Scenariusze\07-pokaz-biblioteki-assetow.xodr"
if not exist "%EXE%" (
  echo Nie znaleziono edytora: "%EXE%"
  pause
  exit /b 1
)
if not exist "%SCENE%" (
  echo Nie znaleziono sceny pokazowej: "%SCENE%"
  pause
  exit /b 1
)
tasklist /FI "IMAGENAME eq roadmaker-editor.exe" /NH 2>NUL | find /I "roadmaker-editor.exe" >NUL
if not errorlevel 1 (
  echo PrawkoRoadStudio jest juz uruchomione. W otwartym programie wybierz Plik / Otworz.
  echo Nie otwieram drugiej instancji, aby nie nadpisac niezapisanej sceny.
  pause
  exit /b 2
)
start "" /D "%ROOT%" "%EXE%" "%SCENE%"
rem START can dispatch the GUI but still leave a nonzero shell ERRORLEVEL.
rem File existence and the already-running protection were checked above.
exit /b 0
