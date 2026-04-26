-- Juro Darksynth (90000007): replace seven one-off couriers with a 4-step chain.
-- Steps 1-3: local (same system or up to 2 gates, sysRange=13) — ISK + bonus only.
-- Step 4: 8-15 gates to low-sec or null-sec (sysRange=14) — BPC kit granted in C++ (AgentBound) on complete.
-- +migrate Up
ALTER TABLE qstCourier
    ADD COLUMN chainIndex TINYINT UNSIGNED NOT NULL DEFAULT 0
        COMMENT 'Mission chain step; 0 = not chained. Juro: 1-4' AFTER raceID;

DELETE FROM qstCourier WHERE agentID = 90000007;

INSERT INTO qstCourier (
    id, briefingID, name, level, typeID, sysRange, important, storyline, raceID, chainIndex,
    itemTypeID, itemQty, rewardISK, rewardItemID, rewardItemQty, bonusISK, bonusTime, collateral, agentID
) VALUES
    (58370, 130400, 'Neighborhood Roster (1 of 4)', 1, 3, 13, 0, 0, 0, 1,  3721, 20,  400000, 0, 0,  500000, 240, 0, 90000007),
    (58371, 130997, 'Neighborhood Roster (2 of 4)', 1, 3, 13, 0, 0, 0, 2,  3705, 200, 400000, 0, 0,  500000, 240, 0, 90000007),
    (58372, 130400, 'Neighborhood Roster (3 of 4)', 1, 3, 13, 0, 0, 0, 3,  3721, 8,  400000, 0, 0,  500000, 240, 0, 90000007),
    (58373, 132243, 'Out-of-Band Shipment (4 of 4)', 1, 3, 14, 0, 0, 0, 4,  3771, 200, 1500000, 0, 0, 2500000, 240, 0, 90000007);

-- +migrate Down
DELETE FROM qstCourier WHERE id IN (58370, 58371, 58372, 58373) AND agentID = 90000007;
ALTER TABLE qstCourier DROP COLUMN chainIndex;
