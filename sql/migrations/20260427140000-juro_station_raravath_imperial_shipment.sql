-- Juro (90000007): Station Services match *exact* stationID; Agent Finder uses solar system, so
-- you see "0 jumps" for Juro at Imperial Shipment while he was still on seed station 60007117.
-- This points agt + chr to Raravath III - Imperial Shipment Storage, moves CQ custom row, and
-- re-applies Blood Raider corp (station owner is Amarr, so the sheet showed RIN without this).
-- +migrate Up

SET @juro := 90000007;
-- Prefer "Raravath III" + Imperial Shipment (player dock in screenshots); fall back to any Raravath* + Imperial*Shipment* in the DB.
SET @st := (
  SELECT s.stationID
  FROM staStations s
  WHERE s.stationName LIKE '%Raravath III%Imperial%Shipment%'
  LIMIT 1
);
SET @st := IFNULL(
  @st,
  (SELECT s.stationID
   FROM staStations s
   WHERE s.stationName LIKE '%Raravath%'
     AND s.stationName LIKE '%Imperial%Shipment%'
   LIMIT 1)
);
SET @juroBrCorp := (SELECT MIN(corporationID) FROM crpNPCCorporations WHERE factionID = 500012 LIMIT 1);

UPDATE agtAgents a
  INNER JOIN staStations s ON s.stationID = @st
SET
  a.locationID = s.stationID,
  a.corporationID = IFNULL(@juroBrCorp, s.corporationID)
WHERE
  a.agentID = @juro
  AND @st IS NOT NULL;

UPDATE chrNPCCharacters c
  INNER JOIN staStations s ON s.stationID = @st
SET
  c.stationID = s.stationID,
  c.solarSystemID = s.solarSystemID,
  c.corporationID = IFNULL(@juroBrCorp, c.corporationID)
WHERE
  c.characterID = @juro
  AND @st IS NOT NULL;

-- Show in Services -> Agents; safe if 27120000 already applied
UPDATE chrNPCCharacters
SET excludedFromAgentSearch = 0
WHERE characterID = @juro;

UPDATE staCQCustomAgents
SET stationID = @st
WHERE missionAgentID = @juro
  AND @st IS NOT NULL;

-- +migrate Down
SET @juro2 := 90000007;
SET @oldst := 60007117;
UPDATE agtAgents a
  INNER JOIN staStations s ON s.stationID = @oldst
SET
  a.locationID = @oldst,
  a.corporationID = s.corporationID
WHERE
  a.agentID = @juro2
  AND @oldst IS NOT NULL;
UPDATE chrNPCCharacters c
  INNER JOIN staStations s ON s.stationID = @oldst
SET
  c.stationID = @oldst,
  c.solarSystemID = s.solarSystemID,
  c.corporationID = s.corporationID
WHERE
  c.characterID = @juro2
  AND @oldst IS NOT NULL;
UPDATE staCQCustomAgents
SET stationID = @oldst
WHERE missionAgentID = @juro2
  AND @oldst IS NOT NULL;
