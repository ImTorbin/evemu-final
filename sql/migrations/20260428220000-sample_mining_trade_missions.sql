-- Sample EVEmu mission templates: mining (qstMining) and trade (qstCourier typeID=4).
-- Requires static tables from the EVE dump. Adjust IDs if they conflict with your DB.
--
-- +migrate Up
INSERT INTO qstMining (
    id, briefingID, name, level, typeID, important, storyline,
    itemTypeID, itemQty, rewardISK, rewardItemID, rewardItemQty, bonusISK, bonusTime, sysRange, raceID
) VALUES (
    80001, 130400, 'Veldspar Survey (EVEmu)', 1, 5, 0, 0,
    1230, 10000, 120000, 0, 0, 60000, 120, 2, 0
) ON DUPLICATE KEY UPDATE
    name = VALUES(name),
    itemTypeID = VALUES(itemTypeID),
    itemQty = VALUES(itemQty),
    rewardISK = VALUES(rewardISK),
    bonusISK = VALUES(bonusISK),
    bonusTime = VALUES(bonusTime),
    sysRange = VALUES(sysRange);

INSERT INTO qstCourier (
    id, briefingID, name, level, typeID, sysRange, important, storyline, raceID, chainIndex,
    itemTypeID, itemQty, rewardISK, rewardItemID, rewardItemQty, bonusISK, bonusTime, collateral, agentID
) VALUES (
    70001, 130400, 'Commodity Run (EVEmu)', 1, 4, 2, 0, 0, 0, 0,
    3721, 40, 90000, 0, 0, 30000, 90, 0, 0
) ON DUPLICATE KEY UPDATE
    name = VALUES(name),
    typeID = VALUES(typeID),
    itemTypeID = VALUES(itemTypeID),
    itemQty = VALUES(itemQty),
    rewardISK = VALUES(rewardISK),
    bonusISK = VALUES(bonusISK),
    bonusTime = VALUES(bonusTime);

-- +migrate Down
DELETE FROM qstMining WHERE id = 80001;
DELETE FROM qstCourier WHERE id = 70001 AND agentID = 0;
