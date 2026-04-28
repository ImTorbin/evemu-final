<#
.SYNOPSIS
    Extra blue.dll patches: bypass VerifyManifest-style JNZ +0x11c branches (evemu utils/dev/patcher.cpp).

.DESCRIPTION
    community blue_patcher.exe only NOPs one JNZ in the compiled-code signature path.
    Build 360229 can still hit SystemExit('Signature check failed') until every
    `0F 85 1C 01 00 00` failure branch that remains after the first bypass is patched:
      0F 85 1C 01  ->  E9 1D 01 00  (first 4 bytes only)

    This script patches two anchored sites on known Crucible blue.dll builds.

    Close the EVE client and launcher before running.

.PARAMETER EVEInstall
    Install root (contains bin\blue.dll).
#>
[CmdletBinding()]
param(
    [string] $EVEInstall
)

$ErrorActionPreference = 'Stop'
if (-not $EVEInstall) {
    foreach ($c in @("D:\Evemu\EVE", "${Env:ProgramFiles(x86)}\CCP\EVE", "$Env:ProgramFiles\CCP\EVE", "C:\CCP\EVE")) {
        if ($c -and (Test-Path (Join-Path $c 'bin\blue.dll'))) { $EVEInstall = $c; break }
    }
}
if (-not $EVEInstall) { throw "Pass -EVEInstall or put the client at a known path." }

$dll = Join-Path $EVEInstall 'bin\blue.dll'
if (-not (Test-Path $dll)) { throw "Missing $dll" }

function Find-AnchorOffset {
    param([byte[]] $Haystack, [byte[]] $Needle)
    for ($i = 0; $i -le $Haystack.Length - $Needle.Length; $i++) {
        $ok = $true
        for ($j = 0; $j -lt $Needle.Length; $j++) {
            if ($Haystack[$i + $j] -ne $Needle[$j]) { $ok = $false; break }
        }
        if ($ok) { return $i }
    }
    return -1
}

$bytes = [IO.File]::ReadAllBytes($dll)

# Site A: test eax,eax; jnz +0x11c
$anchorA = [byte[]](0x85, 0xC0, 0x0F, 0x85, 0x1C, 0x01, 0x00, 0x00)
# Site B: cmp [esp+38], ebp; jnz +0x11c
$anchorB = [byte[]](0x39, 0x6C, 0x24, 0x38, 0x0F, 0x85, 0x1C, 0x01)

$jobs = @(
    @{ Name = 'test eax / manifest'; Anchor = $anchorA; PatchRel = 2 }
    @{ Name = 'cmp [esp+38]';       Anchor = $anchorB; PatchRel = 4 }
)

$patchOffsets = New-Object System.Collections.Generic.List[int]
foreach ($job in $jobs) {
    $idx = Find-AnchorOffset $bytes $job.Anchor
    if ($idx -lt 0) {
        if ($job.Name -eq 'test eax / manifest') {
            Write-Host "Site A skipped (already patched or different layout)." -ForegroundColor DarkGray
        } elseif ($job.Name -eq 'cmp [esp+38]' -and (Find-AnchorOffset $bytes ([byte[]](0x39, 0x6C, 0x24, 0x38, 0xE9, 0x1D, 0x01))) -ge 0) {
            Write-Host "Site B skipped (already patched)." -ForegroundColor DarkGray
        } else {
            Write-Warning "Anchor not found: $($job.Name)"
        }
        continue
    }
    $patchOff = $idx + $job.PatchRel
    if ($bytes[$patchOff] -ne 0x0F -or $bytes[$patchOff + 1] -ne 0x85) {
        Write-Warning "Unexpected bytes at 0x$('{0:X}' -f $patchOff) for $($job.Name); skip."
        continue
    }
    [void]$patchOffsets.Add($patchOff)
}

if ($patchOffsets.Count -eq 0) {
    # Unpatched anchors missing, but both bypasses may already be applied (bytes change).
    $patchedA = (Find-AnchorOffset $bytes ([byte[]](0x85, 0xC0, 0xE9, 0x1D, 0x01, 0x00))) -ge 0
    $patchedB = (Find-AnchorOffset $bytes ([byte[]](0x39, 0x6C, 0x24, 0x38, 0xE9, 0x1D, 0x01))) -ge 0
    if ($patchedA -and $patchedB) {
        Write-Host "blue.dll already has both manifest bypasses (nothing to write)." -ForegroundColor Green
        exit 0
    }
    throw "No patch sites found - wrong blue.dll build? Compare with Crucible 360229."
}

$toApply = New-Object System.Collections.Generic.List[int]
foreach ($po in ($patchOffsets | Sort-Object -Unique)) {
    if ($bytes[$po] -eq 0xE9 -and $bytes[$po + 1] -eq 0x1D) { continue }
    [void]$toApply.Add($po)
}

if ($toApply.Count -eq 0) {
    Write-Host "All manifest bypass sites already patched." -ForegroundColor Green
    exit 0
}

$pre = Join-Path (Split-Path $dll) 'blue.dll.pre_manifest_fix'
if (-not (Test-Path $pre)) {
    Copy-Item $dll $pre -Force
    Write-Host "Backed up to $pre"
}

foreach ($po in $toApply) {
    $bytes[$po + 0] = 0xE9
    $bytes[$po + 1] = 0x1D
    $bytes[$po + 2] = 0x01
    $bytes[$po + 3] = 0x00
    Write-Host "Patched JNZ at 0x$('{0:X}' -f $po)" -ForegroundColor Green
}
[IO.File]::WriteAllBytes($dll, $bytes)
Write-Host "Done. Try launching the client."
