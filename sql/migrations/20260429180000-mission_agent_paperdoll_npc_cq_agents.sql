-- Mission agents (real chrNPCCharacters / agtAgents IDs): full paper doll + chrPortraitData on the
-- agent characterID so paperDollServer uses their own row (ResolvePaperDollSourceCharID early-exits),
-- CQ 3D matches doll-driven UI, and Show Info / conversation can render the body.
--
-- Data source: clone from an existing EVEmu character with avatars + avatar_modifiers (+ portrait).
-- Preference order: same typeID as NPC (bloodline mesh), else same gender, else any seeded toon.
-- This approximates TQ DNA when a static dump is not available; replace with exact exports if you have them.
--
-- Targets: CQ log / Agent Finder agents (extend by copying the block and changing @npc).
-- +migrate Up

-- 3013073 Azedi Jihyetu
SET @npc := 3013073;
SET @tpl := (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
      AND c.typeID = (SELECT n.typeID FROM chrNPCCharacters AS n WHERE n.characterID = @npc LIMIT 1)
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
);
SET @tpl := COALESCE(@tpl, (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
      AND c.gender = (SELECT n.gender FROM chrNPCCharacters AS n WHERE n.characterID = @npc LIMIT 1)
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
));
SET @tpl := COALESCE(@tpl, (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
));
SET @ok := IFNULL((SELECT IF(COUNT(*), 1, 0) FROM avatars WHERE charID = @tpl), 0);
SET @has_npc := IFNULL((SELECT IF(COUNT(*), 1, 0) FROM chrNPCCharacters WHERE characterID = @npc), 0);

DELETE FROM avatar_colors WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatar_modifiers WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatar_sculpts WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM chrPortraitData WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatars WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatars (charID, hairDarkness)
SELECT @npc, t.hairDarkness FROM avatars t WHERE t.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_colors (charID, colorID, colorNameA, colorNameBC, weight, gloss)
SELECT @npc, c.colorID, c.colorNameA, c.colorNameBC, c.weight, c.gloss
FROM avatar_colors c WHERE c.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_modifiers (charID, modifierLocationID, paperdollResourceID, paperdollResourceVariation)
SELECT @npc, m.modifierLocationID, m.paperdollResourceID, m.paperdollResourceVariation
FROM avatar_modifiers m WHERE m.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_sculpts (charID, sculptLocationID, weightUpDown, weightLeftRight, weightForwardBack)
SELECT @npc, s.sculptLocationID, s.weightUpDown, s.weightLeftRight, s.weightForwardBack
FROM avatar_sculpts s WHERE s.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO chrPortraitData (
    charID, backgroundID, lightID, lightColorID, cameraX, cameraY, cameraZ,
    cameraPoiX, cameraPoiY, cameraPoiZ, headLookTargetX, headLookTargetY, headLookTargetZ,
    lightIntensity, headTilt, orientChar, browLeftCurl, browLeftTighten, browLeftUpDown,
    browRightCurl, browRightTighten, browRightUpDown, eyeClose, eyesLookVertical, eyesLookHorizontal,
    squintLeft, squintRight, jawSideways, jawUp, puckerLips, frownLeft, frownRight,
    smileLeft, smileRight, cameraFieldOfView, portraitPoseNumber
)
SELECT
    @npc,
    p.backgroundID, p.lightID, p.lightColorID, p.cameraX, p.cameraY, p.cameraZ,
    p.cameraPoiX, p.cameraPoiY, p.cameraPoiZ, p.headLookTargetX, p.headLookTargetY, p.headLookTargetZ,
    p.lightIntensity, p.headTilt, p.orientChar, p.browLeftCurl, p.browLeftTighten, p.browLeftUpDown,
    p.browRightCurl, p.browRightTighten, p.browRightUpDown, p.eyeClose, p.eyesLookVertical, p.eyesLookHorizontal,
    p.squintLeft, p.squintRight, p.jawSideways, p.jawUp, p.puckerLips, p.frownLeft, p.frownRight,
    p.smileLeft, p.smileRight, p.cameraFieldOfView, p.portraitPoseNumber
FROM chrPortraitData p WHERE p.charID = @tpl AND @ok = 1 AND @has_npc = 1;

-- 3013434 Fasaral Hanavi
SET @npc := 3013434;
SET @tpl := (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
      AND c.typeID = (SELECT n.typeID FROM chrNPCCharacters AS n WHERE n.characterID = @npc LIMIT 1)
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
);
SET @tpl := COALESCE(@tpl, (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
      AND c.gender = (SELECT n.gender FROM chrNPCCharacters AS n WHERE n.characterID = @npc LIMIT 1)
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
));
SET @tpl := COALESCE(@tpl, (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
));
SET @ok := IFNULL((SELECT IF(COUNT(*), 1, 0) FROM avatars WHERE charID = @tpl), 0);
SET @has_npc := IFNULL((SELECT IF(COUNT(*), 1, 0) FROM chrNPCCharacters WHERE characterID = @npc), 0);

DELETE FROM avatar_colors WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatar_modifiers WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatar_sculpts WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM chrPortraitData WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatars WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatars (charID, hairDarkness)
SELECT @npc, t.hairDarkness FROM avatars t WHERE t.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_colors (charID, colorID, colorNameA, colorNameBC, weight, gloss)
SELECT @npc, c.colorID, c.colorNameA, c.colorNameBC, c.weight, c.gloss
FROM avatar_colors c WHERE c.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_modifiers (charID, modifierLocationID, paperdollResourceID, paperdollResourceVariation)
SELECT @npc, m.modifierLocationID, m.paperdollResourceID, m.paperdollResourceVariation
FROM avatar_modifiers m WHERE m.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_sculpts (charID, sculptLocationID, weightUpDown, weightLeftRight, weightForwardBack)
SELECT @npc, s.sculptLocationID, s.weightUpDown, s.weightLeftRight, s.weightForwardBack
FROM avatar_sculpts s WHERE s.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO chrPortraitData (
    charID, backgroundID, lightID, lightColorID, cameraX, cameraY, cameraZ,
    cameraPoiX, cameraPoiY, cameraPoiZ, headLookTargetX, headLookTargetY, headLookTargetZ,
    lightIntensity, headTilt, orientChar, browLeftCurl, browLeftTighten, browLeftUpDown,
    browRightCurl, browRightTighten, browRightUpDown, eyeClose, eyesLookVertical, eyesLookHorizontal,
    squintLeft, squintRight, jawSideways, jawUp, puckerLips, frownLeft, frownRight,
    smileLeft, smileRight, cameraFieldOfView, portraitPoseNumber
)
SELECT
    @npc,
    p.backgroundID, p.lightID, p.lightColorID, p.cameraX, p.cameraY, p.cameraZ,
    p.cameraPoiX, p.cameraPoiY, p.cameraPoiZ, p.headLookTargetX, p.headLookTargetY, p.headLookTargetZ,
    p.lightIntensity, p.headTilt, p.orientChar, p.browLeftCurl, p.browLeftTighten, p.browLeftUpDown,
    p.browRightCurl, p.browRightTighten, p.browRightUpDown, p.eyeClose, p.eyesLookVertical, p.eyesLookHorizontal,
    p.squintLeft, p.squintRight, p.jawSideways, p.jawUp, p.puckerLips, p.frownLeft, p.frownRight,
    p.smileLeft, p.smileRight, p.cameraFieldOfView, p.portraitPoseNumber
FROM chrPortraitData p WHERE p.charID = @tpl AND @ok = 1 AND @has_npc = 1;

-- 3014539 (CQ snapshot / paperDoll GetPaperDollDataFor in logs)
SET @npc := 3014539;
SET @tpl := (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
      AND c.typeID = (SELECT n.typeID FROM chrNPCCharacters AS n WHERE n.characterID = @npc LIMIT 1)
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
);
SET @tpl := COALESCE(@tpl, (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
      AND c.gender = (SELECT n.gender FROM chrNPCCharacters AS n WHERE n.characterID = @npc LIMIT 1)
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
));
SET @tpl := COALESCE(@tpl, (
    SELECT c.characterID FROM chrCharacters AS c
    INNER JOIN avatars AS a ON a.charID = c.characterID
    WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)
      AND c.characterID <> @npc
    ORDER BY c.createDateTime ASC, c.characterID ASC LIMIT 1
));
SET @ok := IFNULL((SELECT IF(COUNT(*), 1, 0) FROM avatars WHERE charID = @tpl), 0);
SET @has_npc := IFNULL((SELECT IF(COUNT(*), 1, 0) FROM chrNPCCharacters WHERE characterID = @npc), 0);

DELETE FROM avatar_colors WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatar_modifiers WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatar_sculpts WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM chrPortraitData WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;
DELETE FROM avatars WHERE charID = @npc AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatars (charID, hairDarkness)
SELECT @npc, t.hairDarkness FROM avatars t WHERE t.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_colors (charID, colorID, colorNameA, colorNameBC, weight, gloss)
SELECT @npc, c.colorID, c.colorNameA, c.colorNameBC, c.weight, c.gloss
FROM avatar_colors c WHERE c.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_modifiers (charID, modifierLocationID, paperdollResourceID, paperdollResourceVariation)
SELECT @npc, m.modifierLocationID, m.paperdollResourceID, m.paperdollResourceVariation
FROM avatar_modifiers m WHERE m.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO avatar_sculpts (charID, sculptLocationID, weightUpDown, weightLeftRight, weightForwardBack)
SELECT @npc, s.sculptLocationID, s.weightUpDown, s.weightLeftRight, s.weightForwardBack
FROM avatar_sculpts s WHERE s.charID = @tpl AND @ok = 1 AND @has_npc = 1;

INSERT INTO chrPortraitData (
    charID, backgroundID, lightID, lightColorID, cameraX, cameraY, cameraZ,
    cameraPoiX, cameraPoiY, cameraPoiZ, headLookTargetX, headLookTargetY, headLookTargetZ,
    lightIntensity, headTilt, orientChar, browLeftCurl, browLeftTighten, browLeftUpDown,
    browRightCurl, browRightTighten, browRightUpDown, eyeClose, eyesLookVertical, eyesLookHorizontal,
    squintLeft, squintRight, jawSideways, jawUp, puckerLips, frownLeft, frownRight,
    smileLeft, smileRight, cameraFieldOfView, portraitPoseNumber
)
SELECT
    @npc,
    p.backgroundID, p.lightID, p.lightColorID, p.cameraX, p.cameraY, p.cameraZ,
    p.cameraPoiX, p.cameraPoiY, p.cameraPoiZ, p.headLookTargetX, p.headLookTargetY, p.headLookTargetZ,
    p.lightIntensity, p.headTilt, p.orientChar, p.browLeftCurl, p.browLeftTighten, p.browLeftUpDown,
    p.browRightCurl, p.browRightTighten, p.browRightUpDown, p.eyeClose, p.eyesLookVertical, p.eyesLookHorizontal,
    p.squintLeft, p.squintRight, p.jawSideways, p.jawUp, p.puckerLips, p.frownLeft, p.frownRight,
    p.smileLeft, p.smileRight, p.cameraFieldOfView, p.portraitPoseNumber
FROM chrPortraitData p WHERE p.charID = @tpl AND @ok = 1 AND @has_npc = 1;

-- Verify after migrate Up:
--   SELECT charID, COUNT(*) FROM avatar_modifiers WHERE charID IN (3013073,3013434,3014539) GROUP BY charID;
--   SELECT charID FROM chrPortraitData WHERE charID IN (3013073,3013434,3014539);
-- In server logs, open Show Info for 3013073: expect NO Cyan "[PaperDoll] doll bundle req=3013073 -> dollSource=..." (own rows used).

-- +migrate Down
DELETE FROM avatar_colors WHERE charID IN (3013073, 3013434, 3014539);
DELETE FROM avatar_modifiers WHERE charID IN (3013073, 3013434, 3014539);
DELETE FROM avatar_sculpts WHERE charID IN (3013073, 3013434, 3014539);
DELETE FROM chrPortraitData WHERE charID IN (3013073, 3013434, 3014539);
DELETE FROM avatars WHERE charID IN (3013073, 3013434, 3014539);
