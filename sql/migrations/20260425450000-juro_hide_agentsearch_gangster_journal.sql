-- Juro (90000007): hide from People & Places agent *name* search (SearchDB) and match 20260425270000 intent.
-- The global Agent Finder tree also uses agentMgr.GetAgents(); that rowset now includes excludedFromAgentSearch
-- so a client mod can filter the tree. Stock client may still show the tree entry until patched.
-- Journal mission type label is overridden in code to "Gangster" (qstCourier.typeID stays 3 = courier).
-- +migrate Up
UPDATE chrNPCCharacters
SET excludedFromAgentSearch = 1
WHERE characterID = 90000007;

-- +migrate Down
UPDATE chrNPCCharacters
SET excludedFromAgentSearch = 0
WHERE characterID = 90000007;
