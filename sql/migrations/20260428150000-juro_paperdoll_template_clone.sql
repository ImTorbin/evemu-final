-- Juro (90000007) CQ: paperDollServer reads avatars/avatar_* by charID only. Rows missing or
-- bloodline (chrNPC.typeID) out of sync with paperdollResourceIDs => base body + underwear.
-- Clone from the default test character 90000000 when that character has avatars (stock EVEmu seed).
-- +migrate Up
SET @tpl := 90000000;
SET @ok := (SELECT IF(COUNT(*), 1, 0) FROM avatars WHERE charID = @tpl);

-- Match NPC card/snapshot bloodline+gender to the template so client skeleton matches outfit resources.
UPDATE chrNPCCharacters AS n
INNER JOIN chrCharacters AS t ON t.characterID = @tpl
SET
    n.typeID = t.typeID,
    n.gender = t.gender
WHERE
    n.characterID = 90000007
    AND @ok = 1;

DELETE FROM avatar_colors WHERE charID = 90000007 AND @ok = 1;
DELETE FROM avatar_modifiers WHERE charID = 90000007 AND @ok = 1;
DELETE FROM avatar_sculpts WHERE charID = 90000007 AND @ok = 1;
DELETE FROM chrPortraitData WHERE charID = 90000007 AND @ok = 1;
DELETE FROM avatars WHERE charID = 90000007 AND @ok = 1;

INSERT INTO avatars (charID, hairDarkness)
SELECT 90000007, t.hairDarkness FROM avatars t WHERE t.charID = @tpl AND @ok = 1;

INSERT INTO avatar_colors (charID, colorID, colorNameA, colorNameBC, weight, gloss)
SELECT
    90000007,
    c.colorID,
    c.colorNameA,
    c.colorNameBC,
    c.weight,
    c.gloss
FROM avatar_colors c
WHERE c.charID = @tpl AND @ok = 1;

INSERT INTO avatar_modifiers (charID, modifierLocationID, paperdollResourceID, paperdollResourceVariation)
SELECT
    90000007,
    m.modifierLocationID,
    m.paperdollResourceID,
    m.paperdollResourceVariation
FROM avatar_modifiers m
WHERE m.charID = @tpl AND @ok = 1;

INSERT INTO avatar_sculpts (charID, sculptLocationID, weightUpDown, weightLeftRight, weightForwardBack)
SELECT
    90000007,
    s.sculptLocationID,
    s.weightUpDown,
    s.weightLeftRight,
    s.weightForwardBack
FROM avatar_sculpts s
WHERE s.charID = @tpl AND @ok = 1;

INSERT INTO chrPortraitData (
    charID, backgroundID, lightID, lightColorID, cameraX, cameraY, cameraZ,
    cameraPoiX, cameraPoiY, cameraPoiZ, headLookTargetX, headLookTargetY, headLookTargetZ,
    lightIntensity, headTilt, orientChar, browLeftCurl, browLeftTighten, browLeftUpDown,
    browRightCurl, browRightTighten, browRightUpDown, eyeClose, eyesLookVertical, eyesLookHorizontal,
    squintLeft, squintRight, jawSideways, jawUp, puckerLips, frownLeft, frownRight,
    smileLeft, smileRight, cameraFieldOfView, portraitPoseNumber
)
SELECT
    90000007,
    p.backgroundID, p.lightID, p.lightColorID, p.cameraX, p.cameraY, p.cameraZ,
    p.cameraPoiX, p.cameraPoiY, p.cameraPoiZ, p.headLookTargetX, p.headLookTargetY, p.headLookTargetZ,
    p.lightIntensity, p.headTilt, p.orientChar, p.browLeftCurl, p.browLeftTighten, p.browLeftUpDown,
    p.browRightCurl, p.browRightTighten, p.browRightUpDown, p.eyeClose, p.eyesLookVertical, p.eyesLookHorizontal,
    p.squintLeft, p.squintRight, p.jawSideways, p.jawUp, p.puckerLips, p.frownLeft, p.frownRight,
    p.smileLeft, p.smileRight, p.cameraFieldOfView, p.portraitPoseNumber
FROM chrPortraitData p
WHERE p.charID = @tpl AND @ok = 1;

-- +migrate Down
-- No-op: previous doll state is not stored.
