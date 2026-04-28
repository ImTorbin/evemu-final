"""Extract NPC ship name mentions from a combat site child page wikitext."""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import List, Set

from . import wikitext_util as wu

# Quantity prefix e.g. "3x", "2-4x"
QTY_RE = re.compile(r"^\s*(\d+)\s*[x×]\s*", re.I)
# Templates common on UniWiki
SHIPLINK_RE = re.compile(r"\{\{\s*shiplink\s*\|[^|]*\|([^}|]+)", re.I)
SHIP_TYPE_RE = re.compile(r"\{\{\s*shipType\s*\|[^|]*\|([^}|]+)", re.I)
NPC_TABLE_ROW_RE = re.compile(r"\{\{\s*NPCTableRow\s*\|([\s\S]*?)\}\}", re.I)
BOLD_RE = re.compile(r"'''([^']+)'''")
HEADER_WAVE = re.compile(r"^=+\s*Wave\s+(\d+)", re.I)

_SKIP_NPC_ROW_ROLES = frozenset(
    {
        "structure",
        "structures",
        "rewards",
        "reward",
        "loot",
        "bounty",
        "mining",
        "asteroid",
        "asteroids",
    }
)


@dataclass
class ParsedSpawn:
    ship_name: str
    quantity: int = 1
    wave: int = 0
    source: str = ""  # wave|table|list


@dataclass
class ChildParseResult:
    wiki_title: str
    spawns: List[ParsedSpawn] = field(default_factory=list)
    raw_ship_names: List[str] = field(default_factory=list)


def _clean_name(s: str) -> str:
    s = SHIPLINK_RE.sub(r"\1", s)
    s = SHIP_TYPE_RE.sub(r"\1", s)
    s = wu.LINK_RE.sub(lambda m: m.group(2) or m.group(1), s)
    s = re.sub(r"\{\{[^}]+\}\}", "", s)
    s = s.replace("'''", "").strip()
    s = re.sub(r"\s+", " ", s)
    # Drop trailing parentheticals like (elite)
    s = re.sub(r"\s*\([^)]*\)\s*$", "", s)
    return s.strip()


def _is_likely_ship_name(name: str) -> bool:
    if len(name) < 3 or len(name) > 80:
        return False
    low = name.lower()
    skip = (
        "wave",
        "room",
        "pocket",
        "trigger",
        "loot",
        "bounty",
        "see also",
        "references",
        "faction",
        "damage",
        "resist",
        "strategy",
    )
    if any(low.startswith(x) for x in skip):
        return False
    if low in ("none", "n/a", "unknown", "?"):
        return False
    if "link=" in low or "22px" in low:
        return False
    return True


def _split_npctable_args(inner: str) -> List[str]:
    """Pipe segments before first named argument (``cargo=``, ``note=``, ``drop =``)."""
    out: List[str] = []
    for part in inner.split("|"):
        p = part.strip()
        if re.match(r"^[A-Za-z_][\w_]*\s*=", p):
            break
        out.append(p)
    return out


def _parse_qty_raw(qty_raw: str) -> int:
    s = qty_raw.strip()
    if not s:
        return 1
    m = re.match(r"^(\d+)\s*-\s*(\d+)", s)
    if m:
        return max(int(m.group(1)), int(m.group(2)))
    m = re.match(r"^(\d+)", s)
    if m:
        return int(m.group(1))
    return 1


def _prune_prefix_only_variants(names: List[str]) -> List[str]:
    """Drop ``Decimator`` when ``Decimator Alvi`` is also present (ellipsis slash rows)."""
    if len(names) < 2:
        return names
    out: List[str] = []
    for n in names:
        prefix = n + " "
        if any(o != n and o.startswith(prefix) for o in names):
            continue
        out.append(n)
    return out


def _expand_slash_ship_blob(blob: str) -> List[str]:
    """Expand UniWiki ``Name1/Name2`` and ``Gistii Ambusher/ Hunter`` style lists."""
    raw_chunks: List[str] = []
    for p in blob.split("/"):
        p = re.sub(r"^\s*\.\.\.\s*", "", p.strip())
        if not p or p == "...":
            continue
        raw_chunks.append(p)
    if not raw_chunks:
        return []

    joint_suffix = ""
    last = raw_chunks[-1]
    if " " in last:
        joint_suffix = " " + last.split(" ", 1)[1]

    out: List[str] = []
    for i, p in enumerate(raw_chunks):
        if " " in p:
            out.append(p.strip())
        elif joint_suffix:
            out.append((p + joint_suffix).strip())
        elif i == 0:
            out.append(p.strip())
        else:
            first = raw_chunks[0]
            if " " in first:
                race = first.split()[0]
                out.append(f"{race} {p}".strip())
            elif len(p) <= 2:
                continue
            else:
                out.append(f"{first} {p}".strip())
    return _prune_prefix_only_variants(out)


def _names_from_npctable_inner(inner: str) -> List[str]:
    segs = _split_npctable_args(inner)
    if len(segs) >= 3:
        role = segs[0].strip().lower()
        qty_raw = segs[1]
        name_blob = "|".join(segs[2:]).strip()
    elif len(segs) == 2:
        role = segs[0].strip().lower()
        qty_raw = ""
        name_blob = segs[1].strip()
    else:
        return []

    if role in _SKIP_NPC_ROW_ROLES:
        return []
    if not name_blob:
        return []

    _ = _parse_qty_raw(qty_raw)
    names: List[str] = []
    for expanded in _expand_slash_ship_blob(name_blob):
        cleaned = _clean_name(expanded)
        if _is_likely_ship_name(cleaned):
            names.append(cleaned)
    return names


def _names_from_npctable_templates(wikitext: str) -> List[str]:
    found: List[str] = []
    for m in NPC_TABLE_ROW_RE.finditer(wikitext):
        found.extend(_names_from_npctable_inner(m.group(1)))
    return found


def _names_from_ship_templates(wikitext: str) -> List[str]:
    found: List[str] = []
    for rx in (SHIPLINK_RE, SHIP_TYPE_RE):
        for m in rx.finditer(wikitext):
            name = _clean_name(m.group(1))
            if _is_likely_ship_name(name):
                found.append(name)
    return found


def parse_child_page(title: str, wikitext: str) -> ChildParseResult:
    """Heuristic extraction of ship names from a site article."""
    result = ChildParseResult(wiki_title=title)
    if not wikitext:
        return result

    current_wave = 0
    lines = wikitext.splitlines()

    for i, line in enumerate(lines):
        hm = HEADER_WAVE.match(line.strip())
        if hm:
            current_wave = int(hm.group(1))
            continue

        # Bullet lists
        stripped = line.strip()
        if stripped.startswith("*") or stripped.startswith("**"):
            text = stripped.lstrip("*").strip()
            qty = 1
            qm = QTY_RE.match(text)
            if qm:
                qty = int(qm.group(1))
                text = text[qm.end() :].strip()
            for bm in BOLD_RE.finditer(text):
                name = _clean_name(bm.group(1))
                if _is_likely_ship_name(name):
                    result.spawns.append(
                        ParsedSpawn(
                            ship_name=name,
                            quantity=qty,
                            wave=current_wave,
                            source="list",
                        )
                    )
            # Plain link in bullet
            if not BOLD_RE.search(text):
                for target, disp in wu.extract_links(text):
                    name = _clean_name(disp or target)
                    if _is_likely_ship_name(name):
                        result.spawns.append(
                            ParsedSpawn(
                                ship_name=name,
                                quantity=qty,
                                wave=current_wave,
                                source="list",
                            )
                        )

        # Table row — pick cells that look like ship names (contain link or bold)
        if stripped.startswith("|") and not stripped.startswith("|-"):
            cells = wu.row_cells(line)
            for cell in cells:
                if "dock" in cell.lower() or "gate" in cell.lower():
                    continue
                for target, disp in wu.extract_links(cell):
                    name = _clean_name(disp or target)
                    if _is_likely_ship_name(name):
                        result.spawns.append(
                            ParsedSpawn(
                                ship_name=name,
                                quantity=1,
                                wave=current_wave,
                                source="table",
                            )
                        )

    # UniWiki combat articles use {{NPCTableRow|...}} far more often than raw wikitables.
    for n in _names_from_npctable_templates(wikitext):
        result.spawns.append(ParsedSpawn(ship_name=n, quantity=1, wave=current_wave, source="NPCTableRow"))
    for n in _names_from_ship_templates(wikitext):
        result.spawns.append(ParsedSpawn(ship_name=n, quantity=1, wave=0, source="template"))

    # De-duplicate preserving order
    seen: Set[str] = set()
    uniq: List[ParsedSpawn] = []
    for s in result.spawns:
        k = s.ship_name.lower()
        if k in seen:
            continue
        seen.add(k)
        uniq.append(s)
    result.spawns = uniq
    result.raw_ship_names = [s.ship_name for s in uniq]
    return result
