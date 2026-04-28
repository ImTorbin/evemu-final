<#
.SYNOPSIS
    (Removed) Kill-report client mod.

.DESCRIPTION
    The kill-report UI experiment depended on fragile client bootstrap paths and has been
    removed from this repo. Restore stock behavior with:

      pwsh client_mod/scripts/44_restore_vanilla_compiled_code.ps1 -EVEInstall "<EVE root>"

    If you still have a patched CodeRoot (killReportWindow.py / entries.py), delete those
    changes before the next 40_rebuild_client run.

    Recover old files from git history if needed (commit before removal).
#>
[CmdletBinding()]
param()
Write-Error "41_patch_kill_report.ps1 is disabled: kill-report mod removed. Use 44_restore_vanilla_compiled_code.ps1 to restore stock compiled.code (and blue.dll when EVE is closed)."
exit 1
