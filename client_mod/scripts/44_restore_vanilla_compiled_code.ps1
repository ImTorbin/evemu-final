<#
.SYNOPSIS
    Put back stock Crucible compiled.code and (by default) blue.dll from before evecc pubkey inject.

.DESCRIPTION
    Kill-report / other client Python mods ship inside rebuilt compiled.code. To undo them,
    copy compiled.code.crucible_orig over the live compiled.code paths.

    If you previously ran 43_patch_blue_evecc_pubkey.ps1, stock compiled.code is CCP-signed
    and will NOT verify while blue.dll still embeds evecc.keys.pub. This script therefore
    defaults to also restoring bin\blue.dll from blue.dll.pre_evecc_pubkey when present.

    Close EVE before running.

.PARAMETER EVEInstall
.PARAMETER SkipBlueDll
    If set, only restore compiled.code (you must fix blue.dll yourself for stock scripts).
#>
[CmdletBinding()]
param(
    [string] $EVEInstall,
    [switch] $SkipBlueDll
)

$ErrorActionPreference = 'Stop'
if (-not $EVEInstall) {
    foreach ($c in @("D:\Evemu\EVE", "${Env:ProgramFiles(x86)}\CCP\EVE", "$Env:ProgramFiles\CCP\EVE", "C:\CCP\EVE")) {
        if ($c -and (Test-Path (Join-Path $c 'bin\blue.dll'))) { $EVEInstall = $c; break }
    }
}
if (-not $EVEInstall) { throw "Pass -EVEInstall or use a default install path." }

$orig = Join-Path $EVEInstall 'script\compiled.code.crucible_orig'
if (-not (Test-Path $orig)) {
    throw "Missing $orig - cannot restore vanilla compiled.code."
}

$targets = @(
    (Join-Path $EVEInstall 'script\compiled.code'),
    (Join-Path $EVEInstall 'bin\script\compiled.code')
)
$moddedBackup = Join-Path $EVEInstall 'script\compiled.code.modded_backup'
foreach ($t in $targets) {
    if (-not (Test-Path $t)) { continue }
    if (-not (Test-Path $moddedBackup)) {
        Copy-Item $t $moddedBackup -Force
        Write-Host "Backed up current modded blob to $moddedBackup" -ForegroundColor Cyan
    }
    Copy-Item $orig $t -Force
    Write-Host "Restored vanilla compiled.code -> $t" -ForegroundColor Green
}

if (-not $SkipBlueDll) {
    $prePub = Join-Path $EVEInstall 'bin\blue.dll.pre_evecc_pubkey'
    $dll = Join-Path $EVEInstall 'bin\blue.dll'
    if (Test-Path $prePub) {
        Copy-Item $prePub $dll -Force
        Write-Host "Restored blue.dll from blue.dll.pre_evecc_pubkey (CCP pubkey + prior patches)." -ForegroundColor Green
    } else {
        Write-Warning "No bin\blue.dll.pre_evecc_pubkey - blue.dll unchanged. Stock compiled.code may fail Signature check until you restore CCP public key (re-run 43's inverse manually or repair client)."
    }
} else {
    Write-Host "Skipped blue.dll restore (-SkipBlueDll)." -ForegroundColor Yellow
}

Write-Host "`nDone. Launch with stock client scripts. To return to modded build, copy compiled.code.modded_backup back and re-run 43 if needed." -ForegroundColor Green
