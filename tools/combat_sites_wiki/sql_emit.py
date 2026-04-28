"""Emit EVEmu dungeon SQL migration."""

from __future__ import annotations

import uuid
from dataclasses import dataclass
from typing import List, Tuple


def sql_escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace("'", "''")


@dataclass
class DungeonSQLSpec:
    dungeon_id: int
    room_id: int
    dungeon_name: str
    faction_id: int
    archetype_id: int
    objects: List[Tuple[int, int, float, float, float]]  # typeID, groupID, x, y, z


def emit_migration(
    specs: List[DungeonSQLSpec],
    header_comment: str,
) -> str:
    lines: List[str] = [
        "-- +migrate Up",
        "-- If piping the whole file into mysql/mariadb, stop before `-- +migrate Down` or seeds will be deleted.",
        "-- CC BY-SA: derived from https://wiki.eveuniversity.org/Combat_site",
        "-- " + header_comment.replace("\n", "\n-- "),
        "",
    ]
    for sp in specs:
        du = str(uuid.uuid4())
        lines.append(
            f"INSERT INTO dunDungeons (dungeonID, dungeonName, dungeonStatus, factionID, archetypeID, dungeonUUID) "
            f"VALUES ({sp.dungeon_id}, '{sql_escape(sp.dungeon_name)}', 0, {sp.faction_id}, {sp.archetype_id}, '{du}');"
        )
        lines.append(
            f"INSERT INTO dunRooms (roomID, roomName, dungeonID) VALUES ({sp.room_id}, 'Main', {sp.dungeon_id});"
        )
        for type_id, group_id, x, y, z in sp.objects:
            # Pull groupID + radius from invTypes so CSV need not carry groupID
            lines.append(
                f"INSERT INTO dunRoomObjects (roomID, typeID, groupID, x, y, z, yaw, pitch, roll, radius) "
                f"SELECT {sp.room_id}, {type_id}, groupID, {x}, {y}, {z}, 0, 0, 0, COALESCE(radius, 500) "
                f"FROM invTypes WHERE typeID = {type_id} LIMIT 1;"
            )
        lines.append("")
    lines.append("-- +migrate Down")
    for sp in reversed(specs):
        lines.append(f"DELETE FROM dunRoomObjects WHERE roomID = {sp.room_id};")
        lines.append(f"DELETE FROM dunRooms WHERE dungeonID = {sp.dungeon_id};")
        lines.append(f"DELETE FROM dunDungeons WHERE dungeonID = {sp.dungeon_id};")
    return "\n".join(lines) + "\n"
