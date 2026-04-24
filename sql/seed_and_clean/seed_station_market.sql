
 -- Hub-only market seeding.
 -- This intentionally seeds only major empire trade hubs and leaves wider
 -- regional coverage to MarketBot.
 -- use evemu;   -- set this to your DB name

create temporary table if not exists tStations (stationId int, solarSystemID int, regionID int);
truncate table tStations;
insert into tStations values
  (60003760, 30000142, 10000002), -- Jita IV - Caldari Navy Assembly Plant (The Forge)
  (60011866, 30002659, 10000032), -- Dodixie IX - Federation Navy Assembly Plant (Sinq Laison)
  (60008494, 30002187, 10000043), -- Amarr VIII - Emperor Family Academy (Domain)
  (60005686, 30002053, 10000042); -- Hek VIII - Boundless Creation Factory (Metropolis)

-- categoryID IDs and names found in seed_data.sql

-- actual seeding
INSERT INTO mktOrders (typeID, ownerID, regionID, stationID, price, volEntered, volRemaining, issued,
minVolume, duration, solarSystemID, jumps)
  SELECT typeID, stationID, regionID, stationID, basePrice, 550, 550, 132478179209572976, 1, 250, solarSystemID, 1
  FROM tStations, invTypes inner join invGroups on invTypes.groupID=invGroups.groupID
  WHERE invTypes.published = 1
  AND categoryID IN (4, 5, 6, 7, 8, 9, 16, 17, 18, 20, 22, 23, 24, 25, 32, 34, 35, 39, 40, 41, 42, 43, 46);
UPDATE mktOrders SET price = 100 WHERE price = 0;
-- ~11k orders per seeded station (depends on published type count)

