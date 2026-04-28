"""Resolve ship display names to typeID + groupID using invTypes export."""

from __future__ import annotations

import csv
import io
import json
import re
import unicodedata
from dataclasses import dataclass
from difflib import get_close_matches
from pathlib import Path
from typing import Dict, List, Optional, Tuple


def normalize_name(name: str) -> str:
    n = unicodedata.normalize("NFKC", name)
    n = n.replace("’", "'").replace("`", "'")
    n = re.sub(r"\s+", " ", n).strip().lower()
    n = re.sub(r"\s*\([^)]*\)\s*$", "", n)
    return n


def _read_text_for_csv(path: Path) -> str:
    """Decode invTypes export; PowerShell `docker exec ... > file` often writes UTF-16 LE."""
    raw = path.read_bytes()
    if raw.startswith(b"\xff\xfe"):
        return raw.decode("utf-16-le")
    if raw.startswith(b"\xfe\xff"):
        return raw.decode("utf-16-be")
    if raw.startswith(b"\xef\xbb\xbf"):
        return raw.decode("utf-8-sig")
    return raw.decode("utf-8", errors="replace")


@dataclass
class InvType:
    type_id: int
    type_name: str
    group_id: int


def inv_map_by_type_id(by_norm: Dict[str, InvType]) -> Dict[int, InvType]:
    """First wins for duplicate type_ids under different normalized keys."""
    out: Dict[int, InvType] = {}
    for inv in by_norm.values():
        if inv.type_id not in out:
            out[inv.type_id] = inv
    return out


def load_inv_types_csv(path: Path) -> Dict[str, InvType]:
    """Load typeID,typeName[,groupID] CSV or tab-separated MySQL -B export (header optional)."""
    by_norm: Dict[str, InvType] = {}
    text = _read_text_for_csv(path)
    sample = text[:4096]
    has_header = "typeid" in sample.lower() and "typename" in sample.lower()
    lines = text.splitlines()
    first_line = lines[0] if lines else ""
    delim = "\t" if first_line.count("\t") >= 2 else ","
    reader = csv.reader(io.StringIO(text), delimiter=delim)
    rows = list(reader)
    start = 1 if has_header else 0
    for row in rows[start:]:
        if len(row) < 2:
            continue
        try:
            tid = int(row[0].strip())
        except ValueError:
            continue
        tname = row[1].strip()
        gid = int(row[2].strip()) if len(row) > 2 and row[2].strip().isdigit() else 0
        inv = InvType(tid, tname, gid)
        key = normalize_name(tname)
        # Prefer first; duplicates: keep first published-like
        if key not in by_norm:
            by_norm[key] = inv
    return by_norm


def load_overrides_json(path: Path) -> Dict[str, int]:
    if not path.is_file():
        return {}
    data = json.loads(path.read_text(encoding="utf-8"))
    out: Dict[str, int] = {}
    for k, v in data.items():
        out[normalize_name(str(k))] = int(v)
    return out


@dataclass
class ResolveReport:
    resolved: List[Tuple[str, int, int]]  # name, typeID, groupID
    unresolved: List[str]
    fuzzy_used: List[Tuple[str, str]]  # requested, matched_name


def resolve_names(
    names: List[str],
    inv_by_norm: Dict[str, InvType],
    overrides: Dict[str, int],
    fuzzy: bool = True,
) -> ResolveReport:
    resolved: List[Tuple[str, int, int]] = []
    unresolved: List[str] = []
    fuzzy_used: List[Tuple[str, str]] = []
    all_keys = list(inv_by_norm.keys())

    for raw in names:
        key = normalize_name(raw)
        if key in overrides:
            tid = overrides[key]
            gid = 0
            for invv in inv_by_norm.values():
                if invv.type_id == tid:
                    gid = invv.group_id
                    break
            resolved.append((raw, tid, gid))
            continue
        if key in inv_by_norm:
            inv = inv_by_norm[key]
            resolved.append((raw, inv.type_id, inv.group_id))
            continue
        if fuzzy:
            matches = get_close_matches(key, all_keys, n=1, cutoff=0.82)
            if matches:
                inv = inv_by_norm[matches[0]]
                fuzzy_used.append((raw, inv.type_name))
                resolved.append((raw, inv.type_id, inv.group_id))
                continue
        unresolved.append(raw)

    return ResolveReport(resolved, unresolved, fuzzy_used)
