# Multiplayer Captain's Quarters — Client Mod

This directory ships the client-side and operational pieces required by the
*Multiplayer CQ client mod* plan. The C++ server work is in
`src/eve-server/...` (already wired into the existing build); everything in
this directory has to be applied by hand to a Crucible client install
because Crucible loads its Python out of an encrypted, signed
`compiled.code` blob — not from loose `.py` files.

## Files in here

| Path | Purpose |
| --- | --- |
| `scripts/00_check_prereqs.ps1` | Validates Python 2.7, evecc, evedec, blue_patcher, locates EVE install, prints `start.ini` build number. |
| `scripts/10_dump_client.ps1` | Runs `evecc dumpkeys` + `evecc dumpcode` against `<EVEInstall>\bin\script\compiled.code`. |
| `scripts/11_decompile_client.ps1` | Runs `evedec` then `uncompyle2`/`uncompyle6` over the dumped tree. |
| `scripts/12_grep_modules.ps1` | Greps the decompiled tree for the four real module paths the plan needs you to confirm. |
| `scripts/40_rebuild_client.ps1` | `evecc compilecode` + `blue_patcher` to produce a new `compiled.code` and a CRC-disabled `blue.dll`. |
| `scripts/50_two_client_smoke.md` | The two-client verification checklist. |
| `sql/locationScenes_helper.sql` | Idempotent verification + seed helper for `locationScenes` (the table that maps station / system → CQ scene). |
| `eve/client/script/environment/sharedCQ.py` | The new client service `svc.sharedCQClient`. **This is the core of the mod.** |
| `eve/client/script/ui/station/captainsQuartersRoom_patches.md` | Source-level diff guide for the CQ room module. Apply once Phase 1 confirms the real path. |

## Suggested order of operations

1. `pwsh client_mod/scripts/00_check_prereqs.ps1 -EVEInstall "C:\Program Files (x86)\CCP\EVE"`
2. Inspect `client_mod/sql/locationScenes_helper.sql` and adapt the `INSERT` block to your dev station, then run it against the EVEmu DB.
3. `pwsh client_mod/scripts/10_dump_client.ps1 -EVEInstall ... -Out .\eve_code`
4. `pwsh client_mod/scripts/11_decompile_client.ps1 -CodeRoot .\eve_code`
5. `pwsh client_mod/scripts/12_grep_modules.ps1 -CodeRoot .\eve_code` — record the four module paths it prints.
6. Rebuild and deploy the EVEmu server (server-side enrichment in `src/eve-server/...` already lands when you build).
7. Apply the patch from `eve/client/script/ui/station/captainsQuartersRoom_patches.md` to the real CQ room module path you recorded in step 5, and copy `eve/client/script/environment/sharedCQ.py` into the dumped tree.
8. `pwsh client_mod/scripts/40_rebuild_client.ps1 -CodeRoot .\eve_code -EVEInstall ...`
9. Walk through `scripts/50_two_client_smoke.md`.

Steps 1, 3-5, and 8 are exclusively client-side work. Step 2 is a DB op
against EVEmu's main database. Steps 6-7 are mixed.

## Why no loose-file override

`script.stuff` does not exist in Crucible. All Python lives in
`<EVEInstall>\bin\script\compiled.code`. The blue runtime checks the file
against a CRC stored in `blue.dll`; loose `.py` files in `bin\script\` are
not consulted by the loader. There is no documented `triboot` /
`usePackedScripts=0` flag in this build. The only practical mod path is to
rebuild `compiled.code` and patch the CRC check out of `blue.dll`.
