-- Juro Darksynth (90000007): use Blood Raider faction (user factionID 500012) via a matching NPC corporation.
-- Agent faction in-game comes from crpNPCCorporations.factionID joined on agtAgents.corporationID (see AgentDB.cpp).
-- +migrate Up
SET @juroBrCorp := (SELECT MIN(corporationID) FROM crpNPCCorporations WHERE factionID = 500012 LIMIT 1);

UPDATE agtAgents
SET corporationID = @juroBrCorp
WHERE agentID = 90000007 AND @juroBrCorp IS NOT NULL;

UPDATE chrNPCCharacters
SET corporationID = @juroBrCorp
WHERE characterID = 90000007 AND @juroBrCorp IS NOT NULL;

-- +migrate Down
-- Restore to station owner (same as staCQ / original Juro seed).
UPDATE agtAgents AS a
INNER JOIN staStations s ON s.stationID = a.locationID
SET a.corporationID = s.corporationID
WHERE a.agentID = 90000007;

UPDATE chrNPCCharacters AS c
INNER JOIN staStations s ON s.stationID = c.stationID
SET c.corporationID = s.corporationID
WHERE c.characterID = 90000007;
