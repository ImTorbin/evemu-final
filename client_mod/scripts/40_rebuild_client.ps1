<#
.SYNOPSIS
    Rebuild compiled.code from the modded client tree and install it.

.DESCRIPTION
    Assumes you have already:
      1. Run 00_check_prereqs.ps1 (to stage evecc/blue_patcher/uncompyle2).
      2. Run 10_dump_client.ps1 so the decompiled tree exists.
      3. Made your patches in <CodeRoot>.

    The script compiles the tree, backs up the existing compiled.code in
    place, and installs the rebuilt file. It then runs 42_patch_blue_manifest.ps1
    (VerifyManifest JNZ bypass). blue_patcher is separate: pass -PatchBlue to
    re-run it if blue.dll was replaced.

.PARAMETER EVEInstall
    The Crucible install whose script\compiled.code we replace.

.PARAMETER CodeRoot
    The modded client tree. Defaults to client_mod\eve_code.

.PARAMETER Out
    Directory for the fresh compiled.code. Defaults to client_mod\out.

.PARAMETER PatchBlue
    Re-run blue_patcher on bin\blue.dll (needed only if blue.dll has been
    replaced by a reinstall or never patched).

.PARAMETER ToolDir
    Folder containing evecc.exe and blue_patcher.exe.
#>
[CmdletBinding()]
param(
    [string] $EVEInstall,
    [string] $CodeRoot,
    [string] $Out,
    [switch] $PatchBlue,
    [string] $ToolDir
)

$ErrorActionPreference = 'Stop'
trap {
    Write-Host "`nUNHANDLED ERROR:" -ForegroundColor Red
    Write-Host $_ -ForegroundColor Red
    Read-Host "`n(press Enter to close)"
    exit 1
}

if (-not $ToolDir)  { $ToolDir  = Join-Path $PSScriptRoot '..\tools' }
if (-not $CodeRoot) { $CodeRoot = Join-Path $PSScriptRoot '..\eve_code' }
if (-not $Out)      { $Out      = Join-Path $PSScriptRoot '..\out' }

if (-not $EVEInstall) {
    foreach ($c in @("D:\Evemu\EVE", "${Env:ProgramFiles(x86)}\CCP\EVE", "$Env:ProgramFiles\CCP\EVE", "C:\CCP\EVE")) {
        if ($c -and (Test-Path (Join-Path $c 'bin\blue.dll'))) { $EVEInstall = $c; break }
    }
}
if (-not $EVEInstall) { throw "Pass -EVEInstall or place install at one of the auto-detect paths." }

$evecc       = Join-Path $ToolDir 'evecc.exe'
$bluePatcher = Join-Path $ToolDir 'blue_patcher.exe'
foreach ($exe in $evecc, $bluePatcher) {
    if (-not (Test-Path $exe)) { throw "Missing tool: $exe" }
}
if (-not (Test-Path $CodeRoot)) { throw "CodeRoot missing: $CodeRoot" }

$bundledPython = Join-Path $ToolDir 'Python'
if (Test-Path $bundledPython) {
    $env:PYTHONHOME = $bundledPython
    $env:PYTHONPATH = (Join-Path $bundledPython 'Lib') + ';' + (Join-Path $bundledPython 'Lib\site-packages')
}

if (-not (Test-Path $Out)) { New-Item -ItemType Directory -Force -Path $Out | Out-Null }

# evecc needs genkeys to have been run once so evecc.keys.priv exists. It is
# idempotent to re-run — dumpkeys / genkeys overwrite without prompting.
if (-not (Test-Path (Join-Path $ToolDir 'evecc.keys.priv'))) {
    Push-Location $ToolDir
    try {
        Write-Host "==> evecc genkeys (first-time setup)"
        & $evecc genkeys
        if ($LASTEXITCODE -ne 0) { throw "evecc genkeys exited $LASTEXITCODE" }
    } finally { Pop-Location }
}

# IMPORTANT: evecc prepends "./" to the -o path and cannot handle an absolute
# path (writes fail silently with [Errno 2]). So we must pass a *relative*
# path, run evecc from a working directory we control, and then move the
# result into $Out ourselves.
$compiledOut = Join-Path $Out 'compiled.code'
Remove-Item $compiledOut -Force -ErrorAction SilentlyContinue
$toolTmp = Join-Path $ToolDir 'compiled.code'
Remove-Item $toolTmp -Force -ErrorAction SilentlyContinue
Push-Location $ToolDir
try {
    Write-Host "==> evecc compilecode -I $CodeRoot -o compiled.code  (cwd=$ToolDir)"
    & $evecc compilecode --no-cache -I $CodeRoot -o compiled.code
    if ($LASTEXITCODE -ne 0) { throw "evecc compilecode exited $LASTEXITCODE" }
} finally { Pop-Location }
if (-not (Test-Path $toolTmp)) { throw "evecc compilecode did not produce $toolTmp" }
Move-Item -Force $toolTmp $compiledOut
Write-Host "Built $compiledOut ($([int]((Get-Item $compiledOut).Length/1MB)) MB)" -ForegroundColor Green

$blueDll = Join-Path $EVEInstall 'bin\blue.dll'
$blueBak = Join-Path $EVEInstall 'bin\blue.dll.bak'
if ($PatchBlue) {
    if (-not (Test-Path $blueBak)) {
        Copy-Item $blueDll $blueBak -Force
        Write-Host "Backed up blue.dll to $blueBak"
    }
    Push-Location (Split-Path $blueDll)
    try {
        Write-Host "==> blue_patcher on $blueDll"
        & $bluePatcher
        if ($LASTEXITCODE -ne 0) { throw "blue_patcher exited $LASTEXITCODE" }
    } finally { Pop-Location }
}

# Crucible builds differ: some installs only have script\compiled.code, others use
# bin\script\compiled.code (see README). The loader may prefer bin\script; install
# the same blob to every location that exists or is standard, so we never run a
# stock signed file from one path and a rebuilt file from another.
$compiledCandidates = @('script\compiled.code', 'bin\script\compiled.code', 'bin\compiled.code')
$anchor = $null
foreach ($candidate in $compiledCandidates) {
    $p = Join-Path $EVEInstall $candidate
    if (Test-Path $p) { $anchor = $p; break }
}
if (-not $anchor) { throw "Could not find existing compiled.code under $EVEInstall" }

$targetBackup = "$anchor.crucible_orig"
if (-not (Test-Path $targetBackup)) {
    Copy-Item $anchor $targetBackup -Force
    Write-Host "Backed up original compiled.code to $targetBackup"
}

$installTargets = New-Object System.Collections.Generic.HashSet[string]
[void]$installTargets.Add($anchor)
$scriptDir = Join-Path $EVEInstall 'script'
$scriptCode = Join-Path $scriptDir 'compiled.code'
if (Test-Path $scriptDir) { [void]$installTargets.Add($scriptCode) }
$binScriptCode = Join-Path $EVEInstall 'bin\script\compiled.code'
[void]$installTargets.Add($binScriptCode)
$binCode = Join-Path $EVEInstall 'bin\compiled.code'
if (Test-Path $binCode) { [void]$installTargets.Add($binCode) }

foreach ($targetCompiled in $installTargets) {
    $parent = Split-Path $targetCompiled
    if (-not (Test-Path $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
        Write-Host "Created $parent"
    }
    Copy-Item $compiledOut $targetCompiled -Force
    Write-Host "Installed rebuilt compiled.code at $targetCompiled" -ForegroundColor Green
}

# blue_patcher.exe only clears one JNZ; Crucible still needs the VerifyManifest bypass
# (see utils/dev/patcher.cpp and 42_patch_blue_manifest.ps1).
$manifestScript = Join-Path $PSScriptRoot '42_patch_blue_manifest.ps1'
if (Test-Path $manifestScript) {
    try {
        & $manifestScript -EVEInstall $EVEInstall
    } catch {
        Write-Warning "VerifyManifest patch failed - close EVE and run 42_patch_blue_manifest.ps1: $_"
    }
}

$pubkeyScript = Join-Path $PSScriptRoot '43_patch_blue_evecc_pubkey.ps1'
if (Test-Path $pubkeyScript) {
    try {
        & $pubkeyScript -EVEInstall $EVEInstall -ToolDir $ToolDir
    } catch {
        Write-Warning "evecc pubkey inject failed - run 43_patch_blue_evecc_pubkey.ps1: $_"
    }
}

Write-Host "`n--- current ini state ---"
foreach ($ini in 'common.ini', 'start.ini') {
    foreach ($subdir in '', 'bin\') {
        $path = Join-Path $EVEInstall "$subdir$ini"
        if (Test-Path $path) {
            Write-Host "$path :"
            Get-Content $path | Select-String -Pattern 'cryptoPack|server|build' |
                ForEach-Object { Write-Host "  $($_.Line)" }
            break
        }
    }
}

Write-Host "`nRebuild complete. Launch EVE and tail the client log for [sharedCQClient]." -ForegroundColor Green
Read-Host "`n(press Enter to close)"
