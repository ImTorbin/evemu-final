"""Parse Combat_site hub wikitext into SiteRecord metadata."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Dict, List, Optional, Set, Tuple

from . import wikitext_util as wu
from .faction_map import faction_id_from_wiki_name

# Archetype IDs align with dunArchetypes / Dungeon::Type in EVEmu
ARCH_ANOMALY = 7
ARCH_UNRATED = 8
ARCH_ESCALATION = 9
ARCH_RATED = 10

ANOMALY_TABLE_RE = re.compile(
    r'^\{\|[^\n]*class="wikitable anomalies"[^\n]*\n([\s\S]*?)^\|\}\s*$',
    re.MULTILINE,
)


@dataclass
class SiteRecord:
    wiki_title: str
    display_name: str
    archetype_id: int
    faction_wiki: str
    faction_id: int
    ded_rating: Optional[str] = None  # e.g. "3/10"
    sec_note: str = ""
    source_section: str = ""


def _infer_faction_from_site_name(low: str) -> str:
    if low.startswith("angel "):
        return "Angel Cartel"
    if low.startswith("blood "):
        return "Blood Raiders"
    if low.startswith("guristas") or low.startswith("dread guristas"):
        return "Guristas Pirates"
    if low.startswith("sansha"):
        return "Sansha's Nation"
    if low.startswith("serpentis") or low.startswith("shadow serpentis"):
        return "Serpentis Corporation"
    if "drone" in low or low.startswith("infested"):
        return "Rogue Drones"
    if "edencom" in low:
        return "EDENCOM"
    if low.startswith("triglavian") or "conduit" in low or low.startswith("emerging "):
        return "Triglavian Collective"
    return ""


def _skip_anomaly_meta(low: str) -> bool:
    return low in (
        "exploration",
        "probe scanning",
        "hacking",
        "wormholes",
        "wormhole sites",
        "guide to combat sites",
        "dotlan",
        "high-sec",
        "low-sec",
        "nullsec",
        "rogue drone",
        "angel cartel",
        "blood raiders",
        "guristas pirates",
        "sansha's nation",
        "serpentis corporation",
        "rogue drones",
        "known space",
    ) or "combat_site" in low or low == "combat site"


def parse_hub(wikitext: str) -> List[SiteRecord]:
    """Extract site records from full Combat_site wikitext."""
    sections = {title.lower(): body for title, body in wu.iter_sections(wikitext)}

    records: Dict[Tuple[int, str], SiteRecord] = {}
    seen_pairs: Set[Tuple[int, str]] = set()

    def add(
        title: str,
        disp: str,
        arch: int,
        faction_wiki: str,
        ded: Optional[str] = None,
        sec_note: str = "",
        section: str = "",
    ) -> None:
        nt = wu.normalize_title(title)
        if not nt:
            return
        lk = (arch, nt.lower())
        if lk in seen_pairs:
            return
        seen_pairs.add(lk)
        fid = faction_id_from_wiki_name(faction_wiki)
        records[lk] = SiteRecord(
            wiki_title=nt,
            display_name=disp or nt,
            archetype_id=arch,
            faction_wiki=faction_wiki,
            faction_id=fid,
            ded_rating=ded,
            sec_note=sec_note,
            source_section=section,
        )

    # --- Anomalies: primary table has no == Anomalies == heading on UniWiki ---
    anom_body = sections.get("anomalies", "")
    m_anom = ANOMALY_TABLE_RE.search(wikitext)
    if m_anom:
        anom_body = anom_body + "\n" + m_anom.group(0)
    for target, disp in wu.extract_links(anom_body):
        nt = wu.normalize_title(target)
        if not nt:
            continue
        low = nt.lower()
        if _skip_anomaly_meta(low):
            continue
        disp_low = disp.lower() if disp else ""
        if re.match(r"^\d+/10$", disp_low.strip()):
            continue
        fw = _infer_faction_from_site_name(low)
        add(target, disp, ARCH_ANOMALY, fw, section="Anomalies")

    # Triglavian / EDENCOM anomaly grid (separate small table)
    tri_m = re.search(
        r"=== Triglavian Invasion Combat Sites ===\s*\n([\s\S]*?)=== Homefronts ===",
        wikitext,
    )
    if tri_m:
        for target, disp in wu.extract_links(tri_m.group(1)):
            nt = wu.normalize_title(target)
            if not nt:
                continue
            low = nt.lower()
            if low in ("triglavian", "edencom"):
                continue
            fw = _infer_faction_from_site_name(low)
            if not fw and "edencom" in low:
                fw = "EDENCOM"
            add(target, disp, ARCH_ANOMALY, fw, section="Triglavian/EDENCOM")

    # --- Unrated complexes table ---
    unr_m = re.search(
        r"=== Unrated complexes ===\s*\n([\s\S]*?)=== Blood Raider Temple Complex ===",
        wikitext,
    )
    if unr_m:
        unr_body = unr_m.group(1)
        header_factions: List[str] = []
        for line in unr_body.splitlines():
            line_st = line.strip()
            if not (line_st.startswith("|") or line_st.startswith("!")):
                continue
            cells = wu.row_cells(line)
            if not cells:
                continue
            joined = " ".join(cells).lower()
            if "unrated complexes" in joined and "angel cartel" in joined:
                header_factions = []
                for c in cells:
                    links = wu.extract_links(c)
                    if links:
                        header_factions.append(links[0][0].replace("_", " "))
                continue
            if not header_factions:
                continue
            for i, cell in enumerate(cells):
                if i < 3:
                    continue
                fac = header_factions[i] if i < len(header_factions) else ""
                for target, disp in wu.extract_links(cell):
                    add(target, disp, ARCH_UNRATED, fac, section="Unrated")

    # --- DED table inside Cosmic Signatures (or whole doc fallback) ---
    sig_body = sections.get("cosmic signatures", "") + "\n" + wikitext
    in_ded_table = False
    header_factions: List[str] = []

    for line in sig_body.splitlines():
        line_st = line.strip()
        if "ded rated complexes" in line_st.lower() and line_st.startswith("="):
            in_ded_table = True
            header_factions = []
            continue
        if in_ded_table and line_st.startswith("=") and "chemical" in line_st.lower():
            break
        if not (line_st.startswith("|") or line_st.startswith("!")):
            continue
        cells = wu.row_cells(line)
        if not cells:
            continue
        joined_hdr = " ".join(cells).lower()
        if "ded rating" in joined_hdr and "angel" in joined_hdr:
            header_factions = []
            for c in cells:
                links = wu.extract_links(c)
                if links:
                    header_factions.append(links[0][0].replace("_", " "))
                else:
                    txt = wu.LINK_RE.sub("", c).strip().lower()
                    header_factions.append(txt)
            continue
        c0_plain = wu.LINK_RE.sub("", cells[0]).strip()
        rm = re.match(r"^!?\s*(\d{1,2}/10)\s*$", c0_plain)
        if not rm:
            continue
        rating = rm.group(1)
        for i, cell in enumerate(cells):
            if i < 4:
                continue
            fac_wiki = (
                header_factions[i]
                if i < len(header_factions) and header_factions[i]
                else ""
            )
            for target, disp in wu.extract_links(cell):
                add(
                    target,
                    disp,
                    ARCH_RATED,
                    fac_wiki.replace("_", " "),
                    ded=rating,
                    section="DED",
                )

    # --- Chemical labs table ---
    chem_body = sections.get("cosmic signatures", "")
    if "chemical labs" in chem_body.lower():
        capture = False
        for line in chem_body.splitlines():
            if "chemical labs" in line.lower() and line.startswith("="):
                capture = True
                continue
            if capture and line.startswith("=") and "combat relic" in line.lower():
                break
            if not capture or not (line.strip().startswith("|")):
                continue
            cells = wu.row_cells(line)
            if len(cells) < 2:
                continue
            fac_cell, site_cell = cells[0], cells[1]
            flinks = wu.extract_links(fac_cell)
            faction_wiki = flinks[0][0] if flinks else wu.LINK_RE.sub("", fac_cell)
            faction_wiki = faction_wiki.strip()
            for target, disp in wu.extract_links(site_cell):
                tnorm = target.strip()
                if not wu.normalize_title(tnorm):
                    continue
                add(target.strip(), disp.strip(), ARCH_UNRATED, faction_wiki, section="Chemical lab")

    # --- Combat relic sites (unrated signatures) ---
    relic_m = re.search(
        r"=== Combat relic sites ===\s*\n([\s\S]*?)=== ISHAEKA",
        wikitext,
    )
    if relic_m:
        for target, disp in wu.extract_links(relic_m.group(1)):
            add(target, disp, ARCH_UNRATED, "Rogue Drones", section="Combat relic")

    return list(records.values())
