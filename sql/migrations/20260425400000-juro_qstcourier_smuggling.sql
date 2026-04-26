-- Juro Darksynth (90000007): smuggling couriers (slaves, narcotics, contraband) with ISK + time bonus + faction/pirate BPC rewards.
-- Requires qstCourier.agentID (per-agent template pool) from application of this migration in order.
-- +migrate Up
ALTER TABLE qstCourier
    ADD COLUMN agentID INT(10) UNSIGNED NOT NULL DEFAULT 0
        COMMENT '0 = any agent; else only this agtAgents.agentID' AFTER collateral;

INSERT INTO qstCourier (
    id, briefingID, name, level, typeID, sysRange, important, storyline, raceID,
    itemTypeID, itemQty, rewardISK, rewardItemID, rewardItemQty, bonusISK, bonusTime, collateral, agentID
) VALUES
    (58359, 130400, 'Unscheduled Personnel Transfer', 1, 3, 1, 0, 0, 0, 3721, 20, 800000, 17931, 1, 1000000, 45, 0, 90000007),
    (58360, 130997, 'Pharmaceutical Samples', 1, 3, 1, 0, 0, 0, 3705, 500, 800000, 17716, 1, 1000000, 45, 0, 90000007),
    (58361, 131000, 'Cultural Exports', 1, 3, 1, 0, 0, 0, 3721, 12, 800000, 17933, 1, 1000000, 45, 0, 90000007),
    (58362, 131498, 'Grey-Market Biomass', 1, 3, 1, 0, 0, 0, 3771, 300, 800000, 17721, 1, 1000000, 45, 0, 90000007),
    (58363, 131525, 'Street Supply Run', 1, 3, 1, 0, 0, 0, 3705, 250, 800000, 17719, 1, 1000000, 45, 0, 90000007),
    (58364, 132243, 'Off-the-Books Haul', 1, 3, 1, 0, 0, 0, 3721, 6, 800000, 17925, 1, 1000000, 45, 0, 90000007),
    (58365, 143755, 'Reinforced Delivery', 1, 3, 1, 0, 0, 0, 3721, 3, 800000, 3840, 1, 1000000, 45, 0, 90000007);

-- +migrate Down
DELETE FROM qstCourier WHERE agentID = 90000007;
ALTER TABLE qstCourier DROP COLUMN agentID;
