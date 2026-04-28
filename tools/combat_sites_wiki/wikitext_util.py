"""Wikitext helpers: sections, links, table rows."""

from __future__ import annotations

import re
from typing import Iterator, List, Optional, Tuple

LINK_RE = re.compile(r"\[\[([^|\]#]+)(?:\|([^\]]+))?\]\]")
HEADING_RE = re.compile(r"^(={2,6})\s*([^=]+?)\s*\1\s*$", re.MULTILINE)
DED_RATING_RE = re.compile(r"^(\d{1,2})/10$")


def iter_sections(wikitext: str) -> Iterator[Tuple[str, str]]:
    """Yield (heading_level2_or_empty, body) chunks."""
    matches = list(HEADING_RE.finditer(wikitext))
    if not matches:
        yield ("", wikitext)
        return
    if matches[0].start() > 0:
        yield ("", wikitext[: matches[0].start()])
    for i, m in enumerate(matches):
        title = m.group(2).strip()
        start = m.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(wikitext)
        yield (title, wikitext[start:end])


def extract_links(text: str) -> List[Tuple[str, str]]:
    """Return list of (target, display) for [[target|display]] / [[target]]."""
    out: List[Tuple[str, str]] = []
    for m in LINK_RE.finditer(text):
        target = m.group(1).strip().replace("_", " ")
        disp = (m.group(2) or m.group(1)).strip()
        out.append((target, disp))
    return out


def normalize_title(title: str) -> str:
    t = title.strip()
    # Namespace prefixes we skip for crawl
    skip_prefixes = (
        "File:",
        "Image:",
        "Category:",
        "Template:",
        "Help:",
        "User:",
        "Talk:",
        "EVE University:",
    )
    for p in skip_prefixes:
        if t.startswith(p):
            return ""
    return t


def split_wikitable_rows(section: str) -> List[str]:
    """Roughly split wikitext table into row strings (each still contains |cells|)."""
    rows: List[str] = []
    for line in section.splitlines():
        line = line.strip()
        if line.startswith("{|") or line.startswith("|}"):
            continue
        if line.startswith("|+"):
            continue
        if line.startswith("!") or line.startswith("|-"):
            if line.startswith("|-"):
                continue
        if line.startswith("|") or line.startswith("!"):
            rows.append(line)
    return rows


def row_cells(line: str) -> List[str]:
    """Split a table row line into cells (wikitext, not trimmed of templates)."""
    if line.startswith("!"):
        raw = line[1:]
    elif line.startswith("|"):
        raw = line[1:]
    else:
        raw = line
    parts = raw.split("|")
    return [p.strip() for p in parts]
