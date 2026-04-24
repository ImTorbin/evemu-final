
use evemu;   -- set this to your DB name
-- Hub-only default seeding to preserve trade incentives.
-- Regional depth should be handled by MarketBot (sprinkle).

create temporary table if not exists tStations (stationId int, solarSystemID int, regionID int, corporationID int, security float);
truncate table tStations;
insert into tStations
  select stationID, solarSystemID, regionID, corporationID, security
  from staStations
  where stationID in (60003760, 60011866, 60008494, 60005686);

-- actual seeding
INSERT INTO mktOrders (typeID, ownerID, regionID, stationID, price, volEntered, volRemaining, issued,
minVolume, duration, solarSystemID, jumps)
  SELECT typeID, corporationID, regionID, stationID, basePrice / security, 550, 550, 132478179209572976, 1, 250, solarSystemID, 1
  FROM tStations, invTypes inner join invGroups USING (groupID)
  WHERE invTypes.published = 1
  AND invGroups.categoryID IN (4, 5, 6, 7, 8, 9, 16, 17, 18, 22, 23, 24, 25, 32, 34, 35, 39, 40, 41, 42, 43, 46);
UPDATE mktOrders SET price = 100 WHERE price = 0;

