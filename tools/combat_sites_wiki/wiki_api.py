"""MediaWiki Action API client (read-only)."""

from __future__ import annotations

import json
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Any, Dict, Optional

DEFAULT_API = "https://wiki.eveuniversity.org/api.php"
DEFAULT_UA = "EVEmuCombatSiteCrawler/1.0 (educational; dungeon seed generator)"


def fetch_json(
    api_url: str,
    params: Dict[str, Any],
    user_agent: str = DEFAULT_UA,
    delay_s: float = 1.0,
) -> Dict[str, Any]:
    """GET request with query string; respects simple rate limit between calls."""
    time.sleep(delay_s)
    q = urllib.parse.urlencode(params)
    url = f"{api_url}?{q}"
    req = urllib.request.Request(url, headers={"User-Agent": user_agent})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            data = resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        retry = e.headers.get("Retry-After") if e.headers else None
        if retry:
            time.sleep(float(retry))
            return fetch_json(api_url, params, user_agent, delay_s=0)
        raise
    return json.loads(data)


def get_page_wikitext(
    title: str,
    api_url: str = DEFAULT_API,
    user_agent: str = DEFAULT_UA,
    delay_s: float = 1.0,
) -> Optional[str]:
    """Return main-slot wikitext for a page title, or None if missing."""
    params = {
        "action": "query",
        "format": "json",
        "titles": title,
        "prop": "revisions",
        "rvslots": "main",
        "rvprop": "content",
    }
    data = fetch_json(api_url, params, user_agent, delay_s)
    pages = data.get("query", {}).get("pages", {})
    for _pid, page in pages.items():
        if "missing" in page:
            return None
        revs = page.get("revisions") or []
        if not revs:
            return None
        slots = revs[0].get("slots", {})
        main = slots.get("main", {})
        return main.get("*")
    return None
