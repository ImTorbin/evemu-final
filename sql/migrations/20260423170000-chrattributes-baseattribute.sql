-- Ensure chrAttributes.baseAttribute exists (required by CharacterDB::GetAttributesFromAttributes).
-- Safe if an older DB was imported without running the full 20240614183142 migration.

-- +migrate Up

ALTER TABLE chrAttributes
    ADD COLUMN IF NOT EXISTS baseAttribute TINYINT(3) UNSIGNED DEFAULT 20 AFTER attributeName;

UPDATE chrAttributes
    SET baseAttribute = 19
    WHERE attributeName = 'Charisma';

-- +migrate Down

ALTER TABLE chrAttributes
    DROP COLUMN IF EXISTS baseAttribute;
