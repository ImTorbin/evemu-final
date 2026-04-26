-- Juro Darksynth must exist in chrNPCCharacters for paper doll + GetPublicInfo on the client.
-- 20260425270000 only UPDATEd; if the row was missing, that was a no-op and the CQ spawn fails.
-- +migrate Up
INSERT INTO chrNPCCharacters (
    characterID,
    characterName,
    description,
    corporationID,
    stationID,
    typeID,
    ancestryID,
    gender,
    excludedFromAgentSearch
)
SELECT
    90000007,
    'Juro Darksynth',
    '',
    s.corporationID,
    60007117,
    0,
    0,
    0,
    1
FROM staStations s
WHERE s.stationID = 60007117
LIMIT 1
ON DUPLICATE KEY UPDATE
    characterName = 'Juro Darksynth',
    corporationID = VALUES(corporationID),
    stationID = 60007117,
    excludedFromAgentSearch = 1;

-- +migrate Down
-- DELETE FROM chrNPCCharacters WHERE characterID = 90000007;
