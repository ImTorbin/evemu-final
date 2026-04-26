-- Keep chrNPCCharacters.stationID in sync with agtAgents.locationID so the station
-- "Agents" tab (and GetAgents) shows Juro when docked at his station.
-- +migrate Up
UPDATE chrNPCCharacters AS c
INNER JOIN agtAgents AS a ON a.agentID = c.characterID
SET c.stationID = a.locationID
WHERE a.agentID = 90000007 AND a.locationID IS NOT NULL AND a.locationID > 0;

-- +migrate Down
-- (no reliable inverse if station was hand-edited)
