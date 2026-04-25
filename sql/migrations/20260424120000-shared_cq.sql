-- Shared multiplayer Captain's Quarters persistence
-- +migrate Up
CREATE TABLE staCQInstances (
    worldSpaceID INT(10) NOT NULL AUTO_INCREMENT,
    stationID INT(10) NOT NULL,
    worldSpaceTypeID INT(10) NOT NULL DEFAULT 1,
    isShared TINYINT(1) NOT NULL DEFAULT 1,
    maxOccupants INT(10) NOT NULL DEFAULT 64,
    state TINYINT(4) NOT NULL DEFAULT 1,
    createdAt BIGINT(20) NOT NULL,
    lastActiveAt BIGINT(20) NOT NULL,
    PRIMARY KEY (worldSpaceID),
    KEY idx_staCQInstances_stationID (stationID)
);

CREATE TABLE staCQOccupancy (
    worldSpaceID INT(10) NOT NULL,
    characterID INT(10) NOT NULL,
    occupancyState TINYINT(4) NOT NULL DEFAULT 1,
    joinedAt BIGINT(20) NOT NULL,
    lastSeenAt BIGINT(20) NOT NULL,
    PRIMARY KEY (worldSpaceID, characterID),
    KEY idx_staCQOccupancy_characterID (characterID)
);

-- +migrate Down
DROP TABLE staCQOccupancy;
DROP TABLE staCQInstances;
