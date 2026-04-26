-- Station CQ custom "NPC" avatars: fixed positions + template character paper doll
--
-- Instance id seen by the client: (0xC0000000 + agentID), e.g. agentID=1 -> 3221225473.
-- Add rows manually, or from in-game: captainsQuartersSvc.CQDebugRegisterAgentHere(appearanceCharID, 'label')
-- (requires account role GMH|QA|PROGRAMMER).
-- Example (replace station / coords / template char id):
--   INSERT INTO staCQCustomAgents (stationID, appearanceCharID, posX, posY, posZ, yaw, label, enabled, createdAt, updatedAt)
--   VALUES (60004420, 90000001, 10.0, 2.0, -1.0, 0.0, 'dealer1', 1, UNIX_TIMESTAMP(CURRENT_TIMESTAMP), UNIX_TIMESTAMP(CURRENT_TIMESTAMP));
--
-- +migrate Up
CREATE TABLE IF NOT EXISTS staCQCustomAgents (
    agentID             INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
    stationID            INT(10) UNSIGNED NOT NULL,
    -- instanceCharID sent to clients is always (0xC0000000 + agentID) — not stored here.
    appearanceCharID     INT(10) UNSIGNED NOT NULL,
    posX                 DOUBLE NOT NULL DEFAULT 0,
    posY                 DOUBLE NOT NULL DEFAULT 0,
    posZ                 DOUBLE NOT NULL DEFAULT 0,
    yaw                  FLOAT NOT NULL DEFAULT 0,
    label                VARCHAR(64) NOT NULL DEFAULT '',
    enabled              TINYINT(1) NOT NULL DEFAULT 1,
    createdAt            BIGINT(20) NOT NULL,
    updatedAt            BIGINT(20) NOT NULL,
    PRIMARY KEY (agentID),
    KEY idx_staCQCustomAgents_station (stationID, enabled)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- +migrate Down
DROP TABLE IF EXISTS staCQCustomAgents;
