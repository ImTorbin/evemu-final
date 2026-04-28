-- +migrate Up
-- dunRoomObjects rows that use INSERT...SELECT FROM invTypes insert zero rows when typeID is missing,
-- leaving combat dungeons with no room objects and empty grids after warp.
-- Backfill one asteroid pirate frigate (by group) per empty room for combat-site archetypes.

INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius)
SELECT r.roomID, t.typeID, t.groupID, 0, 0, 0, 0, 0, 0, COALESCE(NULLIF(t.radius, 0), 500)
FROM dunRooms r
INNER JOIN dunDungeons d ON d.dungeonID = r.dungeonID
INNER JOIN invTypes t ON t.typeID = (
    SELECT it.typeID FROM invTypes it
    WHERE it.groupID IN (
        550, 551, 552, 555, 556, 557, 560, 561, 562, 565, 566, 567, 570, 571, 572
    )
    ORDER BY it.typeID ASC
    LIMIT 1
)
WHERE d.archetypeID IN (7, 8, 9, 10)
AND NOT EXISTS (SELECT 1 FROM dunRoomObjects o WHERE o.roomID = r.roomID);

-- +migrate Down
-- Non-reversible data repair; no safe automatic rollback.
SELECT 1;
