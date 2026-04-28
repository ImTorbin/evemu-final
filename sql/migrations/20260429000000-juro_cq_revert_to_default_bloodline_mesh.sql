-- Undo 20260428200000: that migration cloned a random male player's paper doll onto Juro (90000007),
-- which looks like a mismatched "random outfit". Juro's original CQ look was bloodline type 1384
-- (see 20260425370000) with *no* avatars / avatar_* / chrPortraitData — the client uses the default
-- mesh/outfit for that bloodline instead of player paperdoll resources.
--
-- +migrate Up
UPDATE chrNPCCharacters
SET
    typeID = 1384,
    gender = 1
WHERE characterID = 90000007;

DELETE FROM avatar_colors WHERE charID = 90000007;
DELETE FROM avatar_modifiers WHERE charID = 90000007;
DELETE FROM avatar_sculpts WHERE charID = 90000007;
DELETE FROM chrPortraitData WHERE charID = 90000007;
DELETE FROM avatars WHERE charID = 90000007;

UPDATE staCQCustomAgents
SET
    appearanceCharID = 0,
    updatedAt = UNIX_TIMESTAMP(CURRENT_TIMESTAMP)
WHERE missionAgentID = 90000007;

-- +migrate Down
-- Re-run 20260428200000 manually if you want the cloned player doll again.
