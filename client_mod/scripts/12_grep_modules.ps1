<#
.SYNOPSIS
    Locate the four modules the Multiplayer CQ plan needs to patch.

.PARAMETER CodeRoot
    Decompiled client tree (output of 11_decompile_client.ps1).

.NOTES
    Prints the candidate module path for each of:
        - CQ room scene module
        - PaperDollAvatar / paperdoll service
        - sceneManager service
        - BracketClient module
        - ProximityTriggerService (origin of KeyError: 1 the original session
          fix unblocked)

    The plan's notes are best-guess; this script is the ground truth.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $CodeRoot
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $CodeRoot)) { throw "CodeRoot $CodeRoot not found." }

$queries = [ordered]@{
    'CQ room module'         = '(captainsQuartersRoom|class\s+CaptainsQuartersRoom|enterCQ|OnEnterCQ)'
    'PaperDollAvatar class'  = 'class\s+PaperDollAvatar'
    'paperDoll service'      = "__servicename__\s*=\s*['""]paperDoll['""]"
    'sceneManager service'   = "__servicename__\s*=\s*['""]sceneManager['""]"
    'BracketClient class'    = 'class\s+BracketClient'
    'ProximityTrigger svc'   = '(class\s+ProximityTriggerService|RegisterEnterCallback)'
}

foreach ($label in $queries.Keys) {
    $pattern = $queries[$label]
    Write-Host ("`n=== {0} ===" -f $label) -ForegroundColor Cyan
    $hits = Get-ChildItem -Path $CodeRoot -Recurse -Filter *.py -File `
        | Select-String -Pattern $pattern -List
    if ($hits) {
        $hits | ForEach-Object { Write-Host ("  {0}:{1}  {2}" -f $_.Path, $_.LineNumber, $_.Line.Trim()) }
    } else {
        Write-Warning "no matches"
    }
}

Write-Host "`nRecord the file paths above into the plan's Phase 1 step 4 notes."
