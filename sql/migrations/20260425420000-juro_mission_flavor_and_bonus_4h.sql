-- Juro Darksynth (90000007): smuggling courier flavor names, client-safe briefing message IDs,
-- 4h time bonus, 800k/1M ISK, faction/pirate BPC rewards.
-- Briefing *body* is still resolved on the client from `briefingID` (EVE string table). We use IDs
-- that are widely present in Crucible data; 130997 was known to show [no messageID: 130997] for some clients.
-- +migrate Up
UPDATE qstCourier SET
    name = 'Unscheduled Displacement',
    briefingID = 130400,
    rewardISK = 800000,
    rewardItemID = 17931,
    rewardItemQty = 1,
    bonusISK = 1000000,
    bonusTime = 240
WHERE id = 58359 AND agentID = 90000007;

UPDATE qstCourier SET
    name = 'Grey-Label Supply',
    briefingID = 131000,
    rewardISK = 800000,
    rewardItemID = 17716,
    rewardItemQty = 1,
    bonusISK = 1000000,
    bonusTime = 240
WHERE id = 58360 AND agentID = 90000007;

UPDATE qstCourier SET
    name = 'Cultural Reassignment',
    briefingID = 131498,
    rewardISK = 800000,
    rewardItemID = 17933,
    rewardItemQty = 1,
    bonusISK = 1000000,
    bonusTime = 240
WHERE id = 58361 AND agentID = 90000007;

UPDATE qstCourier SET
    name = 'Biomass on the Move',
    briefingID = 131525,
    rewardISK = 800000,
    rewardItemID = 17721,
    rewardItemQty = 1,
    bonusISK = 1000000,
    bonusTime = 240
WHERE id = 58362 AND agentID = 90000007;

UPDATE qstCourier SET
    name = 'Street-Grade Haulage',
    briefingID = 132243,
    rewardISK = 800000,
    rewardItemID = 17719,
    rewardItemQty = 1,
    bonusISK = 1000000,
    bonusTime = 240
WHERE id = 58363 AND agentID = 90000007;

UPDATE qstCourier SET
    name = 'Blind-Stamp Cargo',
    briefingID = 143443,
    rewardISK = 800000,
    rewardItemID = 17925,
    rewardItemQty = 1,
    bonusISK = 1000000,
    bonusTime = 240
WHERE id = 58364 AND agentID = 90000007;

UPDATE qstCourier SET
    name = 'Hardened Legality',
    briefingID = 143755,
    rewardISK = 800000,
    rewardItemID = 3840,
    rewardItemQty = 1,
    bonusISK = 1000000,
    bonusTime = 240
WHERE id = 58365 AND agentID = 90000007;

-- +migrate Down
-- (optional re-apply 20260425400000 snapshot by hand if needed)
