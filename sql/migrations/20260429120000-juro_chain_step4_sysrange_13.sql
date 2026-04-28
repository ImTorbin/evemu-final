-- Juro chain step 4 (58373): sysRange 14 (distant low-sec) often yielded no dropoff on thin map data,
-- leaving destinationID 0 and breaking the client mission window (Pathfinder KeyError). Use the same
-- local band as steps 1–3 (13 = WithinTwoJumpsOfAgent) so dropoff is always reachable.
--
-- +migrate Up
UPDATE qstCourier
SET sysRange = 13
WHERE id = 58373 AND agentID = 90000007;

-- +migrate Down
UPDATE qstCourier
SET sysRange = 14
WHERE id = 58373 AND agentID = 90000007;
