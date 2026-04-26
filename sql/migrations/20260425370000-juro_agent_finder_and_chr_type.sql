-- Juro Darksynth (90000007): make him discoverable + loadable as a normal station agent.
--
-- 1) excludedFromAgentSearch = 1 was set in 20260425270000 / 20260425320000 to hide the NPC
--    from People & Places agent name search. For debugging, clear that flag so "Juro" / "Darksynth"
--    works in the agent finder.
--
-- 2) chrNPCCharacters.typeID = 0 breaks AgentDB::LoadAgentData() (early return when bloodline
--    type is missing), so agentMgr / station UIs may not treat 90000007 as a full agent. Use a
--    valid bloodline typeID present in Crucible static data (same family as many seeded NPCs).
--
-- +migrate Up
UPDATE chrNPCCharacters
SET
    excludedFromAgentSearch = 0,
    typeID = 1384
WHERE characterID = 90000007;

-- +migrate Down
UPDATE chrNPCCharacters
SET
    excludedFromAgentSearch = 1,
    typeID = 0
WHERE characterID = 90000007;
