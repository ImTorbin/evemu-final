#!/usr/bin/env python3
"""
Convert EVE static export `types.jsonl` (inside eve-online-static-data-*-jsonl.zip or standalone)
to a tab-separated file compatible with `combat_sites_wiki --inv-types-csv`.

Each line: typeID, typeName, groupID (English name from `name.en`).
"""

from __future__ import annotations

import argparse
import csv
import json
import zipfile
from pathlib import Path
from typing import BinaryIO, Iterator, TextIO


def _name_en(obj: dict) -> str:
    n = obj.get("name")
    if isinstance(n, dict):
        return str(n.get("en") or n.get("de") or next(iter(n.values()), ""))
    if isinstance(n, str):
        return n
    return ""


def _iter_types_lines(fh: BinaryIO) -> Iterator[str]:
    for raw in fh:
        line = raw.decode("utf-8", errors="replace").strip()
        if line:
            yield line


def main() -> int:
    ap = argparse.ArgumentParser(description="types.jsonl → invTypes-style TSV for combat_sites_wiki")
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--zip", type=Path, help="Path to eve-online-static-data-*-jsonl.zip")
    g.add_argument("--types-jsonl", type=Path, help="Path to types.jsonl")
    ap.add_argument("-o", "--out", type=Path, required=True, help="Output .tsv (typeID, typeName, groupID)")
    args = ap.parse_args()

    if args.zip:
        z = zipfile.ZipFile(args.zip)
        try:
            names = z.namelist()
            inner = "types.jsonl"
            if inner not in names:
                ap.error(f"No {inner!r} in zip (got {len(names)} files)")
            fh = z.open(inner)
        except Exception:
            z.close()
            raise
    else:
        z = None
        fh = args.types_jsonl.open("rb")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    n = 0
    try:
        with args.out.open("w", encoding="utf-8", newline="") as out:
            w = csv.writer(out, delimiter="\t")
            for line in _iter_types_lines(fh):
                obj = json.loads(line)
                tid = int(obj.get("_key", -1))
                if tid < 0:
                    continue
                gid = int(obj.get("groupID", 0))
                tname = _name_en(obj).replace("\t", " ").strip()
                w.writerow([tid, tname, gid])
                n += 1
    finally:
        if z is not None:
            z.close()

    print(f"Wrote {n} rows to {args.out}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
