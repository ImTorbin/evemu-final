-- CQ: 20260428150000 copied player 90000000's full paper doll onto Juro 90000007, so the remote
-- avatar was identical to the logged-in toon. Keep mission/conversation on 90000007 but load the
-- 3D look from a *different* character: the earliest-created one that is not 90000000 and has
-- avatars (typically the second toon, e.g. 90000001). If you only have 90000000, appearance
-- remains 0 (doll on 90000007 only — re-seed or set staCQCustomAgents.appearanceCharID manually).
--
-- +migrate Up
-- Restore Juro NPC card to male Blood Raider type (portrait / agent) — not a clone of 90000000.
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

-- Earliest char with a paper doll, excluding 90000000 (first-created main) so Juro is not "you".
SET @doll := (
    SELECT c.characterID
    FROM chrCharacters c
    INNER JOIN avatars a ON a.charID = c.characterID
    WHERE c.characterID <> 90000000
    ORDER BY c.createDateTime ASC, c.characterID ASC
    LIMIT 1
);

UPDATE staCQCustomAgents
SET
    appearanceCharID = IFNULL(@doll, 0),
    updatedAt = UNIX_TIMESTAMP(CURRENT_TIMESTAMP)
WHERE
    stationID = 60007117
    AND missionAgentID = 90000007;

-- +migrate Down
-- Manual: set appearanceCharID and chrNPC to prior values if needed.
