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
    Author:        Allan
*/


#include "eve-server.h"

#include <set>
#include <sstream>

#include "../../eve-common/EVE_Calendar.h"
#include "system/CalendarDB.h"

namespace {

    void EscapeCalText(std::string& out, const std::string& in)
    {
        sDatabase.DoEscapeString(out, in);
    }

    std::string BuildInviteeListCsv(PyList* list)
    {
        if (list == nullptr)
            return {};
        std::ostringstream oss;
        bool first = true;
        for (PyRep* p : list->items) {
            const uint32 id = PyRep::IntegerValueU32(p);
            if (id == 0)
                continue;
            if (!first)
                oss << ',';
            first = false;
            oss << id;
        }
        return oss.str();
    }

    void ParseInviteeCsv(const char* csv, std::set<uint32>& out)
    {
        if (csv == nullptr || *csv == 0)
            return;
        std::string s(csv);
        size_t pos = 0;
        while (pos < s.length()) {
            size_t comma = s.find(',', pos);
            std::string token = (comma == std::string::npos) ? s.substr(pos) : s.substr(pos, comma - pos);
            if (!token.empty()) {
                const uint32 id = strtoul(token.c_str(), nullptr, 10);
                if (id)
                    out.insert(id);
            }
            if (comma == std::string::npos)
                break;
            pos = comma + 1;
        }
    }

    void CollectCharIds(PyList* list, std::set<uint32>& out)
    {
        if (list == nullptr)
            return;
        for (PyRep* p : list->items) {
            const uint32 id = PyRep::IntegerValueU32(p);
            if (id)
                out.insert(id);
        }
    }

    std::string FormatInviteeCsv(const std::set<uint32>& ids)
    {
        std::ostringstream oss;
        bool first = true;
        for (uint32 id : ids) {
            if (!first)
                oss << ',';
            first = false;
            oss << id;
        }
        return oss.str();
    }

} // namespace


void CalendarDB::DeleteEvent(uint32 eventID)
{
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE sysCalendarEvents SET isDeleted = 1 WHERE eventID = %u", eventID);
}

// for personal char events
PyRep* CalendarDB::SaveNewEvent(uint32 ownerID, Call_CreateEventWithInvites& args)
{
    EvE::TimeParts data = GetTimeParts(args.startDateTime);

    std::string titleEsc, descEsc;
    EscapeCalText(titleEsc, args.title);
    EscapeCalText(descEsc, args.description);

    uint32 eventID(0);
    DBerror err;
    if (args.duration) {
        if (!sDatabase.RunQueryLID(err, eventID,
            "INSERT INTO sysCalendarEvents(ownerID, creatorID, eventDateTime, eventDuration, importance,"
            " eventTitle, eventText, flag, month, year)"
            " VALUES (%u, %u, %lli, %u, %u, '%s', '%s', %u, %u, %u)",
            ownerID, ownerID, args.startDateTime, args.duration, args.important, titleEsc.c_str(),
            descEsc.c_str(), Calendar::Flag::Personal, data.month, data.year))
        {
            codelog(DATABASE__ERROR, "Error in SaveNewEvent query: %s", err.c_str());
            return PyStatic.NewZero();
        }
    } else {
        if (!sDatabase.RunQueryLID(err, eventID,
            "INSERT INTO sysCalendarEvents(ownerID, creatorID, eventDateTime, importance,"
            " eventTitle, eventText, flag, month, year)"
            " VALUES (%u, %u, %lli, %u, '%s', '%s', %u, %u, %u)",
            ownerID, ownerID, args.startDateTime, args.important, titleEsc.c_str(),
            descEsc.c_str(), Calendar::Flag::Personal, data.month, data.year))
        {
            codelog(DATABASE__ERROR, "Error in SaveNewEvent query: %s", err.c_str());
            return PyStatic.NewZero();
        }
    }

    if (args.invitees != nullptr && !args.invitees->empty()) {
        const std::string csv = BuildInviteeListCsv(args.invitees);
        if (!csv.empty()) {
            std::string csvEsc;
            EscapeCalText(csvEsc, csv);
            if (!sDatabase.RunQuery(err,
                    "INSERT INTO `sysCalendarInvitees`(`eventID`, `inviteeList`)"
                    " VALUES (%u, '%s')", eventID, csvEsc.c_str()))
                codelog(DATABASE__ERROR, "SaveNewEvent invitees: %s", err.c_str());
        }
    }

    return new PyInt(eventID);
}

// for corp/alliance events
PyRep* CalendarDB::SaveNewEvent(uint32 ownerID, uint32 creatorID, Call_CreateEvent &args)
{
    uint8 flag(Calendar::Flag::Invalid);
    if (IsCharacterID(ownerID)) {
     flag = Calendar::Flag::Personal;
    } else if (IsCorp(ownerID)) { // this would also be Automated for pos fuel
        flag = Calendar::Flag::Corp;
    } else if (IsAlliance(ownerID)) {
        flag = Calendar::Flag::Alliance;
    } else if (ownerID == 1) {
        flag = Calendar::Flag::CCP;
    } else {
        flag = Calendar::Flag::Automated;
    }

    EvE::TimeParts data = EvE::TimeParts();
    data = GetTimeParts(args.startDateTime);

    std::string titleEsc, descEsc;
    EscapeCalText(titleEsc, args.title);
    EscapeCalText(descEsc, args.description);

    uint32 eventID(0);
    DBerror err;
    if (args.duration) {
        if (!sDatabase.RunQueryLID(err, eventID,
            "INSERT INTO sysCalendarEvents(ownerID, creatorID, eventDateTime, eventDuration, importance,"
            " eventTitle, eventText, flag, month, year)"
            " VALUES (%u, %u, %lli, %u, %u, '%s', '%s', %u, %u, %u)",
            ownerID, creatorID, args.startDateTime, args.duration, args.important,
            titleEsc.c_str(), descEsc.c_str(), flag, data.month, data.year))
        {
            codelog(DATABASE__ERROR, "Error in SaveNewEvent query: %s", err.c_str());
            return PyStatic.NewZero();
        }
    } else {
        if (!sDatabase.RunQueryLID(err, eventID,
            "INSERT INTO sysCalendarEvents(ownerID, creatorID, eventDateTime, importance,"
            " eventTitle, eventText, flag, month, year)"
            " VALUES (%u, %u, %lli, %u, '%s', '%s', %u, %u, %u)",
            ownerID, creatorID, args.startDateTime, args.important,
            titleEsc.c_str(), descEsc.c_str(), flag, data.month, data.year))
        {
            codelog(DATABASE__ERROR, "Error in SaveNewEvent query: %s", err.c_str());
            return PyStatic.NewZero();
        }
    }

    return new PyInt(eventID);
}

// for system/auto events
uint32 CalendarDB::SaveSystemEvent(uint32 ownerID, uint32 creatorID, int64 startDateTime, uint8 autoEventType,
                                   std::string title, std::string description, bool important/*false*/)
{
    EvE::TimeParts data = EvE::TimeParts();
    data = GetTimeParts(startDateTime);

    std::string titleEsc, descEsc;
    EscapeCalText(titleEsc, title);
    EscapeCalText(descEsc, description);

    uint32 eventID(0);
    DBerror err;
    sDatabase.RunQueryLID(err, eventID,
        "INSERT INTO sysCalendarEvents(ownerID, creatorID, eventDateTime, autoEventType,"
        " eventTitle, eventText, flag, month, year, importance)"
        " VALUES (%u, %u, %lli, %u, '%s', '%s', %u, %u, %u, %u)",
        ownerID, creatorID, startDateTime, autoEventType, titleEsc.c_str(), descEsc.c_str(),
        Calendar::Flag::Automated, data.month, data.year, important?1:0);

    return eventID;
}


PyRep* CalendarDB::GetEventList(uint32 ownerID, uint32 month, uint32 year)
{
    PyList* list = new PyList();
    if (ownerID == 0)
        return list;

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT eventID, ownerID, eventDateTime, dateModified, eventDuration, importance, eventTitle, flag,"
        " autoEventType, isDeleted"
        " FROM sysCalendarEvents WHERE ownerID = %u AND month = %u AND year = %u"
        " AND IFNULL(isDeleted, 0) = 0", ownerID, month, year))
    {
        codelog(DATABASE__ERROR, "Error in GetEventList query: %s", res.error.c_str());
        return list;
    }

    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* dict = new PyDict();
            dict->SetItemString("eventID",              new PyInt(row.GetInt(0)));
            dict->SetItemString("ownerID",              new PyInt(row.GetInt(1)));
            dict->SetItemString("eventDateTime",        new PyLong(row.GetInt64(2)));
            dict->SetItemString("dateModified",         row.IsNull(3) ? PyStatic.NewNone() : new PyLong(row.GetInt64(3)));
            dict->SetItemString("eventDuration",        row.IsNull(4) ? PyStatic.NewNone() : new PyInt(row.GetInt(4)));
            dict->SetItemString("importance",           new PyBool(row.GetBool(5)));
            dict->SetItemString("eventTitle",           new PyString(row.GetText(6)));
            dict->SetItemString("flag",                 new PyInt(row.GetInt(7)));
            // client patch to allow non-corp automated events for ram jobs
            if (row.GetInt(7) == Calendar::Flag::Automated)
                dict->SetItemString("autoEventType",    new PyInt(row.GetInt(8)));
            dict->SetItemString("isDeleted",            new PyBool(row.GetBool(9)));
        list->AddItem(new PyObject("util.KeyVal",       dict));
    }

    return list;
}

PyRep* CalendarDB::GetEventDetails(uint32 eventID)
{
    if (eventID == 0)
        return nullptr;

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT ownerID, creatorID, eventText"
        " FROM sysCalendarEvents WHERE eventID = %u", eventID))
    {
        codelog(DATABASE__ERROR, "Error in GetEventList query: %s", res.error.c_str());
        return nullptr;
    }

    DBResultRow row;
    if (!res.GetRow(row))
        return nullptr;

    PyDict* dict = new PyDict();
        dict->SetItemString("eventID",          new PyInt(eventID));
        dict->SetItemString("ownerID",          new PyInt(row.GetInt(0)));
        dict->SetItemString("creatorID",        new PyInt(row.GetInt(1)));
        dict->SetItemString("eventText",        new PyString(row.GetText(2)));

    return new PyObject("util.KeyVal", dict);
}

void CalendarDB::SaveEventResponse(uint32 charID, uint32 eventID, uint32 response)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "DELETE FROM `sysCalendarResponses` WHERE `eventID` = %u AND `charID` = %u", eventID, charID);
    sDatabase.RunQuery(err,
        "INSERT INTO `sysCalendarResponses`(`eventID`, `charID`, `response`)"
        " VALUES (%u, %u, %u)", eventID, charID, response);
}

PyRep* CalendarDB::GetResponsesForCharacter(uint32 charID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,"SELECT eventID, response FROM sysCalendarResponses WHERE charID = %u", charID)) {
        codelog(DATABASE__ERROR, "Error in GetEventList query: %s", res.error.c_str());
        return nullptr;
    }

    DBResultRow row;
    PyList *list = new PyList();
    while (res.GetRow(row)) {
        // list char response for each event
        PyDict* dict = new PyDict();
            dict->SetItemString("eventID", new PyInt(row.GetInt(0)));
            dict->SetItemString("status",  new PyInt(row.GetInt(1)));
        list->AddItem(new PyObject("util.KeyVal", dict));
    }

    return list;
}

PyRep* CalendarDB::GetResponsesToEvent(uint32 eventID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,"SELECT charID, response FROM sysCalendarResponses WHERE eventID = %u", eventID)) {
        codelog(DATABASE__ERROR, "Error in GetEventList query: %s", res.error.c_str());
        return nullptr;
    }

    DBResultRow row;
    PyList *list = new PyList();
    while (res.GetRow(row)) {
        // list char response for each event
        PyDict* dict = new PyDict();
            dict->SetItemString("characterID", new PyInt(row.GetInt(0)));
            dict->SetItemString("status",  new PyInt(row.GetInt(1)));
        list->AddItem(new PyObject("util.KeyVal", dict));
    }

    return list;
}

void CalendarDB::UpdateEventParticipants(uint32 eventID, uint32 actingCharID, PyList* charsToAdd, PyList* charsToRemove)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
            "SELECT ownerID FROM sysCalendarEvents WHERE eventID = %u AND IFNULL(isDeleted, 0) = 0", eventID)) {
        codelog(DATABASE__ERROR, "UpdateEventParticipants: query failed: %s", res.error.c_str());
        return;
    }
    DBResultRow row;
    if (!res.GetRow(row)) {
        codelog(SERVICE__ERROR, "UpdateEventParticipants: event %u not found", eventID);
        return;
    }
    if (row.GetUInt(0) != actingCharID) {
        codelog(SERVICE__ERROR, "UpdateEventParticipants: char %u cannot modify event %u", actingCharID, eventID);
        return;
    }

    std::set<uint32> ids;
    DBQueryResult invRes;
    if (sDatabase.RunQuery(invRes, "SELECT inviteeList FROM sysCalendarInvitees WHERE eventID = %u", eventID)) {
        DBResultRow invRow;
        if (invRes.GetRow(invRow) && !invRow.IsNull(0))
            ParseInviteeCsv(invRow.GetText(0), ids);
    }

    CollectCharIds(charsToAdd, ids);
    if (charsToRemove != nullptr) {
        for (PyRep* p : charsToRemove->items) {
            const uint32 id = PyRep::IntegerValueU32(p);
            if (id)
                ids.erase(id);
        }
    }

    DBerror err;
    sDatabase.RunQuery(err, "DELETE FROM sysCalendarInvitees WHERE eventID = %u", eventID);
    if (!ids.empty()) {
        const std::string csv = FormatInviteeCsv(ids);
        std::string csvEsc;
        EscapeCalText(csvEsc, csv);
        sDatabase.RunQuery(err,
            "INSERT INTO sysCalendarInvitees (eventID, inviteeList) VALUES (%u, '%s')", eventID, csvEsc.c_str());
    }
}

bool CalendarDB::EditEvent(uint32 eventID, uint32 scopeOwnerID, int64 startDateTime, uint32 duration,
                           const std::string& title, const std::string& description, bool important)
{
    EvE::TimeParts data = GetTimeParts(startDateTime);
    std::string titleEsc, descEsc;
    EscapeCalText(titleEsc, title);
    EscapeCalText(descEsc, description);
    const int64 modified = (int64)GetFileTimeNow();
    const uint32 imp = important ? 1 : 0;

    DBerror err;
    uint32 affected = 0;
    bool ok;
    if (duration > 0) {
        ok = sDatabase.RunQuery(err, affected,
            "UPDATE sysCalendarEvents SET eventDateTime = %lli, eventDuration = %u, importance = %u,"
            " eventTitle = '%s', eventText = '%s', month = %u, year = %u, dateModified = %lli"
            " WHERE eventID = %u AND ownerID = %u AND IFNULL(isDeleted, 0) = 0",
            startDateTime, duration, imp, titleEsc.c_str(), descEsc.c_str(),
            data.month, data.year, modified, eventID, scopeOwnerID);
    } else {
        ok = sDatabase.RunQuery(err, affected,
            "UPDATE sysCalendarEvents SET eventDateTime = %lli, eventDuration = NULL, importance = %u,"
            " eventTitle = '%s', eventText = '%s', month = %u, year = %u, dateModified = %lli"
            " WHERE eventID = %u AND ownerID = %u AND IFNULL(isDeleted, 0) = 0",
            startDateTime, imp, titleEsc.c_str(), descEsc.c_str(),
            data.month, data.year, modified, eventID, scopeOwnerID);
    }

    if (!ok) {
        codelog(DATABASE__ERROR, "EditEvent failed: %s", err.c_str());
        return false;
    }
    if (affected == 0)
        codelog(SERVICE__ERROR, "EditEvent: no row updated (eventID %u owner %u)", eventID, scopeOwnerID);
    return affected > 0;
}
