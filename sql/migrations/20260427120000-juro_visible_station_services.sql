-- Juro (90000007): show in Station Services -> Agents
--
-- `excludedFromAgentSearch` was set to 1 in CQ/seed migrations to reduce visibility in
-- People & Places name search. The stock Crucible client also uses this column when
-- building the *docked* station agent list, so the agent never appeared under Services
-- even when stationID matched.
--
-- Agent home station remains agtAgents.locationID / chrNPCCharacters.stationID
-- (60007117 in default seed). You must be docked at *that* station, not just the same
-- system as Raravath, for him to list together with per-station agent UI rules.
-- +migrate Up
UPDATE chrNPCCharacters
SET excludedFromAgentSearch = 0
WHERE characterID = 90000007;

-- +migrate Down
UPDATE chrNPCCharacters
SET excludedFromAgentSearch = 1
WHERE characterID = 90000007;
