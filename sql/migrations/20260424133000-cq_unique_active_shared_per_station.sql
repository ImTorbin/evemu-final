-- Enforce one active shared CQ worldspace per station.
--
-- +migrate Up
-- Keep oldest active shared row per station, deactivate newer duplicates.
UPDATE staCQInstances newer
JOIN staCQInstances older
  ON newer.stationID = older.stationID
 AND newer.isShared = 1
 AND older.isShared = 1
 AND newer.state = 1
 AND older.state = 1
 AND newer.worldSpaceID > older.worldSpaceID
SET newer.state = 0;

-- Generated key is 1 only for active shared rows, NULL otherwise.
-- Unique(stationID, activeSharedKey) then enforces at most one active shared row per station.
ALTER TABLE staCQInstances
  ADD COLUMN activeSharedKey TINYINT(1)
    GENERATED ALWAYS AS (CASE WHEN isShared = 1 AND state = 1 THEN 1 ELSE NULL END) STORED,
  ADD UNIQUE KEY uq_staCQ_station_active_shared (stationID, activeSharedKey);

-- +migrate Down
ALTER TABLE staCQInstances
  DROP INDEX uq_staCQ_station_active_shared,
  DROP COLUMN activeSharedKey;
