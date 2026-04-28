/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        Reve
    Providing clothes to the poor
    Updates:    Allan
*/

#include "eve-server.h"

#include "character/PaperDollDB.h"

namespace {
/** Client expects util.Row (hairDarkness) for appearance; NPCs / empty avatars have no DB row. */
PyRep* DefaultPaperDollAppearanceRow() {
    PyDict* args = new PyDict();
    PyList* header = new PyList(1);
    header->SetItemString(0, "hairDarkness");
    args->SetItemString("header", header);
    PyList* line = new PyList(1);
    line->SetItem(0, new PyFloat(0.5));
    args->SetItemString("line", line);
    return new PyObject("util.Row", args);
}
} // namespace

PyRep* PaperDollDB::GetPaperDollAvatar(uint32 charID) const {

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
		"SELECT hairDarkness FROM avatars WHERE charID=%u", charID))
    {
        _log(DATABASE__ERROR, "Error in GetMyPaperDollData query: %s", res.error.c_str());
        return nullptr;
    }

	DBResultRow row;
	if (!res.GetRow(row))
        return DefaultPaperDollAppearanceRow();

	return DBRowToRow(row, "util.Row");
}

PyRep* PaperDollDB::GetPaperDollAvatarColors(uint32 charID) const {

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
		"SELECT colorID, colorNameA, colorNameBC, weight, gloss  FROM avatar_colors WHERE charID=%u", charID))
    {
        _log(DATABASE__ERROR, "Error in GetMyPaperDollData query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToCRowset(res);
}

PyRep* PaperDollDB::GetPaperDollAvatarModifiers(uint32 charID) const {

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
		"SELECT modifierLocationID, paperdollResourceID, paperdollResourceVariation FROM avatar_modifiers WHERE charID=%u", charID))
    {
        _log(DATABASE__ERROR, "Error in GetMyPaperDollData query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToCRowset(res);
}

PyRep* PaperDollDB::GetPaperDollAvatarSculpts(uint32 charID) const {

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
		"SELECT sculptLocationID, weightUpDown, weightLeftRight, weightForwardBack FROM avatar_sculpts WHERE charID=%u", charID))
    {
        _log(DATABASE__ERROR, "Error in GetMyPaperDollData query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToCRowset(res);
}

PyRep* PaperDollDB::GetPaperDollPortraitData(uint32 charID) const
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT * FROM chrPortraitData WHERE charID = %u", charID)) {
        _log(DATABASE__ERROR, "Error in GetMyPaperDollData query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToCRowset(res);
}

uint32 PaperDollDB::ResolvePaperDollSourceCharID(uint32 characterID, uint32 excludeDonorCharID) const
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT gender FROM chrNPCCharacters WHERE characterID=%u LIMIT 1",
        characterID))
    {
        _log(DATABASE__ERROR, "ResolvePaperDollSourceCharID: %s", res.error.c_str());
        return characterID;
    }
    DBResultRow row;
    if (!res.GetRow(row)) {
        return characterID;
    }

    const uint32 gender = row.GetUInt(0);

    if (sDatabase.RunQuery(res, "SELECT 1 FROM avatar_modifiers WHERE charID=%u LIMIT 1", characterID)) {
        if (res.GetRow(row)) {
            return characterID;
        }
    }

    /** Prefer chrNPCCharacters rows with outfits (seed donors), then oldest; optional exclude = viewer. */
    const char kOrderNpcFirst[] =
        " ORDER BY CASE WHEN EXISTS (SELECT 1 FROM chrNPCCharacters AS n WHERE n.characterID = c.characterID) "
        "  THEN 0 ELSE 1 END, c.createDateTime ASC, c.characterID ASC LIMIT 1";

    if (excludeDonorCharID != 0) {
        if (sDatabase.RunQuery(res,
            "SELECT c.characterID FROM chrCharacters AS c "
            "INNER JOIN avatars AS a ON a.charID = c.characterID "
            "WHERE c.gender = %u AND c.characterID != %u "
            "AND EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)"
            "%s",
            gender, excludeDonorCharID, kOrderNpcFirst))
        {
            if (res.GetRow(row)) {
                const uint32 tpl = row.GetUInt(0);
                _log(DATABASE__MESSAGE, "PaperDoll: NPC %u -> donor %u (gender, exclude %u)",
                    characterID, tpl, excludeDonorCharID);
                return tpl;
            }
        }
        if (sDatabase.RunQuery(res,
            "SELECT c.characterID FROM chrCharacters AS c "
            "INNER JOIN avatars AS a ON a.charID = c.characterID "
            "WHERE c.characterID != %u "
            "AND EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)"
            "%s",
            excludeDonorCharID, kOrderNpcFirst))
        {
            if (res.GetRow(row)) {
                const uint32 tpl = row.GetUInt(0);
                _log(DATABASE__MESSAGE, "PaperDoll: NPC %u -> donor %u (any gender, exclude %u)",
                    characterID, tpl, excludeDonorCharID);
                return tpl;
            }
        }
    }

    if (sDatabase.RunQuery(res,
        "SELECT c.characterID FROM chrCharacters AS c "
        "INNER JOIN avatars AS a ON a.charID = c.characterID "
        "WHERE c.gender = %u "
        "AND EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)"
        "%s",
        gender, kOrderNpcFirst))
    {
        if (res.GetRow(row)) {
            const uint32 tpl = row.GetUInt(0);
            _log(DATABASE__MESSAGE, "PaperDoll: NPC %u -> donor %u (gender %u, last-resort pool)",
                characterID, tpl, gender);
            return tpl;
        }
    }

    if (sDatabase.RunQuery(res,
        "SELECT c.characterID FROM chrCharacters AS c "
        "INNER JOIN avatars AS a ON a.charID = c.characterID "
        "WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID)"
        "%s",
        kOrderNpcFirst))
    {
        if (res.GetRow(row)) {
            const uint32 tpl = row.GetUInt(0);
            _log(DATABASE__MESSAGE, "PaperDoll: NPC %u -> donor %u (fallback pool)",
                characterID, tpl);
            return tpl;
        }
    }

    return characterID;
}
