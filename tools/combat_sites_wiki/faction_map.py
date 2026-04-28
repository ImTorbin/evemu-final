"""Map UniWiki faction labels to EVEmu factionID (EVE_Corp.h)."""

from __future__ import annotations

# Lowercased keys for lookup
FACTION_WIKI_TO_ID = {
    "angel cartel": 500011,
    "blood raiders": 500012,
    "blood raider covenant": 500012,
    "the blood raider covenant": 500012,
    "guristas pirates": 500010,
    "guristas": 500010,
    "sansha's nation": 500019,
    "sanshas nation": 500019,
    "serpentis corporation": 500020,
    "serpentis": 500020,
    "rogue drones": 500022,
    "rogue drone": 500022,
    "mordu's legion": 500018,
    "mordu s legion": 500018,
    "edencom": 500006,  # placeholder — may need real faction for Pochven content
    "triglavian collective": 500021,  # placeholder
}


def faction_id_from_wiki_name(name: str) -> int:
    key = name.strip().lower()
    key = key.replace("'", "'")
    return FACTION_WIKI_TO_ID.get(key, 0)


# Default frigate / light rat per faction for --ensure-all-dungeons (TQ typeIDs from SDE).
_FACTION_FALLBACK_TYPEID = {
    500011: 16879,  # Angel — Gistii Impaler
    500012: 16945,  # Blood — Corpii Raider
    500010: 16981,  # Guristas — Pithi Arrogator
    500019: 17063,  # Sansha's — Centii Enslaver
    500020: 17119,  # Serpentis — Coreli Defender
    500022: 3749,  # Rogue Drones — Decimator Alvi
    500018: 16040,  # Mordu's — Mordus Bounty Hunter
    500006: 54790,  # EDENCOM — EDENCOM Frigate (old)
    500021: 52645,  # Triglavian — Raznaborg Damavik
}


def fallback_typeid_for_faction(faction_id: int) -> int:
    """When a site page yields no invTypes matches, seed one believable rat of that faction."""
    if faction_id in _FACTION_FALLBACK_TYPEID:
        return _FACTION_FALLBACK_TYPEID[faction_id]
    return 16879
