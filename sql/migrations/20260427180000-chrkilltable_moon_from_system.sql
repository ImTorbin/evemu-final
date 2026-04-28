-- Killmails stored moonID=0; Crucible client does cfg.evelocations.Get(moonID) → KeyError 0.
-- New kills use solarSystemID; backfill existing rows.
UPDATE chrKillTable SET moonID = solarSystemID WHERE moonID = 0 AND solarSystemID != 0;
