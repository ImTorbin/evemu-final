-- Juro chain finale (58373): single pirate-faction BPC granted in AgentBound (random choice from server list).
-- Remove ISK / time bonus / item rows from template; LP is zeroed in code for this mission.
-- briefingID 132243 resolves to storyline "Balancing the Books (4 of 10)" in the client; use 130400 like steps 1 & 3.
--
-- +migrate Up
UPDATE qstCourier
SET
    briefingID = 130400,
    rewardISK = 0,
    rewardItemID = 0,
    rewardItemQty = 0,
    bonusISK = 0,
    bonusTime = 0
WHERE id = 58373 AND agentID = 90000007;

UPDATE agtOffers AS o
INNER JOIN qstCourier AS q ON q.id = o.missionID AND q.agentID = o.agentID
SET
    o.briefingID = q.briefingID,
    o.name = q.name,
    o.rewardISK = q.rewardISK,
    o.rewardItemID = q.rewardItemID,
    o.rewardItemQty = q.rewardItemQty,
    o.bonusISK = q.bonusISK,
    o.bonusTime = q.bonusTime,
    o.rewardLP = 0
WHERE o.missionID = 58373 AND o.agentID = 90000007;

-- +migrate Down
UPDATE qstCourier
SET
    briefingID = 132243,
    rewardISK = 1500000,
    rewardItemID = 0,
    rewardItemQty = 0,
    bonusISK = 2500000,
    bonusTime = 240
WHERE id = 58373 AND agentID = 90000007;

UPDATE agtOffers AS o
INNER JOIN qstCourier AS q ON q.id = o.missionID AND q.agentID = o.agentID
SET
    o.briefingID = q.briefingID,
    o.rewardISK = q.rewardISK,
    o.rewardItemID = q.rewardItemID,
    o.rewardItemQty = q.rewardItemQty,
    o.bonusISK = q.bonusISK,
    o.bonusTime = q.bonusTime
WHERE o.missionID = 58373 AND o.agentID = 90000007;
