-- Juro (90000007): Station Services -> Agents lists only standard mission agents (agtAgents.agentTypeID = 2 Basic).
-- Seed used agentTypeID = 1 (None in EVE_Agent.h), so the client hid him while agentMgr bind/conversation still worked.
--
-- +migrate Up
UPDATE agtAgents
SET agentTypeID = 2
WHERE agentID = 90000007;

-- +migrate Down
UPDATE agtAgents
SET agentTypeID = 1
WHERE agentID = 90000007;
