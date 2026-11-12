$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$tests=Join-Path $root 'build\dev-windows\core\tests\roadmaker_core_tests.exe'
$python=Join-Path $root 'reference\RoadMaker\toolchain\Python312\python.exe'
$agent=Join-Path $root 'agent'
$launcher=Join-Path $PSScriptRoot 'prawko-agent.cmd'
if(!(Test-Path $tests)){throw "Missing C++ tests: $tests. First build -Target tests."}
if(!(Test-Path $python) -or !(Test-Path $launcher)){throw 'Missing agent tools'}
& $tests '--gtest_brief=1'
if($LASTEXITCODE -ne 0){throw "C++ tests FAIL: $LASTEXITCODE"}
Push-Location $agent
try {
  & $python -m unittest -v test_roadctl
  if($LASTEXITCODE -ne 0){throw "RoadMakerAgent tests FAIL: $LASTEXITCODE"}
} finally {
  Pop-Location
}
$demo=Join-Path $agent 'projects\prawko_urban_curve\scene.xodr'
if(Test-Path $demo){
  & $launcher validate $demo
  if($LASTEXITCODE -ne 0){throw "Demo scene validation FAIL: $LASTEXITCODE"}
}
Write-Host 'QUALITY_GATE_PASS: native C++ + Python agent + optional demo'
