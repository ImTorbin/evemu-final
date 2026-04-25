-- Verify and seed the `locationScenes` table that ServiceDB::GetSceneIDForStation
-- queries during CQ scene resolution.
--
-- The table is normally part of the static EVE data dump. If the table is
-- missing, GetSceneIDForStation always returns 0 and the client falls back
-- to a black-screen / unresolved scene. This helper (a) ensures the table
-- exists with the columns the server expects, and (b) lets you seed a row
-- for your dev station.
--
-- Run this against the EVEmu main database (NOT the static one) only if
-- you confirmed the static dump is missing the table. Otherwise just use
-- the verification SELECT at the bottom and INSERT into the existing table.

-- 1. Ensure the table exists.
CREATE TABLE IF NOT EXISTS locationScenes (
    locationID INT(10) NOT NULL,
    sceneID    INT(10) NOT NULL,
    PRIMARY KEY (locationID),
    KEY idx_locationScenes_sceneID (sceneID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- 2. Verify which of your CQ-eligible stations currently have a row.
--    Replace the IN (...) list with the station IDs you actually use.
SELECT
    st.stationID,
    st.solarSystemID,
    lsStation.sceneID    AS stationScene,
    lsSystem.sceneID     AS systemScene,
    COALESCE(lsStation.sceneID, lsSystem.sceneID, 0) AS resolvedScene
FROM staStations st
LEFT JOIN locationScenes lsStation ON lsStation.locationID = st.stationID
LEFT JOIN locationScenes lsSystem  ON lsSystem.locationID  = st.solarSystemID
WHERE st.stationID IN (60006340, 60014629, 60003760)
ORDER BY st.stationID;

-- 3. Example seed: register a station-specific scene.
--    The sceneID values used by the Crucible client are recorded in the
--    decompiled `cqScenes`/`captainsQuartersRoom` modules; pull them out of
--    the decompiled tree once Phase 1 finishes. Until then, leave this
--    block commented and rely on existing static data.
--
-- INSERT INTO locationScenes (locationID, sceneID) VALUES
--     (60006340, 9999991),  -- replace 9999991 with the real CQ sceneID
--     (60014629, 9999992)
-- ON DUPLICATE KEY UPDATE sceneID = VALUES(sceneID);

-- 4. After seeding, restart the EVEmu server (or invalidate ObjCacheDB)
--    so Generate_Locationscenes refreshes the cached config.BulkData blob.
