<#
.SYNOPSIS
    Replace embedded CCP RSA public key in blue.dll with evecc.keys.pub (same 148-byte blob).

.DESCRIPTION
    Rebuilt compiled.code is signed with evecc.keys.priv. blue.dll still embeds ccp.keys.pub
    by default, so CryptVerifySignature keeps failing and Python exits with
    Signature check failed even after JNZ bypass patches.

    This script finds ccp.keys.pub bytes inside bin\blue.dll and overwrites them with
    client_mod\tools\evecc.keys.pub (must match the keys used to compile your bundle).

    Close EVE before running.

.PARAMETER EVEInstall
.PARAMETER ToolDir
#>
[CmdletBinding()]
param(
    [string] $EVEInstall,
    [string] $ToolDir
)

$ErrorActionPreference = 'Stop'
if (-not $ToolDir) { $ToolDir = Join-Path $PSScriptRoot '..\tools' }
if (-not $EVEInstall) {
    foreach ($c in @("D:\Evemu\EVE", "${Env:ProgramFiles(x86)}\CCP\EVE", "$Env:ProgramFiles\CCP\EVE", "C:\CCP\EVE")) {
        if ($c -and (Test-Path (Join-Path $c 'bin\blue.dll'))) { $EVEInstall = $c; break }
    }
}
if (-not $EVEInstall) { throw "Pass -EVEInstall or use a default install path." }

$dll = Join-Path $EVEInstall 'bin\blue.dll'
$ccpPub = Join-Path $ToolDir 'ccp.keys.pub'
$evePub = Join-Path $ToolDir 'evecc.keys.pub'
foreach ($p in $dll, $ccpPub, $evePub) {
    if (-not (Test-Path $p)) { throw "Missing $p" }
}

$bytes = [IO.File]::ReadAllBytes($dll)
$oldKey = [IO.File]::ReadAllBytes($ccpPub)
$newKey = [IO.File]::ReadAllBytes($evePub)
if ($oldKey.Length -ne 148 -or $newKey.Length -ne 148) { throw "Expected 148-byte RSA blobs." }

$matches = New-Object System.Collections.Generic.List[int]
for ($i = 0; $i -le $bytes.Length - $oldKey.Length; $i++) {
    $ok = $true
    for ($j = 0; $j -lt $oldKey.Length; $j++) {
        if ($bytes[$i + $j] -ne $oldKey[$j]) { $ok = $false; break }
    }
    if ($ok) { [void]$matches.Add($i) }
}

if ($matches.Count -eq 0) {
    $probe = -1
    for ($i = 0; $i -le $bytes.Length - $newKey.Length; $i++) {
        $ok = $true
        for ($j = 0; $j -lt $newKey.Length; $j++) {
            if ($bytes[$i + $j] -ne $newKey[$j]) { $ok = $false; break }
        }
        if ($ok) { $probe = $i; break }
    }
    if ($probe -ge 0) {
        Write-Host "blue.dll already embeds evecc.keys.pub at 0x$('{0:X}' -f $probe). Nothing to do." -ForegroundColor Green
        exit 0
    }
    throw "ccp.keys.pub blob not found in blue.dll - wrong client build?"
}
if ($matches.Count -gt 1) {
    throw "Multiple ccp.keys.pub matches: $($matches.Count). Refuse to patch blindly."
}

$off = $matches[0]
$already = $true
for ($j = 0; $j -lt $newKey.Length; $j++) {
    if ($bytes[$off + $j] -ne $newKey[$j]) { $already = $false; break }
}
if ($already) {
    Write-Host "Public key at 0x$('{0:X}' -f $off) already matches evecc.keys.pub." -ForegroundColor Green
    exit 0
}

$bak = Join-Path (Split-Path $dll) 'blue.dll.pre_evecc_pubkey'
if (-not (Test-Path $bak)) {
    Copy-Item $dll $bak -Force
    Write-Host "Backed up to $bak"
}

[Array]::Copy($newKey, 0, $bytes, $off, $newKey.Length)
[IO.File]::WriteAllBytes($dll, $bytes)
Write-Host "Replaced embedded pubkey at 0x$('{0:X}' -f $off) with evecc.keys.pub. Launch the client." -ForegroundColor Green
