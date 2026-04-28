-- Kill list double-click: client looks up finalCharacterID in EveOwners (characters only).
-- Older rows stored NPC/structure itemIDs (e.g. 750000047) and crashed CombatLog_CopyText.
-- +migrate Up
UPDATE chrKillTable k
LEFT JOIN chrCharacters c ON c.characterID = k.finalCharacterID
SET k.finalCharacterID = 0
WHERE k.finalCharacterID != 0 AND c.characterID IS NULL;

-- +migrate Down
-- Intentionally empty: cannot restore prior bogus IDs.
