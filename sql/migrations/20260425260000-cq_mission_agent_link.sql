-- Link CQ spawns to real mission agents (agtAgents) so the client can use
-- standard "Agent Conversation" (entity charID = agentID) and optional
-- appearance override for custom dress.
-- +migrate Up
ALTER TABLE staCQCustomAgents
    ADD COLUMN missionAgentID INT(10) UNSIGNED NULL DEFAULT NULL
        COMMENT 'if set, OnCQAvatarMove[0] = this (agtAgents.agentID) for convo' AFTER appearanceCharID,
    ADD KEY idx_staCQCustomAgents_mission (missionAgentID);

-- Hides name-search hits in People & Places (searchResultAgent). Station lists may still use static data.
ALTER TABLE chrNPCCharacters
    ADD COLUMN excludedFromAgentSearch TINYINT(1) NOT NULL DEFAULT 0
        COMMENT '1 = omit from searchResultAgent / QuickQuery for agents';

-- +migrate Down
ALTER TABLE chrNPCCharacters DROP COLUMN excludedFromAgentSearch;
ALTER TABLE staCQCustomAgents DROP KEY idx_staCQCustomAgents_mission, DROP COLUMN missionAgentID;
