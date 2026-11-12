param(
  [ValidateSet('editor','python','tests','all')][string]$Target='editor',
  [int]$Jobs=3
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$qt=Join-Path $root '.qt\6.8.3\msvc2022_64'
$tools=Join-Path $root 'reference\RoadMaker\toolchain'
$cmake="$tools\cmake-3.31.6-windows-x86_64\bin\cmake.exe"
$ninja="$tools\ninja\ninja.exe"
$py="$tools\Python312\python.exe"
$vc=Join-Path $root 'toolchain\VS2022\VC\Auxiliary\Build\vcvars64.bat'
foreach($file in @($cmake,$ninja,$py,$vc)){
  if(!(Test-Path $file)){ throw "Missing build prerequisite: $file" }
}
$env:PATH="$tools\cmake-3.31.6-windows-x86_64\bin;$tools\ninja;$tools\Python312;$qt\bin;$env:PATH"
$build=Join-Path $root 'build\dev-windows'
$cmdline=('"'+$vc+'" >nul && cmake -S "'+$root+'" -B "'+$build+
  '" -G Ninja -DRM_BUILD_EDITOR=ON -DRM_BUILD_TESTS=ON'+
  ' -DRM_BUILD_PYTHON=ON -DPython_EXECUTABLE="'+$py+'"'+
  ' -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH="'+$qt+'"')
& cmd.exe /c $cmdline
if($LASTEXITCODE -ne 0){throw "CMake configure failed: $LASTEXITCODE"}
if($Target -eq 'tests'){
  $action='cmake --build "'+$build+'" --target roadmaker_core_tests --parallel '+$Jobs
}elseif($Target -eq 'all'){
  $action='cmake --build "'+$build+'" --parallel '+$Jobs
}elseif($Target -eq 'python'){
  $action='cmake --build "'+$build+'" --target _roadmaker --parallel '+$Jobs
}else{
  $action='cmake --build "'+$build+'" --target roadmaker-editor _roadmaker --parallel '+$Jobs
}
& cmd.exe /c ('"'+$vc+'" >nul && '+$action)
if($LASTEXITCODE -ne 0){throw "Build failed: $LASTEXITCODE"}
if($Target -eq 'editor' -or $Target -eq 'all'){
  $exe=Join-Path $build 'editor\roadmaker-editor.exe'
  & "$qt\bin\windeployqt.exe" --release --no-translations $exe
  if($LASTEXITCODE -ne 0){throw "Qt deployment failed: $LASTEXITCODE"}
}
Write-Host "BUILD_PASS: $Target"
