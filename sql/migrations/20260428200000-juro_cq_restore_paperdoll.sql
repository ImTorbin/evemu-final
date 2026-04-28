-- Juro CQ: 20260428160000 deleted all avatar_* on 90000007 and set typeID 1384. With no paper-doll
-- rows, the client uses the default mesh for that bloodline (robed / cultist NPC). Restore rows on
-- 90000007 and sync chrNPCCharacters.typeID + gender to the template so skeleton matches modifiers.
--
-- Template: first *male* (gender = 1) chrCharacters row that has avatars; if none, any char with avatars.
-- CQ uses protocol id 90000007 for GetPaperDollDataFor when appearanceCharID is 0 or equals mission id.
--
-- +migrate Up
SET @tpl := COALESCE(
    (
        SELECT c.characterID
        FROM chrCharacters c
        INNER JOIN avatars a ON a.charID = c.characterID
        WHERE c.gender = 1
        ORDER BY c.createDateTime ASC, c.characterID ASC
        LIMIT 1
    ),
    (
        SELECT c.characterID
        FROM chrCharacters c
        INNER JOIN avatars a ON a.charID = c.characterID
        ORDER BY c.createDateTime ASC, c.characterID ASC
        LIMIT 1
    )
);

SET @ok := IF(@tpl IS NOT NULL AND (SELECT COUNT(*) FROM avatars WHERE charID = @tpl) > 0, 1, 0);

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

-- Load doll from 90000007 in CQ (not appearanceCharID pointing at another toon).
UPDATE staCQCustomAgents
SET
    appearanceCharID = 0,
    updatedAt = UNIX_TIMESTAMP(CURRENT_TIMESTAMP)
WHERE
    missionAgentID = 90000007;

-- +migrate Down
-- Manual restore if needed.
