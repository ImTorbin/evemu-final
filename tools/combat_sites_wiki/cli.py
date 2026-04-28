#!/usr/bin/env python3
"""Crawl UniWiki Combat_site hub + child pages; emit JSON + SQL migration."""

from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

from .child_parser import parse_child_page
from .faction_map import faction_id_from_wiki_name, fallback_typeid_for_faction
from .hub_parser import parse_hub
from .esi_client import DEFAULT_ESI_BASE, DEFAULT_UA, esi_compare_to_invtypes
from .sql_emit import DungeonSQLSpec, emit_migration
from .typeid_resolve import inv_map_by_type_id, load_inv_types_csv, load_overrides_json, resolve_names
from .wiki_api import get_page_wikitext

DUNGEON_ID_BASE = 9_200_000
ROOM_ID_BASE = 19_200_000
STEP = 10
OFFSET_XY = 15000.0
# DungeonMgr::GetRandomDungeon(archetype) matches dunDungeons.archetypeID to CosmicSignature.dungeonType.
# Cosmic anomalies use Dungeon::Type::Anomaly (7); unrated probe sites use 8. Emit as 7 so sites appear in the anomaly pool.
SQL_DUNGEON_ARCHETYPE_ANOMALY = 7


def main() -> int:
    ap = argparse.ArgumentParser(description="UniWiki combat sites → EVEmu dungeons")
    ap.add_argument("--api", default="https://wiki.eveuniversity.org/api.php")
    ap.add_argument("--hub-title", default="Combat_site")
    ap.add_argument("--inv-types-csv", type=Path, help="CSV: typeID,typeName[,groupID]")
    ap.add_argument("--overrides", type=Path, help="JSON map shipName -> typeID")
    ap.add_argument("--max-sites", type=int, default=400, help="Max child pages to fetch")
    ap.add_argument("--delay", type=float, default=1.0)
    ap.add_argument("--out-json", type=Path, default=Path("build/combat_sites_uniwiki.json"))
    ap.add_argument("--out-report", type=Path, default=Path("build/combat_sites_report.txt"))
    ap.add_argument("--out-sql", type=Path, help="If set, write migration SQL here")
    ap.add_argument("--min-ships", type=int, default=1)
    ap.add_argument(
        "--fallback-typeid",
        type=int,
        default=0,
        help="If child page resolves no ships, use this invTypes typeID once (e.g. common rat frigate)",
    )
    ap.add_argument(
        "--ensure-all-dungeons",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Emit a dungeon for every hub site: use a faction-appropriate placeholder rat if nothing resolves",
    )
    ap.add_argument(
        "--max-dungeons-sql",
        type=int,
        default=0,
        help="Cap dungeons written to SQL (0 = no cap)",
    )
    ap.add_argument("--skip-fetch", action="store_true", help="Read hub wikitext from --hub-file")
    ap.add_argument("--hub-file", type=Path, help="Local wikitext file (with --skip-fetch)")
    ap.add_argument(
        "--from-json",
        type=Path,
        help="Skip wiki crawl; use a prior --out-json file (regenerate report/SQL only)",
    )
    ap.add_argument("--no-fuzzy", action="store_true")
    ap.add_argument(
        "--esi-verify",
        action="store_true",
        help="After resolving ships, compare typeIDs to ESI /universe/types/ (Tranquility)",
    )
    ap.add_argument("--esi-base-url", default=DEFAULT_ESI_BASE, help="ESI root, e.g. https://esi.evetech.net/latest")
    ap.add_argument(
        "--esi-user-agent",
        default=DEFAULT_UA,
        help="Required by ESI; include contact (app name, URL, or mailto)",
    )
    ap.add_argument("--esi-delay", type=float, default=0.25, help="Seconds between ESI requests")
    ap.add_argument(
        "--esi-cache-dir",
        type=Path,
        default=None,
        help="Cache ESI JSON per typeID (default: <out-json parent>/esi_cache when --esi-verify)",
    )
    ap.add_argument(
        "--esi-access-token",
        default=None,
        help="Optional Bearer token; public universe/types does not need OAuth. Env: EVE_ESI_ACCESS_TOKEN",
    )
    args = ap.parse_args()

    enriched: list[dict]
    if args.from_json and args.from_json.is_file():
        enriched = json.loads(args.from_json.read_text(encoding="utf-8"))
        for e in enriched:
            e.pop("resolved_ships", None)
            e.pop("unresolved_ships", None)
            e.pop("fuzzy_matches", None)
            e.pop("used_fallback_typeid", None)
        print(f"Loaded {len(enriched)} sites from {args.from_json}", flush=True)
    else:
        if args.skip_fetch:
            if not args.hub_file or not args.hub_file.is_file():
                print("Need --hub-file with --skip-fetch", file=sys.stderr)
                return 2
            hub_text = args.hub_file.read_text(encoding="utf-8", errors="replace")
        else:
            print(f"Fetching hub {args.hub_title!r}...", flush=True)
            hub_text = get_page_wikitext(args.hub_title, api_url=args.api, delay_s=args.delay)
            if not hub_text:
                print("Failed to fetch hub page", file=sys.stderr)
                return 1

        sites = parse_hub(hub_text)
        print(f"Hub parsed: {len(sites)} unique site records", flush=True)

        # Sort for stable IDs
        sites.sort(key=lambda s: s.wiki_title.lower())

        enriched = []
        for i, rec in enumerate(sites[: args.max_sites]):
            if args.skip_fetch and args.hub_file:
                wt = None
            else:
                if i % 50 == 0:
                    print(f"  Child {i+1}/{min(len(sites), args.max_sites)} {rec.wiki_title}", flush=True)
                wt = get_page_wikitext(rec.wiki_title, api_url=args.api, delay_s=args.delay)
            parsed = parse_child_page(rec.wiki_title, wt or "")
            d = {
                "wiki_title": rec.wiki_title,
                "display_name": rec.display_name,
                "archetype_id": rec.archetype_id,
                "faction_wiki": rec.faction_wiki,
                "faction_id": rec.faction_id,
                "ded_rating": rec.ded_rating,
                "source_section": rec.source_section,
                "raw_ship_names": parsed.raw_ship_names,
                "spawn_count": len(parsed.spawns),
            }
            enriched.append(d)

    args.out_json.parent.mkdir(parents=True, exist_ok=True)
    args.out_json.write_text(json.dumps(enriched, indent=2), encoding="utf-8")
    print(f"Wrote {args.out_json}", flush=True)

    report_lines = [
        f"Generated: {datetime.now(timezone.utc).isoformat()}",
        f"Sites (capped): {len(enriched)}",
        "",
    ]

    if not args.inv_types_csv:
        report_lines.append("No --inv-types-csv; skipping SQL. Export invTypes from MySQL:")
        report_lines.append(
            '  mysql -N -B -e "SELECT typeID, typeName, groupID FROM invTypes" db > invtypes.csv'
        )
        args.out_report.parent.mkdir(parents=True, exist_ok=True)
        args.out_report.write_text("\n".join(report_lines), encoding="utf-8")
        print(f"Wrote {args.out_report}", flush=True)
        return 0

    inv_map = load_inv_types_csv(args.inv_types_csv)
    inv_by_tid = inv_map_by_type_id(inv_map)
    overrides = load_overrides_json(args.overrides) if args.overrides else {}
    print(f"Loaded {len(inv_map)} invTypes rows", flush=True)

    esi_cache: Path | None = args.esi_cache_dir
    if args.esi_verify and esi_cache is None:
        esi_cache = args.out_json.parent / "esi_cache"

    specs: list[DungeonSQLSpec] = []
    next_d = DUNGEON_ID_BASE
    next_r = ROOM_ID_BASE

    for entry in enriched:
        names = entry["raw_ship_names"]
        rep = resolve_names(names, inv_map, overrides, fuzzy=not args.no_fuzzy)
        entry["unresolved_ships"] = rep.unresolved
        entry["fuzzy_matches"] = [{"from": a, "to": b} for a, b in rep.fuzzy_used]

        report_lines.append(f"== {entry['wiki_title']} ==")
        if rep.unresolved:
            report_lines.append("  UNRESOLVED: " + ", ".join(rep.unresolved[:20]))
            if len(rep.unresolved) > 20:
                report_lines.append(f"  ... +{len(rep.unresolved)-20} more")
        if rep.fuzzy_used:
            report_lines.append(
                "  FUZZY: " + ", ".join(f"{a}→{b}" for a, b in rep.fuzzy_used[:10])
            )

        resolved_rows = list(rep.resolved)
        used_fallback = False
        entry_fid = int(entry["faction_id"]) or faction_id_from_wiki_name(entry.get("faction_wiki") or "") or 500022
        if len(resolved_rows) < args.min_ships and args.fallback_typeid:
            resolved_rows = [("fallback_placeholder", args.fallback_typeid, 0)]
            used_fallback = True
            report_lines.append(f"  FALLBACK typeID {args.fallback_typeid} (no wiki ships resolved)")
        elif len(resolved_rows) < args.min_ships and args.ensure_all_dungeons:
            ftid = fallback_typeid_for_faction(entry_fid)
            gid_fb = 0
            for invv in inv_map.values():
                if invv.type_id == ftid:
                    gid_fb = invv.group_id
                    break
            resolved_rows = [("faction_fallback", ftid, gid_fb)]
            used_fallback = True
            report_lines.append(f"  PLACEHOLDER rat typeID {ftid} (faction default; no resolved wiki ships)")
        elif len(resolved_rows) < args.min_ships:
            entry["resolved_ships"] = [{"name": n, "typeID": tid, "groupID": gid} for n, tid, gid in rep.resolved]
            report_lines.append(f"  SKIP SQL: only {len(resolved_rows)} resolved ships")
            continue

        entry["resolved_ships"] = [{"name": n, "typeID": tid, "groupID": gid} for n, tid, gid in resolved_rows]
        entry["used_fallback_typeid"] = used_fallback

        if args.esi_verify:
            tids = [tid for _, tid, _ in resolved_rows]
            esi_details, esi_rlines = esi_compare_to_invtypes(
                tids,
                inv_by_tid,
                base_url=args.esi_base_url,
                user_agent=args.esi_user_agent,
                access_token=(args.esi_access_token or "").strip() or None,
                delay_s=args.esi_delay,
                cache_dir=esi_cache,
            )
            entry["esi_verify"] = esi_details
            report_lines.extend(esi_rlines)

        fid = entry_fid
        dname = entry["display_name"][:100]
        objects: list[tuple[int, int, float, float, float]] = []
        for idx, (name, tid, gid) in enumerate(resolved_rows):
            x = (idx % 8) * OFFSET_XY
            y = ((idx // 8) % 4) * OFFSET_XY
            z = (idx // 32) * 5000.0
            objects.append((tid, gid, x, y, z))

        specs.append(
            DungeonSQLSpec(
                dungeon_id=next_d,
                room_id=next_r,
                dungeon_name=dname,
                faction_id=fid,
                archetype_id=SQL_DUNGEON_ARCHETYPE_ANOMALY,
                objects=objects,
            )
        )
        next_d += STEP
        next_r += STEP

    sql_specs = specs
    if args.max_dungeons_sql and len(specs) > args.max_dungeons_sql:
        sql_specs = specs[: args.max_dungeons_sql]
        report_lines.append(
            f"\n(SQL capped: emitting {len(sql_specs)} of {len(specs)} dungeons)"
        )

    args.out_json.write_text(json.dumps(enriched, indent=2), encoding="utf-8")

    args.out_report.parent.mkdir(parents=True, exist_ok=True)
    args.out_report.write_text("\n".join(report_lines), encoding="utf-8")
    print(f"Wrote {args.out_report} ({len(specs)} dungeons built, {len(sql_specs)} in SQL)", flush=True)

    if args.out_sql and sql_specs:
        last = sql_specs[-1]
        hdr = (
            f"ids {DUNGEON_ID_BASE}-{last.dungeon_id}, rooms {ROOM_ID_BASE}-{last.room_id}, "
            f"count {len(sql_specs)}"
        )
        sql = emit_migration(sql_specs, hdr)
        args.out_sql.parent.mkdir(parents=True, exist_ok=True)
        args.out_sql.write_text(sql, encoding="utf-8")
        print(f"Wrote {args.out_sql}", flush=True)

    return 0


def entry() -> int:
    return main()


if __name__ == "__main__":
    raise SystemExit(main())
