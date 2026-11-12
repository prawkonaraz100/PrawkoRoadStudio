param([switch]$Verify)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$handoff = Join-Path $root 'HANDOFF.md'
$repos = @(
  @{Name='Editor'; Path=$root},
  @{Name='Agent'; Path=(Join-Path $root 'agent')}
)
if (!(Test-Path -LiteralPath $handoff)) { throw "Missing handoff file: $handoff" }
Write-Host '===== PRAWKO ROAD STUDIO - RESUME CHECK ====='
Write-Host "Machine: $env:COMPUTERNAME | Root: $root"
$stale = $false
foreach ($repo in $repos) {
  $path = $repo.Path
  $branch = (& git.exe -C $path branch --show-current | Out-String).Trim()
  if ($LASTEXITCODE -ne 0) { throw "Git branch check failed in $path" }
  $sha = (& git.exe -C $path rev-parse HEAD | Out-String).Trim()
  if ($LASTEXITCODE -ne 0) { throw "Git SHA check failed in $path" }
  $changes = @(& git.exe -C $path status --porcelain=v1 --untracked-files=normal)
  if ($LASTEXITCODE -ne 0) { throw "Git status failed in $path" }
  Write-Host "[$($repo.Name)] $branch @ $sha"
  if (@($changes | Where-Object { $_ -and $_.Trim() }).Count) {
    $stale = $true
    Write-Warning "UNCOMMITTED CHANGES in $($repo.Name) - inspect, do not overwrite:"
    $changes | ForEach-Object { if ($_) { Write-Host "  $_" } }
  } else {
    Write-Host '  Worktree clean'
  }
  $old = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'
  try {
    $remoteLine = (& git.exe -C $path ls-remote origin "refs/heads/$branch" 2>&1 | Out-String).Trim()
    $exitCode = $LASTEXITCODE
  } finally {
    $ErrorActionPreference = $old
  }
  if ($exitCode -ne 0 -or !($remoteLine -match '^([0-9a-f]{40})\s')) {
    $stale = $true
    Write-Warning "CANNOT VERIFY origin/$branch for $($repo.Name): $remoteLine"
  } elseif ($Matches[1] -ne $sha) {
    $stale = $true
    Write-Warning "LOCAL/REMOTE MISMATCH: local $sha vs origin/$branch $($Matches[1])"
  } else {
    Write-Host "  ORIGIN_SHA_MATCH: $sha"
  }
}
Write-Host ''
Write-Host '===== LAST HANDOFF & NEXT STEP ====='
Get-Content -LiteralPath $handoff -Encoding UTF8 | ForEach-Object { Write-Host $_ }
Write-Host ''
if ($stale) {
  Write-Warning 'RESUME WARNING: resolve dirty worktrees / remote mismatches before editing.'
} else {
  Write-Host 'RESUME_GIT_PASS: both worktrees clean and active branches match origin'
}
if ($Verify) {
  Write-Host '===== CURRENT QUALITY GATE ====='
  $verifyScript = Join-Path $root 'scripts\prawko-verify-win.ps1'
  & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $verifyScript
  if ($LASTEXITCODE -ne 0) { throw "QUALITY_GATE_FAIL: exit code $LASTEXITCODE" }
}
if ($stale) { exit 2 }
