-- Juro finale (58373): restore ISK payout with the random Blood Raider BPC; use sysRange 15
-- (WithinThirteenJumpsOfAgent = 1–13 gate hops, not same system). Enum 13 is only 0–2 jumps.
--
-- +migrate Up
UPDATE qstCourier
SET
    rewardISK = 1500000,
    sysRange = 15
WHERE id = 58373 AND agentID = 90000007;

UPDATE agtOffers
SET rewardISK = 1500000
WHERE missionID = 58373 AND agentID = 90000007;

-- +migrate Down
UPDATE qstCourier
SET
    rewardISK = 0,
    sysRange = 13
WHERE id = 58373 AND agentID = 90000007;

UPDATE agtOffers
SET rewardISK = 0
WHERE missionID = 58373 AND agentID = 90000007;
