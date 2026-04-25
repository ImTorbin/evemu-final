<#
.SYNOPSIS
    Validates the toolchain required to dump, decompile, and rebuild the
    Crucible client's compiled.code, and reports the install's build number.

.PARAMETER EVEInstall
    Root of an EVE Online Crucible install (the directory that contains
    bin\, res\, script.stuff, etc).

.PARAMETER ToolDir
    Optional. Folder containing evecc.exe, evedec.exe, and blue_patcher.exe.
    Defaults to a sibling 'tools' directory next to this script.

.EXAMPLE
    pwsh .\00_check_prereqs.ps1 -EVEInstall "C:\Program Files (x86)\CCP\EVE"
#>
[CmdletBinding()]
param(
    [string] $EVEInstall,
    [string] $ToolDir
)

# Always pause at end if we own the host (prevents the window from vanishing
# on right-click->"Run with PowerShell" and on every other terminal-exit
# path). Wrapped in try/finally so binding errors and mid-script exceptions
# both get caught.
$script:PauseOnExit = $true
trap {
    Write-Host "`nUNHANDLED ERROR:" -ForegroundColor Red
    Write-Host $_ -ForegroundColor Red
    if ($_.InvocationInfo) { Write-Host $_.InvocationInfo.PositionMessage -ForegroundColor DarkYellow }
    if ($script:PauseOnExit) { Read-Host "`n(press Enter to close)" }
    exit 1
}

if (-not $ToolDir) {
    if ($PSScriptRoot) {
        $ToolDir = Join-Path $PSScriptRoot '..\tools'
    } else {
        $ToolDir = Join-Path (Get-Location) 'client_mod\tools'
    }
}

# Auto-detect the Crucible install if -EVEInstall wasn't passed.
if (-not $EVEInstall) {
    $candidates = @(
        "$Env:ProgramFiles\CCP\EVE",
        "${Env:ProgramFiles(x86)}\CCP\EVE",
        "C:\CCP\EVE",
        "D:\CCP\EVE",
        "C:\EVE",
        "D:\EVE",
        "C:\Program Files\EVE",
        "C:\Program Files (x86)\EVE"
    )
    foreach ($c in $candidates) {
        if ($c -and (Test-Path (Join-Path $c 'bin\blue.dll'))) {
            $EVEInstall = $c
            Write-Host "[autodetect] EVEInstall = $EVEInstall" -ForegroundColor DarkCyan
            break
        }
    }
}
if (-not $EVEInstall) {
    Write-Host "EVEInstall not provided and could not be auto-detected." -ForegroundColor Red
    Write-Host "Re-run with the path to your Crucible install, e.g.:" -ForegroundColor Yellow
    Write-Host '    pwsh -NoExit -File .\client_mod\scripts\00_check_prereqs.ps1 -EVEInstall "C:\Program Files (x86)\CCP\EVE"' -ForegroundColor Yellow
    if ($script:PauseOnExit) { Read-Host "`n(press Enter to close)" }
    exit 1
}

$ErrorActionPreference = 'Continue'

function Write-Status($label, $ok, $detail) {
    $tag = if ($ok) { '[ OK ]' } else { '[FAIL]' }
    Write-Host ("{0} {1,-30} {2}" -f $tag, $label, $detail)
}

$failures = 0

# 1. Python 2.7
$python27 = $null
foreach ($candidate in @(
    "$Env:SystemDrive\Python27\python.exe",
    "$Env:LOCALAPPDATA\Programs\Python\Python27\python.exe",
    "$Env:ProgramFiles\Python27\python.exe",
    "${Env:ProgramFiles(x86)}\Python27\python.exe"
)) {
    if (Test-Path $candidate) { $python27 = $candidate; break }
}
if ($python27) {
    $ver = & $python27 -V 2>&1
    Write-Status 'python27' $true "$python27 ($ver)"
} else {
    Write-Status 'python27' $false 'not found; install Python 2.7 32-bit'
    $failures++
}

# 2. uncompyle2 / uncompyle6 — load via PYTHONHOME=$ToolDir\Python so we
#    discover the bundled site-packages whether or not the user ever ran
#    pip install.
$bundledPython = Join-Path $ToolDir 'Python'
$bundledLib = Join-Path $bundledPython 'Lib'
foreach ($pkg in 'uncompyle2', 'uncompyle6') {
    if ($python27) {
        $env:PYTHONHOME = $bundledPython
        $env:PYTHONPATH = "$bundledLib;$bundledLib\site-packages"
        $probe = & $python27 -c "import importlib, sys; sys.exit(0 if importlib.import_module('$pkg') else 1)" 2>&1
        $ok = $LASTEXITCODE -eq 0
        Remove-Item Env:PYTHONHOME, Env:PYTHONPATH -ErrorAction SilentlyContinue
        $detail = if ($ok) { 'importable' } else {
            if ($pkg -eq 'uncompyle6') { 'optional fallback; uncompyle2 covers most modules' }
            else { "missing; extract $pkg.zip into $bundledLib\site-packages" }
        }
        Write-Status $pkg $ok $detail
        if (-not $ok -and $pkg -eq 'uncompyle2') { $failures++ }
    } else {
        Write-Status $pkg $false 'skipped (no python27)'
        $failures++
    }
}

# 3. evecc + blue_patcher (.exe) and evedec (.py — community pack ships a
#    Python script, not a binary).
foreach ($exe in 'evecc.exe', 'blue_patcher.exe') {
    $path = Join-Path $ToolDir $exe
    if (Test-Path $path) { Write-Status $exe $true $path }
    else { Write-Status $exe $false "missing under $ToolDir"; $failures++ }
}
$evedecPy = Join-Path $ToolDir 'evedec.py'
if (Test-Path $evedecPy) {
    Write-Status 'evedec.py' $true $evedecPy
} else {
    $evedecExe = Join-Path $ToolDir 'evedec.exe'
    if (Test-Path $evedecExe) { Write-Status 'evedec.exe' $true $evedecExe }
    else { Write-Status 'evedec' $false "evedec.py / evedec.exe missing under $ToolDir"; $failures++ }
}

# 4. EVE install. Crucible distros put script\ and the ini files at either
#    the install root (your case) or under bin\. Probe both.
if (-not (Test-Path $EVEInstall)) {
    Write-Status 'eve install' $false "$EVEInstall does not exist"
    $failures++
} else {
    Write-Status 'eve install' $true $EVEInstall

    $compiled = $null
    foreach ($candidate in @('bin\script\compiled.code', 'script\compiled.code', 'bin\compiled.code')) {
        $p = Join-Path $EVEInstall $candidate
        if (Test-Path $p) { $compiled = $p; break }
    }
    if ($compiled) { Write-Status 'compiled.code' $true $compiled }
    else { Write-Status 'compiled.code' $false "not found under $EVEInstall (looked in bin\script, script, bin)"; $failures++ }

    $blue = Join-Path $EVEInstall 'bin\blue.dll'
    Write-Status 'blue.dll' (Test-Path $blue) $blue
    if (-not (Test-Path $blue)) { $failures++ }

    # Build number lives in common.ini for Crucible 360229; some older
    # distros put it in start.ini. Try both.
    $build = $null
    foreach ($iniName in 'common.ini', 'start.ini') {
        foreach ($iniDir in '', 'bin\') {
            $iniPath = Join-Path $EVEInstall ($iniDir + $iniName)
            if (Test-Path $iniPath) {
                $match = Select-String -Path $iniPath -Pattern 'build\s*=\s*(\d+)' | Select-Object -First 1
                if ($match) {
                    $build = $match.Matches[0].Groups[1].Value
                    Write-Status "$iniName build" $true "$iniPath -> build=$build"
                    break
                }
            }
        }
        if ($build) { break }
    }
    if (-not $build) {
        Write-Status 'build number' $false 'no build= line in start.ini or common.ini'
        $failures++
    } elseif ([int]$build -lt 360000 -or [int]$build -gt 420000) {
        Write-Warning "Build $build is outside the 360000-420000 Crucible window. Toolchain layouts may differ."
    }
}

if ($failures -eq 0) {
    Write-Host "`nAll prereqs satisfied." -ForegroundColor Green
    if ($script:PauseOnExit) { Read-Host "`n(press Enter to close)" }
    exit 0
} else {
    Write-Host "`n$failures prereq check(s) failed. See above." -ForegroundColor Red
    if ($script:PauseOnExit) { Read-Host "`n(press Enter to close)" }
    exit 1
}
