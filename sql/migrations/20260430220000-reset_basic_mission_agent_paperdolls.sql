-- One-time reset: remove persisted doll rows for Basic non-locator mission agents so the server can
-- re-seed them with EnsureRandomMissionAgentPaperDollInDb (random template, then stable).
-- agentTypeID 2 = Agents::Type::Basic (EVE_Agent.h).
-- +migrate Up

DELETE ac FROM avatar_colors ac
 INNER JOIN agtAgents ag ON ag.agentID = ac.charID
 WHERE ag.isLocator = 0 AND ag.agentTypeID = 2;

DELETE am FROM avatar_modifiers am
 INNER JOIN agtAgents ag ON ag.agentID = am.charID
 WHERE ag.isLocator = 0 AND ag.agentTypeID = 2;

DELETE ascu FROM avatar_sculpts ascu
 INNER JOIN agtAgents ag ON ag.agentID = ascu.charID
 WHERE ag.isLocator = 0 AND ag.agentTypeID = 2;

DELETE pd FROM chrPortraitData pd
 INNER JOIN agtAgents ag ON ag.agentID = pd.charID
 WHERE ag.isLocator = 0 AND ag.agentTypeID = 2;

DELETE av FROM avatars av
 INNER JOIN agtAgents ag ON ag.agentID = av.charID
 WHERE ag.isLocator = 0 AND ag.agentTypeID = 2;

-- +migrate Down
-- Doll state cannot be restored.
