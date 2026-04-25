<#
.SYNOPSIS
    Decompile the dumped Crucible client tree to source.

.PARAMETER CodeRoot
    The output of 10_dump_client.ps1 (tree of .pyj/.pyc files).

.PARAMETER ToolDir
    Folder containing evedec.exe.

.PARAMETER Python27
    Path to a Python 2.7 32-bit interpreter with uncompyle2 + uncompyle6
    installed.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $CodeRoot,
    [string] $ToolDir = (Join-Path $PSScriptRoot '..\tools'),
    [string] $Python27 = (Get-Item -ErrorAction SilentlyContinue 'C:\Python27\python.exe').FullName
)

$ErrorActionPreference = 'Stop'

$evedec = Join-Path $ToolDir 'evedec.exe'
if (-not (Test-Path $evedec)) {
    throw "evedec.exe not found at $evedec."
}
if (-not $Python27 -or -not (Test-Path $Python27)) {
    throw "Python 2.7 interpreter not found. Pass -Python27 explicitly."
}
if (-not (Test-Path $CodeRoot)) {
    throw "CodeRoot $CodeRoot does not exist. Run 10_dump_client.ps1 first."
}

# 1. evedec rewrites .pyj into .py in place. It is faster than uncompyle but
#    will fail on a few hundred modules with 'try/except' or 'assert' edges.
Write-Host "==> evedec on $CodeRoot"
& $evedec $CodeRoot
if ($LASTEXITCODE -ne 0) {
    Write-Warning "evedec exited $LASTEXITCODE; continuing to uncompyle pass."
}

# 2. Sweep what evedec missed. Anything that still has a .pyj sibling
#    without a .py sibling needs to be retried with uncompyle2 then
#    uncompyle6.
$pyjFiles = Get-ChildItem -Path $CodeRoot -Recurse -Filter *.pyj -File
$pendings = @()
foreach ($pyj in $pyjFiles) {
    $pyTwin = [IO.Path]::ChangeExtension($pyj.FullName, '.py')
    if (-not (Test-Path $pyTwin) -or (Get-Item $pyTwin).Length -eq 0) {
        $pendings += $pyj
    }
}

Write-Host "Files needing uncompyle: $($pendings.Count)"
$failed = @()
foreach ($pyj in $pendings) {
    $target = [IO.Path]::ChangeExtension($pyj.FullName, '.py')
    & $Python27 -m uncompyle2 -o $target $pyj.FullName 2>$null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $target)) {
        & $Python27 -m uncompyle6 -o $target $pyj.FullName 2>$null
    }
    if (-not (Test-Path $target) -or (Get-Item $target).Length -eq 0) {
        $failed += $pyj.FullName
    }
}

if ($failed.Count -gt 0) {
    Write-Warning "$($failed.Count) modules could not be decompiled. Sample:"
    $failed | Select-Object -First 10 | ForEach-Object { Write-Host "  $_" }
    Write-Warning "Most CQ-relevant modules are usually clean; treat the failures as non-blocking unless one of the four target modules is in the list."
} else {
    Write-Host "All .pyj decompiled successfully." -ForegroundColor Green
}
