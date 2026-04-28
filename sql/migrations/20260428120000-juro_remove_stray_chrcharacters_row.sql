-- Remove duplicate player identity for mission agent Juro (90000007) if present.
-- Public info for agtAgents-linked agents is served from chrNPCCharacters; a chrCharacters
-- row forces a player-style Character Information sheet.
-- +migrate Up
DELETE FROM chrCharacters WHERE characterID = 90000007;

-- +migrate Down
-- Intentionally empty: we do not restore a synthetic player row.
