# UniWiki combat sites → EVEmu dungeons

Source: [Combat site](https://wiki.eveuniversity.org/Combat_site) (CC BY-SA).

## Requirements

- Python 3.10+
- Network access to `wiki.eveuniversity.org` (unless using `--skip-fetch`)
- MySQL `invTypes` export for ship name resolution

## Export `invTypes` from your EVEmu database

```bash
mysql -N -B -uUSER -pPASS evemu -e "SELECT typeID, typeName, groupID FROM invTypes" > invtypes.csv
```

(Optional) First line header:

```text
typeID,typeName,groupID
```

## Tranquility `types.jsonl` (static export zip)

If you have `eve-online-static-data-*-jsonl.zip` (e.g. from CCP’s static data releases), build the same TSV without MySQL:

```bash
cd tools
python combat_sites_wiki/sde_jsonl_invtypes.py --zip /path/to/eve-online-static-data-3316380-jsonl.zip -o ../build/invtypes_from_sde.tsv
python -m combat_sites_wiki --inv-types-csv ../build/invtypes_from_sde.tsv ...
```

Names use the English locale (`name.en`). For spawns in **EVEmu**, your live DB export still matches what the server actually has; SDE is useful for wiki ↔ TQ name checks and overrides.

## Run (from `tools/` directory)

```bash
cd tools
python -m combat_sites_wiki \
  --inv-types-csv /path/to/invtypes.csv \
  --out-json ../build/combat_sites_uniwiki.json \
  --out-report ../build/combat_sites_report.txt \
  --out-sql ../sql/migrations/YYYYMMDDHHMMSS-combat_sites_uniwiki.sql \
  --max-sites 500 \
  --delay 1.0
```

By default **`--ensure-all-dungeons`** is on: every hub-linked site gets a `dunDungeons` row. If UniWiki lists no ships that resolve to `invTypes`, the tool drops in one **faction-appropriate placeholder rat** (see `faction_map.fallback_typeid_for_faction`). Use **`--no-ensure-all-dungeons`** to skip sites with zero resolved ships (older behavior).

NPC spawn lists on UniWiki are mostly **`{{NPCTableRow|…}}`** templates; the crawler expands slash-separated names (e.g. `Gistii Ambusher/ Hunter`, `Ripper/Shatter Alvior`) into hull names before matching SDE/MySQL `typeName`.

Emitted **`dunDungeons.archetypeID`** is always **7** (Dungeon::Type::Anomaly) so `DungeonMgr::GetRandomDungeon` can select these sites when the server spawns **cosmic anomalies** (probeless scanner tab). Wiki archetype metadata remains in `--out-json` as `archetype_id`.

Optional overrides when the wiki name does not match `typeName` exactly (JSON map: `"Guristas Kestrel": 12345`).

```bash
python -m combat_sites_wiki ... --overrides combat_sites_wiki/overrides.example.json
```

## Offline hub test

Save wikitext of `Combat_site` to `hub.txt` and:

```bash
python -m combat_sites_wiki --skip-fetch --hub-file hub.txt --inv-types-csv invtypes.csv --out-json ...
```

Child pages will be empty unless you also fetch them (omit `--skip-fetch` for full crawl).

## ID ranges

Generated `dungeonID` starts at **9200000**; `roomID` at **19200000**, step 10. Adjust in `cli.py` if they collide with your data.

## ESI verification (optional)

[`/universe/types/{id}/`](https://esi.evetech.net/ui/#/Universe/get_universe_types_type_id) is **public** on Tranquility — no OAuth **client secret** is required. CCP still expects a descriptive **User-Agent** (your app name + contact).

After resolving ships from `invTypes`, you can cross-check each emitted **typeID** against live ESI:

```bash
python -m combat_sites_wiki ... --inv-types-csv invtypes.csv --esi-verify \
  --esi-user-agent "YourApp/1.0 (mailto:you@example.com)"
```

Responses are cached under `build/esi_cache/` (next to `--out-json`) unless you set `--esi-cache-dir`.

Optional **`EVE_ESI_ACCESS_TOKEN`** or **`--esi-access-token`**: not needed for `universe/types`; use if you later call authenticated ESI routes.

## Limitations

- Child-page NPC extraction is heuristic; many sites need `overrides` or wiki cleanup.
- Fuzzy matching can mis-resolve similar names; review `combat_sites_report.txt`.
- EVEmu does not model acceleration gates, triggers, or escalations per pocket; SQL seeds a single room with a flat spawn layout.
- ESI describes **Tranquility**; your DB may differ for custom or outdated rows.
