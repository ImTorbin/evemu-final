-- Backfill solarSystemID when an NPC is tied to a station but chr.solarSystemID was never set
-- (avoids solarsystemID=0 in agent location wrappers / client agentdialogueutil KeyError on cfg.solarsystems[0]).
-- +migrate Up
UPDATE chrNPCCharacters AS c
INNER JOIN staStations AS s ON s.stationID = c.stationID
SET c.solarSystemID = s.solarSystemID
WHERE c.solarSystemID = 0
  AND c.stationID != 0;

-- +migrate Down
-- No reliable inverse without storing prior values; data-only fix
