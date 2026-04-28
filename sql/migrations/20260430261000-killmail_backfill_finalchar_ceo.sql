-- Rows cleared by killmail_final_character_not_chr: restore a valid NPC pilot id from corp CEO
-- (Crucible shows #System when finalCharacterID is 0).
-- +migrate Up
UPDATE chrKillTable k
INNER JOIN crpCorporation c ON c.corporationID = k.finalCorporationID
SET k.finalCharacterID = c.ceoID
WHERE k.finalCharacterID = 0 AND k.finalCorporationID != 0 AND c.ceoID != 0;

-- +migrate Down
-- Intentionally empty
