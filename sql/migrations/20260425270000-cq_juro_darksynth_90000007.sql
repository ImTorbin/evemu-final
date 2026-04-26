-- Juro Darksynth — mission / CQ agent (character 90000007, station 60007117).
-- Requires prior migrations: shared_cq, staCQCustomAgents, cq_mission_agent_link (missionAgentID + excludedFromAgentSearch).
-- +migrate Up
SET @agent := 90000007;
SET @st := 60007117;

-- Display name + hide from People & Places agent *name* search
-- (agent name for AgentDB / search is `characterName`)
UPDATE chrNPCCharacters
SET
    characterName = 'Juro Darksynth',
    excludedFromAgentSearch = 1
WHERE characterID = @agent;

-- Station agent: location = station, corp = station owner (matches AgentDB)
INSERT INTO agtAgents (agentID, agentTypeID, divisionID, `level`, `quality`, corporationID, locationID, isLocator)
VALUES (
    @agent,
    1,
    2,
    1,
    0,
    (SELECT s.corporationID FROM staStations s WHERE s.stationID = @st LIMIT 1),
    @st,
    0
) ON DUPLICATE KEY UPDATE
    locationID = VALUES(locationID),
    corporationID = VALUES(corporationID);

-- Captain's Quarters spawn: protocol char = mission agent id; your logged placement
UPDATE staCQCustomAgents
SET
    missionAgentID = @agent,
    appearanceCharID = 0,
    posX = -0.352,
    posY = 0.248,
    posZ = 3.798,
    yaw = 0.826088,
    label = 'Juro Darksynth',
    updatedAt = UNIX_TIMESTAMP(CURRENT_TIMESTAMP)
WHERE stationID = @st
ORDER BY agentID ASC
LIMIT 1;

-- If you had no staCQCustomAgents row for this station yet, add this row once.
INSERT INTO staCQCustomAgents (stationID, missionAgentID, appearanceCharID, posX, posY, posZ, yaw, label, enabled, createdAt, updatedAt)
SELECT @st, @agent, 0, -0.352, 0.248, 3.798, 0.826088, 'Juro Darksynth', 1, UNIX_TIMESTAMP(CURRENT_TIMESTAMP), UNIX_TIMESTAMP(CURRENT_TIMESTAMP)
FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM staCQCustomAgents ca WHERE ca.stationID = @st);

-- +migrate Down
-- Revert is optional: clear mission link only; do not remove agt/ chr rows blindly.
UPDATE staCQCustomAgents SET missionAgentID = NULL WHERE stationID = 60007117 AND missionAgentID = 90000007;
UPDATE chrNPCCharacters SET excludedFromAgentSearch = 0 WHERE characterID = 90000007;
