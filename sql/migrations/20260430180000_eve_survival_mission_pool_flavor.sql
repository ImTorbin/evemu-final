-- Expand generic mission pools with titles from the EVE-Survival mission index
-- (https://eve-survival.org/?wakka=MissionReports). Rows are EVEmu courier/mining
-- templates (briefingID/item stacks are placeholders); server also slices global
-- pools per agent so offers differ between agents.
--
-- +migrate Up

-- Courier / trade-style (qstCourier): ids 73001–73028, agentID 0 = global pool
INSERT INTO qstCourier (
    id, briefingID, name, level, typeID, sysRange, important, storyline, raceID, chainIndex,
    itemTypeID, itemQty, rewardISK, rewardItemID, rewardItemQty, bonusISK, bonusTime, collateral, agentID
) VALUES
    (73001, 130400, 'Cargo Delivery', 1, 3, 3, 0, 0, 0, 0,  3721, 25,  95000, 0, 0, 32000, 90, 0, 0),
    (73002, 130400, 'Carrying AIMED''s', 1, 3, 2, 0, 0, 0, 0,  3705, 120, 88000, 0, 0, 28000, 90, 0, 0),
    (73003, 130400, 'Case of Kidnapping, A', 1, 3, 3, 0, 0, 0, 0,  3779, 15,  102000, 0, 0, 34000, 120, 0, 0),
    (73004, 130400, 'Clear the Trade Route', 1, 3, 2, 0, 0, 0, 0,  3689, 40,  91000, 0, 0, 30000, 90, 0, 0),
    (73005, 130400, 'Corporate Records', 1, 3, 2, 0, 0, 0, 0,  3721, 12,  87000, 0, 0, 29000, 90, 0, 0),
    (73006, 130400, 'Escaped Dissident', 1, 3, 3, 0, 0, 0, 0,  3771, 8,   99000, 0, 0, 33000, 120, 0, 0),
    (73007, 130400, 'Escaped Spy', 1, 3, 2, 0, 0, 0, 0,  3721, 18,  93000, 0, 0, 31000, 90, 0, 0),
    (73008, 130400, 'Forbidden Cloning', 1, 3, 4, 0, 0, 0, 0,  9834, 6,   110000, 0, 0, 36000, 120, 0, 0),
    (73009, 130400, 'Desperate Measures', 1, 3, 2, 0, 0, 0, 0,  3705, 90,  86000, 0, 0, 28000, 90, 0, 0),
    (73010, 130400, 'CONCORD Scout Spotted', 1, 3, 3, 0, 0, 0, 0,  3721, 30,  100000, 0, 0, 35000, 120, 0, 0),
    (73011, 130400, 'Commodity Run', 1, 4, 2, 0, 0, 0, 0,  3721, 35,  92000, 0, 0, 31000, 90, 0, 0),
    (73012, 130400, 'Artifact Recovery', 2, 3, 4, 0, 0, 0, 0,  3779, 28,  240000, 0, 0, 80000, 150, 0, 0),
    (73013, 130400, 'Dead Drift', 2, 3, 4, 0, 0, 0, 0,  3705, 220, 228000, 0, 0, 76000, 150, 0, 0),
    (73014, 130400, 'Fetching Friends', 2, 3, 3, 0, 0, 0, 0,  3721, 45,  235000, 0, 0, 78000, 150, 0, 0),
    (73015, 130400, 'Communications Cold War', 2, 3, 4, 0, 0, 0, 0,  3689, 55,  242000, 0, 0, 81000, 150, 0, 0),
    (73016, 130400, 'Customs Interdiction', 2, 3, 4, 0, 0, 0, 0,  3771, 24,  238000, 0, 0, 79000, 150, 0, 0),
    (73017, 130400, 'Data Mining', 2, 4, 3, 0, 0, 0, 0,  3721, 50,  230000, 0, 0, 77000, 150, 0, 0),
    (73018, 130400, 'Black Market Hub, The', 3, 3, 5, 0, 0, 0, 0,  9834, 20,  520000, 0, 0, 170000, 180, 0, 0),
    (73019, 130400, 'Covering Your Tracks', 3, 3, 6, 0, 0, 0, 0,  3705, 400, 510000, 0, 0, 165000, 180, 0, 0),
    (73020, 130400, 'Crystal Dreams Shattered', 3, 3, 5, 0, 0, 0, 0,  3779, 40,  530000, 0, 0, 175000, 180, 0, 0),
    (73021, 130400, 'Cut-Throat Competition', 3, 3, 5, 0, 0, 0, 0,  3721, 70,  515000, 0, 0, 168000, 180, 0, 0),
    (73022, 130400, 'Chain Reaction', 3, 4, 5, 0, 0, 0, 0,  3721, 60,  505000, 0, 0, 166000, 180, 0, 0),
    (73023, 130400, 'Diplomatic Incident', 4, 3, 8, 0, 0, 0, 0,  9834, 35,  1200000, 0, 0, 400000, 240, 0, 0),
    (73024, 130400, 'Federal Confidence', 4, 3, 8, 0, 0, 0, 0,  3705, 600, 1180000, 0, 0, 390000, 240, 0, 0),
    (73025, 130400, 'Feeding Frenzy', 4, 3, 8, 0, 0, 0, 0,  3779, 55,  1220000, 0, 0, 405000, 240, 0, 0),
    (73026, 130400, 'Enemies Abound (Courier Leg)', 4, 3, 8, 0, 0, 0, 0,  3771, 180, 1190000, 0, 0, 395000, 240, 0, 0),
    (73027, 130400, 'End to Eavesdropping, An', 4, 4, 7, 0, 0, 0, 0,  3721, 100, 1210000, 0, 0, 402000, 240, 0, 0),
    (73028, 130400, 'Exploited Sensitivities', 4, 3, 8, 0, 0, 0, 0,  3689, 90,  1175000, 0, 0, 388000, 240, 0, 0)
ON DUPLICATE KEY UPDATE
    name = VALUES(name),
    typeID = VALUES(typeID),
    sysRange = VALUES(sysRange),
    itemTypeID = VALUES(itemTypeID),
    itemQty = VALUES(itemQty),
    rewardISK = VALUES(rewardISK),
    bonusISK = VALUES(bonusISK),
    bonusTime = VALUES(bonusTime);

-- Mining names (qstMining): ids 81002–81012
INSERT INTO qstMining (
    id, briefingID, name, level, typeID, important, storyline,
    itemTypeID, itemQty, rewardISK, rewardItemID, rewardItemQty, bonusISK, bonusTime, sysRange, raceID
) VALUES
    (81002, 130400, 'Drone Distribution', 1, 5, 0, 0,  1228, 8000,  115000, 0, 0, 58000, 120, 2, 0),
    (81003, 130400, 'Ice Installation', 1, 5, 0, 0,  1230, 11000, 125000, 0, 0, 62000, 120, 2, 0),
    (81004, 130400, 'Like Drones to a Cloud', 1, 5, 0, 0,  1230, 9000,  118000, 0, 0, 59000, 120, 2, 0),
    (81005, 130400, 'Mercium Belt', 1, 5, 0, 0,  1224, 12000, 122000, 0, 0, 61000, 120, 2, 0),
    (81006, 130400, 'Mother Lode', 2, 5, 0, 0,  1228, 24000, 280000, 0, 0, 140000, 150, 3, 0),
    (81007, 130400, 'Persistent Pests', 2, 5, 0, 0,  1230, 22000, 275000, 0, 0, 138000, 150, 3, 0),
    (81008, 130400, 'Gas Injections', 2, 5, 0, 0,  1225, 20000, 290000, 0, 0, 145000, 150, 3, 0),
    (81009, 130400, 'Geodite and Gemology', 3, 5, 0, 0,  1226, 35000, 620000, 0, 0, 310000, 180, 4, 0),
    (81010, 130400, 'Bountiful Bandine', 3, 5, 0, 0,  1225, 38000, 615000, 0, 0, 308000, 180, 4, 0),
    (81011, 130400, 'Cheap Chills', 3, 5, 0, 0,  1227, 32000, 605000, 0, 0, 302000, 180, 4, 0),
    (81012, 130400, 'Mercium Experiments', 4, 5, 0, 0,  1228, 50000, 1350000, 0, 0, 675000, 240, 5, 0)
ON DUPLICATE KEY UPDATE
    name = VALUES(name),
    itemTypeID = VALUES(itemTypeID),
    itemQty = VALUES(itemQty),
    rewardISK = VALUES(rewardISK),
    bonusISK = VALUES(bonusISK),
    bonusTime = VALUES(bonusTime),
    sysRange = VALUES(sysRange);

-- +migrate Down
DELETE FROM qstCourier WHERE id BETWEEN 73001 AND 73028 AND agentID = 0;
DELETE FROM qstMining WHERE id BETWEEN 81002 AND 81012;
