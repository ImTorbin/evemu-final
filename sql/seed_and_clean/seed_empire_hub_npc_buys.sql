-- One-time seed: NPC buy orders (bids) at the four classic empire trade hubs.
-- Owner 1000125 matches Trader Joe (CONCORD) in MarketBotMgr.
-- Verify station IDs exist in staStations before running; adjust IN list if your DB differs.
--
-- use evemu;   -- set to your database name

SET @owner = 1000125;
SET @issued = 132478179209572976;
SET @duration = 5;
SET @vol = 550;

INSERT INTO mktOrders (
    typeID, ownerID, regionID, stationID, solarSystemID, orderRange,
    bid, price, escrow, minVolume, volEntered, volRemaining,
    issued, contraband, duration, jumps, isCorp, accountKey, memberID
)
SELECT
    it.typeID,
    @owner,
    s.regionID,
    s.stationID,
    s.solarSystemID,
    32767,
    1,
    GREATEST(COALESCE(it.basePrice, 0) * 1.05, 100),
    GREATEST(COALESCE(it.basePrice, 0) * 1.05, 100) * @vol,
    1,
    @vol,
    @vol,
    @issued,
    0,
    @duration,
    1,
    0,
    1000,
    0
FROM staStations s
CROSS JOIN invTypes it
INNER JOIN invGroups ig ON it.groupID = ig.groupID
WHERE s.stationID IN (60003760, 60011866, 60008494, 60005686)
  AND it.published = 1
  AND ig.categoryID IN (
      4, 5, 6, 7, 8, 9, 16, 17, 18, 22, 23, 24, 25, 32, 34, 35, 39, 40, 41, 42, 43, 46
  );

UPDATE mktOrders SET price = 100, escrow = 100 * @vol
WHERE ownerID = @owner AND bid = 1 AND price = 0;
