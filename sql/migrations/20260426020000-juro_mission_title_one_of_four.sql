-- Juro: client shows wrong chain title (e.g. "Balancing the Books (4 of 10)") when briefingID 131000
-- matches Sisters of EVE / career string tables. Use explicit "(1 of 4)" names and non-131000 briefings.
-- +migrate Up
UPDATE qstCourier
SET
    name = 'Neighborhood Roster (1 of 4)',
    briefingID = 130400
WHERE id = 58370 AND agentID = 90000007;

UPDATE qstCourier
SET
    name = 'Neighborhood Roster (2 of 4)',
    briefingID = 130997
WHERE id = 58371 AND agentID = 90000007;

-- Step 3: avoid 131000 (resolves to SoE "Balancing the Books" in many client builds)
UPDATE qstCourier
SET
    name = 'Neighborhood Roster (3 of 4)',
    briefingID = 130400
WHERE id = 58372 AND agentID = 90000007;

UPDATE qstCourier
SET
    name = 'Out-of-Band Shipment (4 of 4)',
    briefingID = 132243
WHERE id = 58373 AND agentID = 90000007;

-- Refresh open/accepted offers so the convo title matches (server reads from agtOffers).
UPDATE agtOffers AS o
INNER JOIN qstCourier AS q ON q.id = o.missionID AND q.agentID = 90000007
SET
    o.name = q.name,
    o.briefingID = q.briefingID
WHERE o.agentID = 90000007;

-- +migrate Down
UPDATE qstCourier
SET
    name = 'Neighborhood Roster 1/4',
    briefingID = 130400
WHERE id = 58370 AND agentID = 90000007;
UPDATE qstCourier
SET
    name = 'Neighborhood Roster 2/4',
    briefingID = 130997
WHERE id = 58371 AND agentID = 90000007;
UPDATE qstCourier
SET
    name = 'Neighborhood Roster 3/4',
    briefingID = 131000
WHERE id = 58372 AND agentID = 90000007;
UPDATE qstCourier
SET
    name = 'Out-of-Band Shipment 4/4',
    briefingID = 132243
WHERE id = 58373 AND agentID = 90000007;

UPDATE agtOffers AS o
INNER JOIN qstCourier AS q ON q.id = o.missionID AND q.agentID = 90000007
SET
    o.name = q.name,
    o.briefingID = q.briefingID
WHERE o.agentID = 90000007;
