-- Juro finale (58373): second reward slot for a rolled Blood Raider ship blueprint copy (Cruor / Succubus / Ashimmu; no Bhaalgorn).
-- Module reward stays in rewardItemID; BPC blueprint typeID is stored in rewardExtraItemID.
--
-- +migrate Up
ALTER TABLE agtOffers
    ADD COLUMN rewardExtraItemID SMALLINT UNSIGNED NOT NULL DEFAULT 0
    AFTER rewardItemQty;

-- +migrate Down
ALTER TABLE agtOffers DROP COLUMN rewardExtraItemID;
