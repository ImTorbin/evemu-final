-- Juro (90000007): Crucible Station Services -> Agents only lists agents whose agtAgents.corporationID
-- matches the station owner (staStations.corporationID). BR employment was on agt only, so Juro vanished
-- from the list while sharing locationID with Imperial Shipment agents.
-- Keep chrNPCCharacters.corporationID as Blood Raiders; AgentDB joins crpNPCCorporations from chr when set.
--
-- +migrate Up
UPDATE agtAgents AS agt
INNER JOIN staStations AS s ON s.stationID = agt.locationID
SET agt.corporationID = s.corporationID
WHERE agt.agentID = 90000007;

-- +migrate Down
SET @juroBrCorp := (SELECT MIN(corporationID) FROM crpNPCCorporations WHERE factionID = 500012 LIMIT 1);
UPDATE agtAgents
SET corporationID = @juroBrCorp
WHERE agentID = 90000007 AND @juroBrCorp IS NOT NULL;
