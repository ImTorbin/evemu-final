<#
.SYNOPSIS
    Dump the Crucible client's compiled.code into an unpacked .pyj/.pyc tree.

.PARAMETER EVEInstall
    Root of an EVE Online Crucible install.

.PARAMETER Out
    Output directory where the dumped tree is written.

.PARAMETER ToolDir
    Folder containing evecc.exe.
#>
[CmdletBinding()]
param(
    [string] $EVEInstall,
    [string] $Out,
    [string] $ToolDir
)

$ErrorActionPreference = 'Stop'
trap {
    Write-Host "`nUNHANDLED ERROR:" -ForegroundColor Red
    Write-Host $_ -ForegroundColor Red
    Read-Host "`n(press Enter to close)"
    exit 1
}

if (-not $ToolDir) { $ToolDir = Join-Path $PSScriptRoot '..\tools' }
if (-not $Out) { $Out = Join-Path $PSScriptRoot '..\eve_code' }

if (-not $EVEInstall) {
    foreach ($c in @("D:\Evemu\EVE", "${Env:ProgramFiles(x86)}\CCP\EVE", "$Env:ProgramFiles\CCP\EVE", "C:\CCP\EVE")) {
        if ($c -and (Test-Path (Join-Path $c 'bin\blue.dll'))) { $EVEInstall = $c; break }
    }
}
if (-not $EVEInstall) { throw "Pass -EVEInstall or place install at one of the auto-detect paths." }

$evecc = Join-Path $ToolDir 'evecc.exe'
if (-not (Test-Path $evecc)) { throw "evecc.exe not found at $evecc." }

$blue = Join-Path $EVEInstall 'bin\blue.dll'
if (-not (Test-Path $blue)) { throw "Required file missing: $blue" }

$compiled = $null
foreach ($candidate in @('script\compiled.code', 'bin\script\compiled.code', 'bin\compiled.code')) {
    $p = Join-Path $EVEInstall $candidate
    if (Test-Path $p) { $compiled = $p; break }
}
if (-not $compiled) { throw "compiled.code not found under $EVEInstall." }

if (-not (Test-Path $Out)) { New-Item -ItemType Directory -Force -Path $Out | Out-Null }

# evecc.exe embeds Python and needs PYTHONHOME pointed at its bundled stdlib.
$bundledPython = Join-Path $ToolDir 'Python'
if (Test-Path $bundledPython) {
    $env:PYTHONHOME = $bundledPython
    $env:PYTHONPATH = (Join-Path $bundledPython 'Lib') + ';' + (Join-Path $bundledPython 'Lib\site-packages')
    Write-Host "PYTHONHOME = $env:PYTHONHOME" -ForegroundColor DarkCyan
}

Write-Host "==> evecc dumpkeys -i $blue"
& $evecc dumpkeys -i $blue
if ($LASTEXITCODE -ne 0) { throw "evecc dumpkeys exited $LASTEXITCODE" }

Write-Host "==> evecc dumpcode -i $compiled -o $Out"
Push-Location $ToolDir
try {
    & $evecc dumpcode -i $compiled -o $Out
    if ($LASTEXITCODE -ne 0) { throw "evecc dumpcode exited $LASTEXITCODE" }
} finally {
    Pop-Location
}

$pyjCount = (Get-ChildItem -Path $Out -Recurse -Include *.pyj, *.pyc, *.py -File -ErrorAction SilentlyContinue).Count
Write-Host "Dumped $pyjCount .pyj/.pyc/.py files into $Out" -ForegroundColor Green

if ($pyjCount -eq 0) {
    throw "Output tree is empty. Check $ToolDir\evecc.log for evecc errors."
}

Read-Host "`n(press Enter to close)"
