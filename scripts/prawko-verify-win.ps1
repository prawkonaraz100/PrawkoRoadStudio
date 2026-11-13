$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$tests=Join-Path $root 'build\dev-windows\core\tests\roadmaker_core_tests.exe'
$python=Join-Path $root 'reference\RoadMaker\toolchain\Python312\python.exe'
$agent=Join-Path $root 'agent'
$launcher=Join-Path $PSScriptRoot 'prawko-agent.cmd'
if(!(Test-Path $tests)){throw "Missing C++ tests: $tests. First build -Target tests."}
if(!(Test-Path $python) -or !(Test-Path $launcher)){throw 'Missing agent tools'}
# The agent must export using the same native mesh engine as the editor.
$bindings=@(Get-ChildItem -LiteralPath (Join-Path $root 'build\dev-windows\python') -Filter '_roadmaker*.pyd' -File)
if($bindings.Count -ne 1){throw 'Missing or ambiguous built Python binding. Build -Target python.'}
$vendored=Join-Path $agent ('vendor\roadmaker\'+$bindings[0].Name)
if(!(Test-Path $vendored)){throw "Missing agent binding: $vendored"}
$sha=[System.Security.Cryptography.SHA256]::Create()
try {
  $a=[BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($bindings[0].FullName)))
  $b=[BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($vendored)))
  if($a -ne $b){throw 'STALE_AGENT_BINDING: rebuild using -Target python or editor to update agent/vendor before exporting.'}
} finally {$sha.Dispose()}
Write-Host 'AGENT_BINDING_PARITY_PASS'
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
