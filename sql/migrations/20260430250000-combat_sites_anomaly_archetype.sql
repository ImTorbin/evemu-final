-- +migrate Up
-- dunDungeons.archetypeID must match Dungeon::Type for GetRandomDungeon (see EVE_Dungeon.h).
-- Archetype 7 = cosmic anomaly pool; 8 = unrated signature pool. UniWiki combat seeds should use 7 so they spawn as anomalies.
UPDATE dunDungeons SET archetypeID = 7 WHERE dungeonID >= 9200000 AND dungeonID <= 9201450 AND archetypeID IN (8, 9, 10);

-- +migrate Down
-- Prior archetype split (7 vs 8) not restored; re-export from combat_sites_wiki if needed.
