param(
  [ValidateSet('status', 'backup', 'finish')]
  [string]$Action = 'status',
  [string]$BackupRoot = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if ([string]::IsNullOrWhiteSpace($BackupRoot)) {
  $BackupRoot = Join-Path $root 'archive\git-backups'
}
$repos = @(
  @{ Name = 'editor'; Path = $root },
  @{ Name = 'agent'; Path = (Join-Path $root 'agent') }
)

function Invoke-CheckedGit([string]$Repo, [string[]]$GitArgs) {
  $previousPreference = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'
  try {
    $result = & git.exe -C $Repo @GitArgs 2>&1
    $gitExit = $LASTEXITCODE
  } finally {
    $ErrorActionPreference = $previousPreference
  }
  if ($gitExit -ne 0) {
    throw "Git failed ($Repo): git $($GitArgs -join ' ') : $result"
  }
  return $result
}

$dirty = $false
$records = @()
foreach ($repo in $repos) {
  $path = $repo.Path
  $name = $repo.Name
  if (!(Test-Path (Join-Path $path '.git'))) {
    throw "Missing Git repository: $path"
  }
  $branch = (Invoke-CheckedGit $path @('branch', '--show-current') | Out-String).Trim()
  $sha = (Invoke-CheckedGit $path @('rev-parse', 'HEAD') | Out-String).Trim()
  $changes = @(Invoke-CheckedGit $path @('status', '--porcelain=v1', '--untracked-files=normal'))
  $changes = @($changes | Where-Object { $_ -and $_.Trim() })
  $remotes = @(Invoke-CheckedGit $path @('remote'))
  Write-Host "[$name] $branch @ $sha"
  if ($changes.Count -gt 0) {
    $dirty = $true
    Write-Warning "[$name] $($changes.Count) uncommitted paths:"
    $changes | ForEach-Object { Write-Host "  $_" }
  } else {
    Write-Host "  working tree clean"
  }
  if ($remotes -notcontains 'origin') {
    Write-Warning "[$name] No project origin remote; no off-machine project backup is configured."
  }
  if ($remotes.Count -gt 0) {
    $remotes | ForEach-Object {
      $url = (Invoke-CheckedGit $path @('remote', 'get-url', $_) | Out-String).Trim()
      Write-Host "  remote $_ = $url"
    }
  }
  $records += [pscustomobject]@{
    component = $name
    directory = $path
    branch = $branch
    commit = $sha
    dirty_paths = $changes.Count
    remotes = @($remotes)
  }
}

if ($Action -eq 'status') {
  if ($dirty) { Write-Warning 'Commit or otherwise secure changes before ending the session.' }
  exit 0
}

if ($Action -eq 'finish') {
  if ($dirty) {
    throw 'FINISH BLOCKED: uncommitted edits in editor or agent. Review, test and commit each repo separately.'
  }
  $verify = Join-Path $root 'scripts\prawko-verify-win.ps1'
  & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $verify
  if ($LASTEXITCODE -ne 0) {
    throw "FINISH BLOCKED: Quality Gate failed with exit code $LASTEXITCODE."
  }
}

New-Item -ItemType Directory -Force -Path $BackupRoot | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$backupFiles = @()
foreach ($repo in $repos) {
  $target = Join-Path $BackupRoot "$($repo.Name)-$stamp.bundle"
  Invoke-CheckedGit $repo.Path @('bundle', 'create', $target, '--all') | Out-Null
  Invoke-CheckedGit $repo.Path @('bundle', 'verify', $target) | Out-Null
  # A bundle may verify in its source repo yet miss history for an empty clone.
  $restoreCheck = Join-Path $BackupRoot "_restore-check-$($repo.Name)-$stamp.git"
  if (Test-Path $restoreCheck) { throw "Restore check path exists: $restoreCheck" }
  $previousPreference = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'
  try {
    $cloneOutput = & git.exe clone --bare --quiet $target $restoreCheck 2>&1
    $cloneExit = $LASTEXITCODE
  } finally {
    $ErrorActionPreference = $previousPreference
  }
  if ($cloneExit -ne 0) {
    throw "Standalone restore failed: $target : $cloneOutput"
  }
  $expected = @($records | Where-Object { $_.component -eq $repo.Name })[0]
  $restored = (Invoke-CheckedGit $restoreCheck @('rev-parse', "refs/heads/$($expected.branch)") | Out-String).Trim()
  if ($restored -ne $expected.commit) {
    throw "Restore SHA mismatch for $($repo.Name): $restored vs $($expected.commit)"
  }
  Remove-Item -LiteralPath $restoreCheck -Force -Recurse
  Write-Host "STANDALONE_RESTORE_PASS: $($repo.Name) @ $restored"
  $sha256 = [System.Security.Cryptography.SHA256]::Create()
  $stream = [System.IO.File]::OpenRead($target)
  try {
    $hash = [System.BitConverter]::ToString($sha256.ComputeHash($stream)).Replace('-', '')
  } finally {
    $stream.Dispose()
    $sha256.Dispose()
  }
  $backupFiles += [pscustomobject]@{
    component = $repo.Name
    path = $target
    sha256 = $hash
  }
  Write-Host "BUNDLE_VERIFIED: $target"
}
$scenes = Join-Path $root 'agent\projects'
$sceneBackup = $null
if (Test-Path $scenes) {
  $sceneBackup = Join-Path $BackupRoot "scenes-$stamp"
  $sourceCount = @(Get-ChildItem -LiteralPath $scenes -File -Recurse).Count
  Copy-Item -LiteralPath $scenes -Destination $sceneBackup -Recurse -Force
  $backupCount = @(Get-ChildItem -LiteralPath $sceneBackup -File -Recurse).Count
  if ($sourceCount -ne $backupCount) {
    throw "Saved scene backup mismatch: $sourceCount vs $backupCount"
  }
  Write-Host "SCENES_COPIED: $sceneBackup ($backupCount files)"
}
$manifest = [pscustomobject]@{
  created_at = (Get-Date).ToString('o')
  machine = $env:COMPUTERNAME
  note = 'Git bundles do not contain uncommitted edits. Saved agent/projects files are copied separately. Unsaved GUI scenes, tools and SDKs are NOT included.'
  repositories = $records
  bundles = $backupFiles
  saved_scenes_backup = $sceneBackup
}
$manifestPath = Join-Path $BackupRoot "manifest-$stamp.json"
$manifest | ConvertTo-Json -Depth 6 | Set-Content -Path $manifestPath -Encoding UTF8
Write-Host "MANIFEST: $manifestPath"
if ($dirty) {
  Write-Warning 'Git bundles omit uncommitted edits; only saved files under agent/projects are copied.'
}
if ($Action -eq 'finish') {
  Write-Host 'SESSION_FINISH_PASS: clean repos + tests + verified Git bundles'
} else {
  Write-Host 'GIT_BACKUP_PASS: both repo bundles verified'
}
