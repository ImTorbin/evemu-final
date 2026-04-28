"""ESI (EVE Swagger Interface) — universe type lookups for Tranquility cross-checks."""

from __future__ import annotations

import json
import os
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from .typeid_resolve import InvType, normalize_name

DEFAULT_ESI_BASE = "https://esi.evetech.net/latest"
DEFAULT_UA = "EVEmuCombatSites/1.0 (UniWiki combat site seed generator)"


def _env_token() -> Optional[str]:
    t = os.environ.get("EVE_ESI_ACCESS_TOKEN", "").strip()
    return t or None


def fetch_universe_type(
    type_id: int,
    *,
    base_url: str = DEFAULT_ESI_BASE,
    user_agent: str = DEFAULT_UA,
    access_token: Optional[str] = None,
    delay_s: float = 0.2,
    cache_dir: Optional[Path] = None,
) -> Optional[Dict[str, Any]]:
    """
    GET /universe/types/{type_id}/ (public; no OAuth required).
    Returns parsed JSON, or None if the type is unknown on Tranquility (404).
    """
    token = access_token or _env_token()

    if cache_dir:
        cache_dir.mkdir(parents=True, exist_ok=True)
        cache_path = cache_dir / f"type_{type_id}.json"
        if cache_path.is_file():
            return json.loads(cache_path.read_text(encoding="utf-8"))

    if delay_s > 0:
        time.sleep(delay_s)

    url = f"{base_url.rstrip('/')}/universe/types/{int(type_id)}/"
    headers = {"User-Agent": user_agent, "Accept": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"

    req = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=45) as resp:
            raw = resp.read().decode("utf-8", errors="replace")
            data = json.loads(raw)
    except urllib.error.HTTPError as e:
        ra = e.headers.get("Retry-After") if e.headers else None
        if ra and e.code in (420, 429, 503):
            time.sleep(float(ra))
            return fetch_universe_type(
                type_id,
                base_url=base_url,
                user_agent=user_agent,
                access_token=token,
                delay_s=0,
                cache_dir=cache_dir,
            )
        if e.code == 404:
            return None
        raise
    except urllib.error.URLError:
        raise

    if cache_dir:
        (cache_dir / f"type_{type_id}.json").write_text(
            json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8"
        )
    return data


def verify_type_ids_against_esi(
    type_ids: List[int],
    *,
    base_url: str = DEFAULT_ESI_BASE,
    user_agent: str = DEFAULT_UA,
    access_token: Optional[str] = None,
    delay_s: float = 0.2,
    cache_dir: Optional[Path] = None,
) -> List[Dict[str, Any]]:
    """
    Fetch each unique type_id from ESI once (with cache). Returns rows:
    { type_id, esi_name, esi_group_id, ok_esi } ok_esi False if 404/error.
    """
    seen: set[int] = set()
    out: List[Dict[str, Any]] = []
    for tid in type_ids:
        if tid in seen:
            continue
        seen.add(tid)
        try:
            payload = fetch_universe_type(
                tid,
                base_url=base_url,
                user_agent=user_agent,
                access_token=access_token,
                delay_s=delay_s,
                cache_dir=cache_dir,
            )
        except Exception as e:  # noqa: BLE001 — surface as row
            out.append(
                {
                    "type_id": tid,
                    "esi_name": None,
                    "esi_group_id": None,
                    "ok_esi": False,
                    "esi_error": str(e),
                }
            )
            continue
        if not payload:
            out.append(
                {
                    "type_id": tid,
                    "esi_name": None,
                    "esi_group_id": None,
                    "ok_esi": False,
                    "esi_error": "404 or empty",
                }
            )
            continue
        out.append(
            {
                "type_id": tid,
                "esi_name": payload.get("name"),
                "esi_group_id": payload.get("group_id"),
                "ok_esi": True,
                "esi_error": None,
            }
        )
    return out


def esi_compare_to_invtypes(
    type_ids: List[int],
    inv_by_tid: Dict[int, InvType],
    *,
    base_url: str = DEFAULT_ESI_BASE,
    user_agent: str = DEFAULT_UA,
    access_token: Optional[str] = None,
    delay_s: float = 0.2,
    cache_dir: Optional[Path] = None,
) -> Tuple[List[Dict[str, Any]], List[str]]:
    """
    For each unique type_id, fetch ESI and compare name to local invTypes (if present).
    Returns (detail_rows, short_report_lines) — report lines only list problems.
    """
    uniq = sorted({int(t) for t in type_ids})
    esi_rows = verify_type_ids_against_esi(
        uniq,
        base_url=base_url,
        user_agent=user_agent,
        access_token=access_token,
        delay_s=delay_s,
        cache_dir=cache_dir,
    )
    by_tid: Dict[int, Dict[str, Any]] = {int(r["type_id"]): r for r in esi_rows}
    details: List[Dict[str, Any]] = []
    report_lines: List[str] = []

    for tid in uniq:
        er = by_tid.get(tid, {})
        inv = inv_by_tid.get(tid)
        inv_name = inv.type_name if inv else None
        esi_name = er.get("esi_name")
        ok_esi = bool(er.get("ok_esi"))

        status = "ok"
        if not ok_esi:
            status = "esi_error"
            report_lines.append(
                f"  ESI: typeID {tid} not on Tranquility ({er.get('esi_error', 'unknown')})"
            )
        elif inv_name and esi_name and normalize_name(inv_name) != normalize_name(esi_name):
            status = "name_mismatch"
            report_lines.append(
                f"  ESI: typeID {tid} invTypes {inv_name!r} vs ESI {esi_name!r}"
            )

        details.append(
            {
                "type_id": tid,
                "invTypes_name": inv_name,
                "esi_name": esi_name,
                "esi_group_id": er.get("esi_group_id"),
                "status": status,
            }
        )

    return details, report_lines
