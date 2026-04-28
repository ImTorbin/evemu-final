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
    Author:     Zhur
    Updates:     Allan
*/

#include <boost/algorithm/string.hpp>
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include "eve-server.h"

#include "EntityList.h"
#include "EVEServerConfig.h"
#include "../eve-common/EVE_Agent.h"
#include "ServiceDB.h"

namespace {
/** Hand-tuned CQ floor spots (Crucible Amarr quarters); first N agents use these in order. */
static constexpr struct {
    double x, y, z;
    float yaw;
} kCQMissionAgentSlots[] = {
    { 1.3350, 0.0019, -2.4842, 5.472f },
    { 2.768, -0.001, 1.402, 3.141f },
    { 7.393, -0.498, -1.959, 0.733f },
    { 1.031, -0.001, -2.686, 5.565f },
    { -0.350, 0.000, -3.924, 0.197f },
};
static constexpr unsigned kCQMissionAgentSlotCount = sizeof(kCQMissionAgentSlots) / sizeof(kCQMissionAgentSlots[0]);

constexpr double kCQLayoutEpsPos = 1e-4;
constexpr float kCQLayoutEpsYaw = 1e-3f;

static bool CQMissionAgentLayoutClose(double a, double b) {
    return std::fabs(a - b) < kCQLayoutEpsPos;
}

static bool CQMissionAgentYawClose(float a, float b) {
    return std::fabs(a - b) < kCQLayoutEpsYaw;
}

/** Legacy arc along the room; used for agents beyond kCQMissionAgentSlotCount. */
static void CQMissionAgentLayoutArc(unsigned index, unsigned total, double& x, double& y, double& z, float& yaw)
{
    constexpr double kPi = 3.14159265358979323846;
    y = 0.248;
    if (total == 0) {
        x = z = 0.0;
        yaw = 0.826088f;
        return;
    }
    const double cx = 0.15;
    const double cz = 4.05;
    const double R = 2.35;
    const double theta0 = 64.0 * kPi / 180.0;
    const double theta1 = 116.0 * kPi / 180.0;
    const double t = (total <= 1) ? 0.5 : static_cast<double>(index) / static_cast<double>(total - 1);
    const double theta = theta0 + (theta1 - theta0) * t;
    x = cx + R * std::sin(theta);
    z = cz + R * std::cos(theta);
    const float fx = static_cast<float>(cx - x);
    const float fz = static_cast<float>(cz - z);
    yaw = std::atan2(fx, fz);
}

/** Positions for mission CQ agents: predefined slots first, then arc. DB yaw gets +pi on wire (CQManager). */
static void CQMissionAgentLayoutSlot(unsigned index, unsigned total, double& x, double& y, double& z, float& yaw)
{
    if (index < kCQMissionAgentSlotCount) {
        x = kCQMissionAgentSlots[index].x;
        y = kCQMissionAgentSlots[index].y;
        z = kCQMissionAgentSlots[index].z;
        yaw = kCQMissionAgentSlots[index].yaw;
        return;
    }
    const unsigned arcTotal = (total > kCQMissionAgentSlotCount) ? (total - kCQMissionAgentSlotCount) : 1u;
    const unsigned arcIndex = index - kCQMissionAgentSlotCount;
    CQMissionAgentLayoutArc(arcIndex, arcTotal, x, y, z, yaw);
}
} // namespace

uint32 ServiceDB::SetClientSeed()
{
    DBQueryResult res;
    sDatabase.RunQuery(res, "SELECT ClientSeed FROM srvStatus WHERE AI = 1");
    DBResultRow row;
    res.GetRow(row);
    return row.GetInt(0);
}

bool ServiceDB::ValidateAccountName(CryptoChallengePacket& ccp, std::string& failMsg)
{
    if (ccp.user_name.empty()) {
        failMsg = "Account Name is empty.";
        return false;
    }
    if (ccp.user_name.length() < 3) {
        failMsg = "Account Name is too short.";
        return false;
    }
    //if (name.length() < 4)
    //    throw UserError ("CharNameInvalidMinLength");
    if (ccp.user_name.length() > 20) {    //client caps at 24
        failMsg = "Account Name is too long.";
        return false;
    }

    for (const auto cur : badChars)
        if (EvE::icontains(ccp.user_name, cur)) {
            failMsg = "Account Name contains invalid characters.";
            return false;
        }

    return true;
}

bool ServiceDB::GetAccountInformation( CryptoChallengePacket& ccp, AccountData& aData, std::string& failMsg )
{
    //added auto account    -allan 18Jan14   -UD 16Jan18  -ud 15Dec18  -ud failMsgs 15Jun19  -ud type 4Nov20
    std::string eLogin;
    sDatabase.DoEscapeString(eLogin, ccp.user_name);

    DBQueryResult res;
    if ( !sDatabase.RunQuery( res,
        "SELECT accountID, clientID, password, hash, role, type, online, banned, logonCount, lastLogin"
        " FROM account WHERE accountName = '%s'", eLogin.c_str() ) )
    {
        sLog.Error( "ServiceDB", "Error in query: %s.", res.error.c_str() );
        failMsg = "Error in DB Query";
        failMsg += ": Account not found for ";
        failMsg += eLogin;
        //failMsg += res.error.c_str();     // do we wanna sent the db error msg to client?
        return false;
    }

    DBResultRow row;
    if (!res.GetRow( row )) {
        // account not found, create new one if autoAccountRole is not zero (0)
        if (sConfig.account.autoAccountRole > 0) {
            std::string ePass, ePassHash;
            sDatabase.DoEscapeString(ePass, ccp.user_password);
            sDatabase.DoEscapeString(ePassHash, ccp.user_password_hash);
            uint32 accountID = CreateNewAccount( eLogin.c_str(), ePass.c_str(), ePassHash.c_str(), sConfig.account.autoAccountRole);
            if ( accountID > 0 ) {
                // add new account successful, get account info
                return GetAccountInformation(ccp, aData, failMsg);
            } else {
                failMsg = "Failed to create a new account.";
                return false;
            }
        } else {
            failMsg = "That account doesn't exist and AutoAccount is disabled.";
            return false;
        }
    }

    aData.name       = eLogin;
    aData.id         = row.GetInt(0);
    aData.clientID   = row.GetInt(1);
    aData.password   = (row.IsNull(2) ? "" : row.GetText(2));
    aData.hash       = (row.IsNull(3) ? "" : row.GetText(3));
    aData.role       = row.GetInt64(4);
    aData.type       = row.GetUInt(5);
    aData.online     = row.GetInt(6) ? true : false;
    aData.banned     = row.GetInt(7) ? true : false;
    aData.visits     = row.GetInt(8);
    aData.last_login = (row.IsNull(9) ? "" : row.GetText(9));

    return true;
}

bool ServiceDB::UpdateAccountHash( const char* username, std::string & hash )
{
    std::string eLogin, eHash;
    sDatabase.DoEscapeString(eLogin, username);
    sDatabase.DoEscapeString(eHash, hash);

    DBerror err;
    if (!sDatabase.RunQuery(err, "UPDATE account SET hash='%s' WHERE accountName='%s'", eHash.c_str(), eLogin.c_str())) {
        sLog.Error( "AccountDB", "Unable to update account information for: %s.", username );
        return false;
    }

    return true;
}

bool ServiceDB::IncrementLoginCount( uint32 accountID )
{
    DBerror err;
    if (!sDatabase.RunQuery(err, "UPDATE account SET lastLogin=now(), logonCount=logonCount+1 WHERE accountID=%u", accountID)) {
        sLog.Error( "AccountDB", "Unable to update account information for accountID %u.", accountID);
        return false;
    }

    return true;
}

uint32 ServiceDB::CreateNewAccount( const char* login, const char* pass, const char* passHash, int64 role )
{
    uint32 accountID(0);
    uint32 clientID(sEntityList.GetClientSeed());

    DBerror err;
    if ( !sDatabase.RunQueryLID( err, accountID,
            "INSERT INTO account ( accountName, password, hash, role, clientID )"
            " VALUES ( '%s', '%s', '%s', %llu, %u )",
                    login, pass, passHash, role, clientID ) )
    {
        sLog.Error( "ServiceDB", "Failed to create a new account '%s':'%s': %s.", login, pass, err.c_str() );
        return 0;
    }

    sDatabase.RunQuery(err, "UPDATE srvStatus SET ClientSeed = ClientSeed + 1 WHERE AI = 1");
    return accountID;
}

void ServiceDB::UpdatePassword(uint32 accountID, const char* pass)
{
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE account SET password = '%s' WHERE accountID=%u", pass, accountID);
}

void ServiceDB::SetServerOnlineStatus(bool online) {
    DBerror err;
    sDatabase.RunQuery(err, "UPDATE srvStatus SET Online = %u, Connections = 0, startTime = %s WHERE AI = 1",
        (online ? 1 : 0), (online ? "UNIX_TIMESTAMP(CURRENT_TIMESTAMP)" : "0"));

    //this is only called on startup/shutdown.  reset all char online counts/status'
    sDatabase.RunQuery(err, "UPDATE chrCharacters SET online = 0 WHERE 1");
    sDatabase.RunQuery(err, "UPDATE account SET online = 0 WHERE 1");
    sDatabase.RunQuery( err, "DELETE FROM chrPausedSkillQueue WHERE 1");
    sDatabase.RunQuery( err, "DELETE FROM staCQOccupancy WHERE 1");
}

void ServiceDB::SetAccountOnlineStatus(uint32 accountID, bool online) {
    DBerror err;
    if (!sDatabase.RunQuery(err, "UPDATE account SET online = %u WHERE accountID= %u ", (online?1:0), accountID)) {
        codelog(DATABASE__ERROR, "Error in query: %s", err.c_str());
    }
}

void ServiceDB::SetAccountBanStatus(uint32 accountID, bool banned) {
    DBerror err;
    if (!sDatabase.RunQuery(err, "UPDATE account SET banned = %u WHERE accountID = %u", (banned?1:0), accountID)) {
        codelog(DATABASE__ERROR, "Error in query: %s", err.c_str());
    }
}

uint32 ServiceDB::GetStationOwner(uint32 stationID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT corporationID FROM staStations WHERE stationID = %u", stationID)) {
        codelog(DATABASE__ERROR, "Failed to query info for station %u: %s.", stationID, res.error.c_str());
        return false;
    }

    DBResultRow row;
    if (res.GetRow(row)) {
        return row.GetInt(0);
    } else {
        return 1;
    }
}

bool ServiceDB::GetConstant(const char *name, uint32 &into)
{
    DBQueryResult res;

    std::string escaped;
    sDatabase.DoEscapeString(escaped, name);

    if (!sDatabase.RunQuery(res, "SELECT constantValue FROM eveConstants WHERE constantID='%s'", escaped.c_str() ))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if (!res.GetRow(row)) {
        _log(DATABASE__MESSAGE, "Unable to find constant %s", name);
        return false;
    }

    into = row.GetUInt(0);

    return true;
}

void ServiceDB::ProcessStringChange(const char* key, const std::string& oldValue, std::string newValue, PyDict* notif, std::vector< std::string >& dbQ)
{
    if (oldValue.compare(newValue) == 0)
        return;
    // add to notification
    PyTuple* val = new PyTuple(2);
    val->items[0] = new PyString(oldValue);
    val->items[1] = new PyString(newValue);
    notif->SetItemString(key, val);

    std::string newEscValue;
    sDatabase.DoEscapeString(newEscValue, newValue);

    std::string qValue = " ";
    qValue += key;
    qValue += " = '";
    qValue += newEscValue.c_str();
    qValue += "' ";
    dbQ.push_back(qValue);
}

void ServiceDB::ProcessRealChange(const char * key, double oldValue, double newValue, PyDict* notif, std::vector<std::string> & dbQ)
{
    if (oldValue == newValue)
        return;
    // add to notification
    PyTuple* val = new PyTuple(2);
    val->items[0] = new PyFloat(oldValue);
    val->items[1] = new PyFloat(newValue);
    notif->SetItemString(key, val);

    int* nullInt(nullptr);
    std::string qValue(key);
    qValue += " = ";
    qValue += fcvt(newValue, 2, nullInt, nullInt);
    dbQ.push_back(qValue);
}

void ServiceDB::ProcessIntChange(const char * key, uint32 oldValue, uint32 newValue, PyDict* notif, std::vector<std::string> & dbQ)
{
    if (oldValue == newValue)
        return;
    // add to notification
    PyTuple* val = new PyTuple(2);
    val->items[0] = new PyInt(oldValue);
    val->items[1] = new PyInt(newValue);
    notif->SetItemString(key, val);

    std::string qValue(key);
    qValue += " = ";
    qValue += std::to_string(newValue);
    dbQ.push_back(qValue);
}

void ServiceDB::ProcessLongChange(const char* key, int64 oldValue, int64 newValue, PyDict* notif, std::vector< std::string >& dbQ)
{
    if (oldValue == newValue)
        return;
    // add to notification
    PyTuple* val = new PyTuple(2);
    val->items[0] = new PyLong(oldValue);
    val->items[1] = new PyLong(newValue);
    notif->SetItemString(key, val);

    std::string qValue(key);
    qValue += " = ";
    qValue += std::to_string(newValue);
    dbQ.push_back(qValue);
}

void ServiceDB::SaveServerStats(double threads, float rss, float vm, float user, float kernel, uint32 items, uint32 bubbles) {
  DBerror err;
  sDatabase.RunQuery(err,
	"UPDATE srvStatus"
	" SET threads = %f,"
	"     rss = %f,"
	"     vm = %f,"
	"     user = %f,"
	"     kernel = %f,"
	"     items = %u,"
    "     bubbles = %u,"
	"     systems = %u,"
    "     npcs = %u,"
    //"     Connections = %u,"
	"     updateTime = UNIX_TIMESTAMP(CURRENT_TIMESTAMP)"
	" WHERE AI = 1",
	    threads, rss, vm, user, kernel, items, bubbles, sEntityList.GetSystemCount(), sEntityList.GetNPCCount()/*, sEntityList.GetConnections()*/);

    _log(DATABASE__INFO, "Server Stats Saved");
}

uint32 ServiceDB::GetOrCreateCQWorldSpace(uint32 stationID) {
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT worldSpaceID FROM staCQInstances "
        "WHERE stationID = %u AND state = 1 "
        "ORDER BY worldSpaceID ASC LIMIT 1",
        stationID))
    {
        _log(DATABASE__ERROR, "GetOrCreateCQWorldSpace: query failed: %s", res.error.c_str());
        return 0;
    }

    DBResultRow row;
    if (res.GetRow(row)) {
        const uint32 ws = row.GetUInt(0);
        sLog.Green("CQ", "[CQ] GetOrCreateCQWorldSpace: station=%u -> existing worldSpaceID=%u", stationID, ws);
        return ws;
    }

    DBerror err;
    uint32 worldSpaceID = 0;
    if (!sDatabase.RunQueryLID(err, worldSpaceID,
        "INSERT INTO staCQInstances (stationID, worldSpaceTypeID, isShared, maxOccupants, state, createdAt, lastActiveAt) "
        "VALUES (%u, 1, 1, 64, 1, UNIX_TIMESTAMP(CURRENT_TIMESTAMP), UNIX_TIMESTAMP(CURRENT_TIMESTAMP))",
        stationID))
    {
        // Another process/thread may have inserted first; re-query active row.
        if (sDatabase.RunQuery(res,
            "SELECT worldSpaceID FROM staCQInstances "
            "WHERE stationID = %u AND state = 1 "
            "ORDER BY worldSpaceID ASC LIMIT 1",
            stationID))
        {
            DBResultRow retryRow;
            if (res.GetRow(retryRow)) {
                return retryRow.GetUInt(0);
            }
        }
        _log(DATABASE__ERROR, "GetOrCreateCQWorldSpace: insert failed and retry-select empty: %s", err.c_str());
        return 0;
    }
    sLog.Green("CQ", "[CQ] GetOrCreateCQWorldSpace: station=%u -> inserted worldSpaceID=%u", stationID, worldSpaceID);
    return worldSpaceID;
}

void ServiceDB::SetCQOccupancy(uint32 worldSpaceID, uint32 characterID, bool inWorldspace) {
    DBerror err;
    if (inWorldspace) {
        sDatabase.RunQuery(err,
            "INSERT INTO staCQOccupancy (worldSpaceID, characterID, occupancyState, joinedAt, lastSeenAt) "
            "VALUES (%u, %u, 1, UNIX_TIMESTAMP(CURRENT_TIMESTAMP), UNIX_TIMESTAMP(CURRENT_TIMESTAMP)) "
            "ON DUPLICATE KEY UPDATE occupancyState = 1, lastSeenAt = UNIX_TIMESTAMP(CURRENT_TIMESTAMP)",
            worldSpaceID, characterID);
        sLog.Green("CQ", "[CQ] SetCQOccupancy: UPSERT char=%u worldSpace=%u (in=1)", characterID, worldSpaceID);
    } else {
        sDatabase.RunQuery(err,
            "DELETE FROM staCQOccupancy WHERE worldSpaceID = %u AND characterID = %u",
            worldSpaceID, characterID);
        sLog.Green("CQ", "[CQ] SetCQOccupancy: DELETE char=%u worldSpace=%u (in=0)", characterID, worldSpaceID);
    }
}

uint32 ServiceDB::GetSceneIDForStation(uint32 stationID) {
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT COALESCE(lsStation.sceneID, lsSystem.sceneID, 0) "
        "FROM staStations st "
        "LEFT JOIN locationScenes lsStation ON lsStation.locationID = st.stationID "
        "LEFT JOIN locationScenes lsSystem ON lsSystem.locationID = st.solarSystemID "
        "WHERE st.stationID = %u "
        "LIMIT 1",
        stationID))
    {
        _log(DATABASE__ERROR, "GetSceneIDForStation: query failed for station %u: %s", stationID, res.error.c_str());
        return 0;
    }

    DBResultRow row;
    if (!res.GetRow(row)) {
        return 0;
    }
    return row.GetUInt(0);
}

// lookupService db calls moved here...made no sense in LSCDB file.
PyRep* ServiceDB::LookupChars(const char *match, bool exact) {
    DBQueryResult res;

    std::string matchEsc;
    sDatabase.DoEscapeString(matchEsc, match);
    if (matchEsc == "__ALL__") {
        if (!sDatabase.RunQuery(res,
            "SELECT "
            "   characterID AS ownerID"
            " FROM chrCharacters"
            " WHERE characterID > %u", maxNPCItem))
        {
            _log(DATABASE__ERROR, "Error in LookupChars query: %s", res.error.c_str());
            return nullptr;
        }
    } else {
        if (!sDatabase.RunQuery(res,
            "SELECT "
            "   itemID AS ownerID"
            " FROM entity"
            " WHERE itemName %s '%s'",
            exact?"=":"LIKE", matchEsc.c_str()
        ))
        {
            _log(DATABASE__ERROR, "Error in LookupChars query: %s", res.error.c_str());
            return nullptr;
        }
    }

    return DBResultToRowset(res);
}


PyRep* ServiceDB::LookupOwners(const char *match, bool exact) {
    DBQueryResult res;

    std::string matchEsc;
    sDatabase.DoEscapeString(matchEsc, match);

    // so each row needs "ownerID", "ownerName", and "groupID"
    // ownerID      =  itemID
    // ownerName    =  name
    // groupID      = 1 for character, 2 for corporation, 32 for alliance

    sDatabase.RunQuery(res,
        "SELECT"
        "  characterID AS ownerID,"
        "  characterName AS ownerName,"
        "  1 AS groupID"
        " FROM chrCharacters"
        " WHERE characterName %s '%s'", (exact?"=":"LIKE"), matchEsc.c_str());

    sDatabase.RunQuery(res,
        "SELECT"
        "  corporationID AS ownerID,"
        "  corporationName AS ownerName,"
        "  2 AS groupID"
        " FROM crpCorporation"
        " WHERE corporationName %s '%s'", (exact?"=":"LIKE"), matchEsc.c_str());

    sDatabase.RunQuery(res,
        "SELECT"
        "  corporationID AS ownerID,"
        "  corporationName AS ownerName,"
        "  2 AS groupID"
        " FROM crpCorporation"
        " WHERE tickerName %s '%s'", (exact?"=":"LIKE"), matchEsc.c_str());

    sDatabase.RunQuery(res,
        "SELECT"
        "  allianceID AS ownerID,"
        "  allianceName AS ownerName,"
        "  32 AS groupID"
        " FROM alnAlliance"
        " WHERE allianceName %s '%s'", (exact?"=":"LIKE"), matchEsc.c_str());

    sDatabase.RunQuery(res,
        "SELECT"
        "  allianceID AS ownerID,"
        "  shortName AS ownerName,"
        "  32 AS groupID"
        " FROM alnAlliance"
        " WHERE shortName %s '%s'", (exact?"=":"LIKE"), matchEsc.c_str());

    return DBResultToRowset(res);
}

PyRep* ServiceDB::LookupCorporations(const std::string & search) {
    DBQueryResult res;
    std::string secure;
    sDatabase.DoEscapeString(secure, search);

    if (!sDatabase.RunQuery(res,
        "SELECT"
        "   corporationID, corporationName, corporationType "
        " FROM crpCorporation "
        " WHERE corporationName LIKE '%s'", secure.c_str()))
    {
        _log(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return 0;
    }

    return DBResultToRowset(res);
}


PyRep* ServiceDB::LookupFactions(const std::string & search) {
    DBQueryResult res;
    std::string secure;
    sDatabase.DoEscapeString(secure, search);

    if (!sDatabase.RunQuery(res,
        "SELECT"
        "   factionID, factionName "
        " FROM facFactions "
        " WHERE factionName LIKE '%s'", secure.c_str()))
    {
        _log(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return 0;
    }

    return DBResultToRowset(res);
}


PyRep* ServiceDB::LookupCorporationTickers(const std::string & search) {
    DBQueryResult res;
    std::string secure;
    sDatabase.DoEscapeString(secure, search);

    if (!sDatabase.RunQuery(res,
        "SELECT"
        "   corporationID, corporationName, tickerName "
        " FROM crpCorporation "
        " WHERE tickerName LIKE '%s'", secure.c_str()))
    {
        _log(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return 0;
    }

    return DBResultToRowset(res);
}


PyRep* ServiceDB::LookupStations(const std::string & search) {
    DBQueryResult res;
    std::string secure;
    sDatabase.DoEscapeString(secure, search);

    if (!sDatabase.RunQuery(res,
        "SELECT"
        "   stationID, stationName, stationTypeID "
        " FROM staStations "
        " WHERE stationName LIKE '%s'", secure.c_str()))
    {
        _log(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return 0;
    }

    return DBResultToRowset(res);
}

PyRep* ServiceDB::LookupKnownLocationsByGroup(const std::string & search, uint32 groupID) {
    /* Client passes invGroups-style location group (e.g. Solar_System = 5), not a bool "exact"
     * and not an inventory typeID. Match names by prefix (client: "beginning of its name"). */
    DBQueryResult res;
    if (search.empty()) {
        if (!sDatabase.RunQuery(res,
                "SELECT solarSystemID AS itemID, solarSystemName AS itemName, %u AS typeID "
                " FROM mapSolarSystems WHERE 1=0",
                (unsigned)EVEDB::invGroups::Solar_System))
            return nullptr;
        return DBResultToRowset(res);
    }

    std::string secure;
    sDatabase.DoEscapeString(secure, search);

    bool ok = false;
    switch (groupID) {
        case EVEDB::invGroups::Region:
            ok = sDatabase.RunQuery(res,
                "SELECT"
                "   regionID AS itemID, regionName AS itemName, %u AS typeID "
                " FROM mapRegions"
                " WHERE regionName LIKE '%s%%'"
                " LIMIT 50",
                (unsigned)EVEDB::invGroups::Region, secure.c_str());
            break;
        case EVEDB::invGroups::Constellation:
            ok = sDatabase.RunQuery(res,
                "SELECT"
                "   constellationID AS itemID, constellationName AS itemName, %u AS typeID "
                " FROM mapConstellations"
                " WHERE constellationName LIKE '%s%%'"
                " LIMIT 50",
                (unsigned)EVEDB::invGroups::Constellation, secure.c_str());
            break;
        case EVEDB::invGroups::Solar_System:
            ok = sDatabase.RunQuery(res,
                "SELECT"
                "   solarSystemID AS itemID, solarSystemName AS itemName, %u AS typeID "
                " FROM mapSolarSystems"
                " WHERE solarSystemName LIKE '%s%%'"
                " LIMIT 50",
                (unsigned)EVEDB::invGroups::Solar_System, secure.c_str());
            break;
        case EVEDB::invGroups::Station:
            ok = sDatabase.RunQuery(res,
                "SELECT"
                "   stationID AS itemID, stationName AS itemName, stationTypeID AS typeID "
                " FROM staStations"
                " WHERE stationName LIKE '%s%%'"
                " LIMIT 50",
                secure.c_str());
            break;
        default:
            _log(DATA__WARNING, "LookupKnownLocationsByGroup: unsupported groupID %u", groupID);
            ok = sDatabase.RunQuery(res,
                "SELECT"
                "   solarSystemID AS itemID, solarSystemName AS itemName, %u AS typeID "
                " FROM mapSolarSystems WHERE 1=0",
                (unsigned)EVEDB::invGroups::Solar_System);
            break;
    }

    if (!ok) {
        _log(DATABASE__ERROR, "LookupKnownLocationsByGroup failed: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

/** @todo look into this...may be wrong */
PyRep* ServiceDB::PrimeOwners(std::vector< int32 >& itemIDs)
{
    DBQueryResult res;
    DBResultRow row;
    PyDict* dict = new PyDict();
    for (auto cur : itemIDs) {
        if (IsCharacterID(cur)) {
            sDatabase.RunQuery(res, "SELECT characterID, characterName, typeID FROM chrCharacters WHERE characterID = %u", cur);
        } else if (IsPlayerCorp(cur)) {
            sDatabase.RunQuery(res, "SELECT corporationID, corporationName, typeID FROM crpCorporation WHERE corporationID = %u", cur);
        } else if (IsAlliance(cur)) {
            sDatabase.RunQuery(res, "SELECT allianceID, allianceName, typeID FROM alnAlliance WHERE allianceID = %u", cur);
        } else {
            ; // make error
        }
        if (res.GetRow(row)) {
            PyList* list = new PyList();
                list->AddItem(new PyInt(row.GetInt(0)));
                list->AddItem(new PyString(row.GetText(1)));
                list->AddItem(new PyInt(row.GetInt(2)));
            dict->SetItem(new PyInt(row.GetInt(0)), list);
        }
    }

    return dict;
}

void ServiceDB::GetCorpHangarNames(uint32 corpID, std::map<uint8, std::string> &hangarNames) {
    std::string table = "crpWalletDivisons";
    if (IsNPCCorp(corpID))
        table = "crpNPCWalletDivisons";
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        " SELECT division1,division2,division3,division4,division5,division6,division7"
        " FROM %s"
        " WHERE corporationID = %u", table.c_str(), corpID))
    {
        codelog(CORP__DB_ERROR, "Error in retrieving corporation's data (%u)", corpID);
        return;
    }

    DBResultRow row;
    if (res.GetRow(row)) {
        hangarNames.emplace(flagHangar, row.GetText(0));
        hangarNames.emplace(flagCorpHangar2, row.GetText(1));
        hangarNames.emplace(flagCorpHangar3, row.GetText(2));
        hangarNames.emplace(flagCorpHangar4, row.GetText(3));
        hangarNames.emplace(flagCorpHangar5, row.GetText(4));
        hangarNames.emplace(flagCorpHangar6, row.GetText(5));
        hangarNames.emplace(flagCorpHangar7, row.GetText(6));
    } else {
        _log(CORP__DB_ERROR, "CorpID %u has no division data.", corpID);
    }
}

uint32 ServiceDB::CQInstanceCharIDFromAgentID(uint32 agentID) {
    return 0xC0000000u + (agentID & 0x0FFFFFFFu);
}

bool ServiceDB::CQAgentIDFromInstanceCharID(uint32 instanceCharID, uint32& outAgentID) {
    if ((instanceCharID & 0xF0000000u) != 0xC0000000u) {
        return false;
    }
    outAgentID = instanceCharID - 0xC0000000u;
    return outAgentID != 0; // agentID 0 is invalid
}

void ServiceDB::GetCQCustomAgentsForStation(uint32 stationID, std::vector<CQCustomAgentRow>& into) {
    into.clear();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT ca.agentID, ca.missionAgentID, ca.appearanceCharID, ca.posX, ca.posY, ca.posZ, ca.yaw,"
        "  COALESCE(agt.corporationID, chapp.corporationID, 0),"
        "  IF(ca.appearanceCharID > 0 AND ca.missionAgentID > 0 AND ca.appearanceCharID != ca.missionAgentID,"
        "     chapp.gender, COALESCE(chNPC.gender, chapp.gender, 0)),"
        "  IF(ca.appearanceCharID > 0 AND ca.missionAgentID > 0 AND ca.appearanceCharID != ca.missionAgentID,"
        "     chapp.bloodlineID, COALESCE(bl.bloodlineID, chapp.bloodlineID, 0)),"
        "  IF(ca.appearanceCharID > 0 AND ca.missionAgentID > 0 AND ca.appearanceCharID != ca.missionAgentID,"
        "     chapp.raceID, COALESCE(chapp.raceID, 0))"
        " FROM staCQCustomAgents ca"
        "  LEFT JOIN agtAgents agt ON agt.agentID = ca.missionAgentID"
        "  LEFT JOIN chrNPCCharacters chNPC ON chNPC.characterID = ca.missionAgentID"
        "  LEFT JOIN bloodlineTypes bl ON bl.typeID = chNPC.typeID"
        "  LEFT JOIN chrCharacters chapp ON chapp.characterID = ca.appearanceCharID"
        " WHERE ca.stationID = %u AND ca.enabled = 1"
        " ORDER BY ca.agentID ASC",
        stationID))
    {
        _log(DATABASE__ERROR, "GetCQCustomAgentsForStation: %s", res.error.c_str());
        return;
    }
    DBResultRow row;
    while (res.GetRow(row)) {
        CQCustomAgentRow r;
        r.agentID = row.GetUInt(0);
        r.missionAgentID = row.IsNull(1) ? 0 : row.GetUInt(1);
        r.appearanceCharID = row.GetUInt(2);
        r.posX = row.GetDouble(3);
        r.posY = row.GetDouble(4);
        r.posZ = row.GetDouble(5);
        r.yaw = static_cast<float>(row.GetDouble(6));
        r.corporationID = row.GetUInt(7);
        r.genderID = static_cast<uint8>(row.GetUInt(8));
        r.bloodlineID = row.GetUInt(9);
        r.raceID = row.GetUInt(10);
        r.instanceCharID = CQInstanceCharIDFromAgentID(r.agentID);
        into.push_back(r);
    }
}

bool ServiceDB::EnsureRandomMissionAgentPaperDollInDb(uint32 missionAgentCharID)
{
    DBQueryResult res;
    DBResultRow row;

    if (!sDatabase.RunQuery(res,
        "SELECT 1 FROM agtAgents WHERE agentID=%u AND isLocator=0 AND agentTypeID=%u LIMIT 1",
        missionAgentCharID, static_cast<unsigned>(Agents::Type::Basic)))
    {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: agent check: %s", res.error.c_str());
        return false;
    }
    if (!res.GetRow(row)) {
        return false;
    }

    if (!sDatabase.RunQuery(res,
        "SELECT 1 FROM avatar_modifiers WHERE charID=%u LIMIT 1",
        missionAgentCharID))
    {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: modifiers check: %s", res.error.c_str());
        return false;
    }
    if (res.GetRow(row)) {
        return false;
    }

    uint32 tpl = 0;
    if (!sDatabase.RunQuery(res,
        "SELECT c.characterID FROM chrCharacters AS c "
        "INNER JOIN avatars AS a ON a.charID = c.characterID "
        "WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID) "
        "AND c.characterID <> %u "
        "AND c.gender = (SELECT n.gender FROM chrNPCCharacters AS n WHERE n.characterID = %u LIMIT 1) "
        "ORDER BY RAND() LIMIT 1",
        missionAgentCharID, missionAgentCharID))
    {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: template (gender): %s", res.error.c_str());
        return false;
    }
    if (!res.GetRow(row)) {
        if (!sDatabase.RunQuery(res,
            "SELECT c.characterID FROM chrCharacters AS c "
            "INNER JOIN avatars AS a ON a.charID = c.characterID "
            "WHERE EXISTS (SELECT 1 FROM avatar_modifiers AS m WHERE m.charID = c.characterID) "
            "AND c.characterID <> %u "
            "ORDER BY RAND() LIMIT 1",
            missionAgentCharID))
        {
            _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: template (any): %s", res.error.c_str());
            return false;
        }
        if (!res.GetRow(row)) {
            _log(DATABASE__MESSAGE, "EnsureRandomMissionAgentPaperDollInDb: no dressed chrCharacters template for agent %u",
                missionAgentCharID);
            return false;
        }
    }
    tpl = row.GetUInt(0);

    DBerror err;
    uint32 affected = 0;
    if (!sDatabase.RunQuery(err, affected, "DELETE FROM avatar_colors WHERE charID=%u", missionAgentCharID)) {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: %s", err.c_str());
        return false;
    }
    if (!sDatabase.RunQuery(err, affected, "DELETE FROM avatar_modifiers WHERE charID=%u", missionAgentCharID)) {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: %s", err.c_str());
        return false;
    }
    if (!sDatabase.RunQuery(err, affected, "DELETE FROM avatar_sculpts WHERE charID=%u", missionAgentCharID)) {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: %s", err.c_str());
        return false;
    }
    if (!sDatabase.RunQuery(err, affected, "DELETE FROM chrPortraitData WHERE charID=%u", missionAgentCharID)) {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: %s", err.c_str());
        return false;
    }
    if (!sDatabase.RunQuery(err, affected, "DELETE FROM avatars WHERE charID=%u", missionAgentCharID)) {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: %s", err.c_str());
        return false;
    }

    if (!sDatabase.RunQuery(err, affected,
        "INSERT INTO avatars (charID, hairDarkness) SELECT %u, t.hairDarkness FROM avatars t WHERE t.charID=%u LIMIT 1",
        missionAgentCharID, tpl))
    {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: insert avatars: %s", err.c_str());
        return false;
    }
    if (affected == 0) {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: insert avatars affected 0 (tpl=%u)", tpl);
        return false;
    }

    if (!sDatabase.RunQuery(err, affected,
        "INSERT INTO avatar_colors (charID, colorID, colorNameA, colorNameBC, weight, gloss) "
        "SELECT %u, c.colorID, c.colorNameA, c.colorNameBC, c.weight, c.gloss FROM avatar_colors c WHERE c.charID=%u",
        missionAgentCharID, tpl))
    {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: insert avatar_colors: %s", err.c_str());
        return false;
    }
    if (!sDatabase.RunQuery(err, affected,
        "INSERT INTO avatar_modifiers (charID, modifierLocationID, paperdollResourceID, paperdollResourceVariation) "
        "SELECT %u, m.modifierLocationID, m.paperdollResourceID, m.paperdollResourceVariation "
        "FROM avatar_modifiers m WHERE m.charID=%u",
        missionAgentCharID, tpl))
    {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: insert avatar_modifiers: %s", err.c_str());
        return false;
    }
    if (!sDatabase.RunQuery(err, affected,
        "INSERT INTO avatar_sculpts (charID, sculptLocationID, weightUpDown, weightLeftRight, weightForwardBack) "
        "SELECT %u, s.sculptLocationID, s.weightUpDown, s.weightLeftRight, s.weightForwardBack "
        "FROM avatar_sculpts s WHERE s.charID=%u",
        missionAgentCharID, tpl))
    {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: insert avatar_sculpts: %s", err.c_str());
        return false;
    }

    if (!sDatabase.RunQuery(err, affected,
        "INSERT INTO chrPortraitData ("
        " charID, backgroundID, lightID, lightColorID, cameraX, cameraY, cameraZ,"
        " cameraPoiX, cameraPoiY, cameraPoiZ, headLookTargetX, headLookTargetY, headLookTargetZ,"
        " lightIntensity, headTilt, orientChar, browLeftCurl, browLeftTighten, browLeftUpDown,"
        " browRightCurl, browRightTighten, browRightUpDown, eyeClose, eyesLookVertical, eyesLookHorizontal,"
        " squintLeft, squintRight, jawSideways, jawUp, puckerLips, frownLeft, frownRight,"
        " smileLeft, smileRight, cameraFieldOfView, portraitPoseNumber)"
        " SELECT %u,"
        " p.backgroundID, p.lightID, p.lightColorID, p.cameraX, p.cameraY, p.cameraZ,"
        " p.cameraPoiX, p.cameraPoiY, p.cameraPoiZ, p.headLookTargetX, p.headLookTargetY, p.headLookTargetZ,"
        " p.lightIntensity, p.headTilt, p.orientChar, p.browLeftCurl, p.browLeftTighten, p.browLeftUpDown,"
        " p.browRightCurl, p.browRightTighten, p.browRightUpDown, p.eyeClose, p.eyesLookVertical, p.eyesLookHorizontal,"
        " p.squintLeft, p.squintRight, p.jawSideways, p.jawUp, p.puckerLips, p.frownLeft, p.frownRight,"
        " p.smileLeft, p.smileRight, p.cameraFieldOfView, p.portraitPoseNumber"
        " FROM chrPortraitData p WHERE p.charID=%u",
        missionAgentCharID, tpl))
    {
        _log(DATABASE__ERROR, "EnsureRandomMissionAgentPaperDollInDb: insert chrPortraitData: %s", err.c_str());
        return false;
    }

    _log(DATABASE__MESSAGE, "CQ: persisted random paper doll for mission agent %u from template %u", missionAgentCharID, tpl);
    return true;
}

void ServiceDB::EnsureCQMissionAgentSpawnsForStation(uint32 stationID, bool* outLayoutChanged)
{
    if (outLayoutChanged != nullptr) {
        *outLayoutChanged = false;
    }

    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT agt.agentID, COALESCE(chr.characterName, CONCAT('Agent ', agt.agentID))"
        " FROM agtAgents AS agt"
        " INNER JOIN chrNPCCharacters AS chr ON chr.characterID = agt.agentID"
        " INNER JOIN staStations AS st ON st.stationID = agt.locationID"
        " WHERE agt.locationID = %u AND agt.isLocator = 0 AND agt.agentTypeID = %u"
        " ORDER BY agt.agentID ASC",
        stationID, static_cast<unsigned>(Agents::Type::Basic)))
    {
        _log(DATABASE__ERROR, "EnsureCQMissionAgentSpawnsForStation: %s", res.error.c_str());
        return;
    }

    std::vector<std::pair<uint32, std::string>> agents;
    DBResultRow row;
    while (res.GetRow(row)) {
        agents.emplace_back(row.GetUInt(0), row.GetText(1));
    }

    const unsigned N = static_cast<unsigned>(agents.size());
    if (N == 0) {
        return;
    }

    for (unsigned i = 0; i < N; ++i) {
        const uint32 aid = agents[i].first;
        std::string name = agents[i].second;
        if (name.empty()) {
            name = "Agent " + std::to_string(aid);
        }

        DBQueryResult resExist;
        if (!sDatabase.RunQuery(resExist,
            "SELECT agentID FROM staCQCustomAgents WHERE stationID=%u AND missionAgentID=%u AND enabled=1 LIMIT 1",
            stationID, aid))
        {
            _log(DATABASE__ERROR, "EnsureCQMissionAgentSpawnsForStation EXISTS: %s", resExist.error.c_str());
            continue;
        }
        DBResultRow existRow;
        if (resExist.GetRow(existRow)) {
            continue;
        }

        double px, py, pz;
        float pyaw;
        CQMissionAgentLayoutSlot(i, N, px, py, pz, pyaw);

        std::string nameEsc;
        sDatabase.DoEscapeString(nameEsc, name.c_str());

        DBerror err;
        uint32 rowLid = 0;
        if (!sDatabase.RunQueryLID(err, rowLid,
            "INSERT INTO staCQCustomAgents (stationID, missionAgentID, appearanceCharID, posX, posY, posZ, yaw, label, enabled, createdAt, updatedAt)"
            " VALUES (%u, %u, 0, %f, %f, %f, %f, '%s', 1, UNIX_TIMESTAMP(CURRENT_TIMESTAMP), UNIX_TIMESTAMP(CURRENT_TIMESTAMP))",
            stationID, aid, px, py, pz, static_cast<double>(pyaw), nameEsc.c_str()))
        {
            _log(DATABASE__ERROR, "EnsureCQMissionAgentSpawnsForStation INSERT: %s", err.c_str());
            continue;
        }
        if (outLayoutChanged != nullptr) {
            *outLayoutChanged = true;
        }
        (void)rowLid;
    }

    DBQueryResult resPos;
    if (!sDatabase.RunQuery(resPos,
        "SELECT missionAgentID, posX, posY, posZ, yaw FROM staCQCustomAgents"
        " WHERE stationID=%u AND missionAgentID IS NOT NULL AND enabled=1",
        stationID))
    {
        _log(DATABASE__ERROR, "EnsureCQMissionAgentSpawnsForStation positions: %s", resPos.error.c_str());
        return;
    }

    struct CurTransform {
        double x, y, z;
        float yaw;
    };
    std::map<uint32, CurTransform> curByMission;
    while (resPos.GetRow(row)) {
        CurTransform ct;
        ct.x = row.GetDouble(1);
        ct.y = row.GetDouble(2);
        ct.z = row.GetDouble(3);
        ct.yaw = static_cast<float>(row.GetDouble(4));
        curByMission[row.GetUInt(0)] = ct;
    }

    for (unsigned i = 0; i < N; ++i) {
        const uint32 aid = agents[i].first;
        double px, py, pz;
        float pyaw;
        CQMissionAgentLayoutSlot(i, N, px, py, pz, pyaw);

        auto it = curByMission.find(aid);
        if (it == curByMission.end()) {
            continue;
        }
        const CurTransform& c = it->second;
        if (CQMissionAgentLayoutClose(c.x, px) && CQMissionAgentLayoutClose(c.y, py)
            && CQMissionAgentLayoutClose(c.z, pz) && CQMissionAgentYawClose(c.yaw, pyaw)) {
            continue;
        }

        DBerror err;
        uint32 affected = 0;
        if (!sDatabase.RunQuery(err, affected,
            "UPDATE staCQCustomAgents SET posX=%f, posY=%f, posZ=%f, yaw=%f, updatedAt=UNIX_TIMESTAMP(CURRENT_TIMESTAMP)"
            " WHERE stationID=%u AND missionAgentID=%u AND enabled=1",
            px, py, pz, static_cast<double>(pyaw), stationID, aid))
        {
            _log(DATABASE__ERROR, "EnsureCQMissionAgentSpawnsForStation reposition: %s", err.c_str());
            continue;
        }
        if (affected > 0 && outLayoutChanged != nullptr) {
            *outLayoutChanged = true;
        }
    }

    for (unsigned i = 0; i < N; ++i) {
        if (EnsureRandomMissionAgentPaperDollInDb(agents[i].first) && outLayoutChanged != nullptr) {
            *outLayoutChanged = true;
        }
    }
}

bool ServiceDB::InsertCQCustomAgent(uint32 stationID, uint32 appearanceCharID, double x, double y, double z, float yaw, const std::string& label, uint32& outInstanceCharID) {
    outInstanceCharID = 0;
    std::string labelEsc;
    sDatabase.DoEscapeString(labelEsc, label.c_str());
    DBerror err;
    uint32 rowLid = 0;
    if (!sDatabase.RunQueryLID(err, rowLid,
        "INSERT INTO staCQCustomAgents (stationID, appearanceCharID, posX, posY, posZ, yaw, label, enabled, createdAt, updatedAt)"
        " VALUES (%u, %u, %f, %f, %f, %f, '%s', 1, UNIX_TIMESTAMP(CURRENT_TIMESTAMP), UNIX_TIMESTAMP(CURRENT_TIMESTAMP))",
        stationID, appearanceCharID, x, y, z, static_cast<double>(yaw), labelEsc.c_str())) {
        _log(DATABASE__ERROR, "InsertCQCustomAgent: %s", err.c_str());
        return false;
    }
    outInstanceCharID = CQInstanceCharIDFromAgentID(rowLid);
    return true;
}

bool ServiceDB::UpdateCQCustomAgentTransformByInstance(uint32 instanceCharID, double x, double y, double z, float yaw) {
    uint32 agentID = 0;
    if (!GetCQTableAgentIdFromProtocolId(instanceCharID, agentID)) {
        return false;
    }
    DBerror err;
    uint32 affected = 0;
    if (!sDatabase.RunQuery(err, affected,
        "UPDATE staCQCustomAgents SET posX=%f, posY=%f, posZ=%f, yaw=%f, updatedAt=UNIX_TIMESTAMP(CURRENT_TIMESTAMP) WHERE agentID=%u",
        x, y, z, static_cast<double>(yaw), agentID)) {
        _log(DATABASE__ERROR, "UpdateCQCustomAgentTransformByInstance: %s", err.c_str());
        return false;
    }
    return affected > 0;
}

bool ServiceDB::DeleteCQCustomAgentByInstance(uint32 instanceCharID) {
    uint32 agentID = 0;
    if (!GetCQTableAgentIdFromProtocolId(instanceCharID, agentID)) {
        return false;
    }
    DBerror err;
    uint32 affected = 0;
    if (!sDatabase.RunQuery(err, affected, "DELETE FROM staCQCustomAgents WHERE agentID=%u", agentID)) {
        _log(DATABASE__ERROR, "DeleteCQCustomAgentByInstance: %s", err.c_str());
        return false;
    }
    return affected > 0;
}

bool ServiceDB::GetCQTableAgentIdFromProtocolId(uint32 protocolCharId, uint32& outStaAgentId) {
    outStaAgentId = 0;
    uint32 internal = 0;
    if (CQAgentIDFromInstanceCharID(protocolCharId, internal)) {
        outStaAgentId = internal;
        return true;
    }
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT agentID FROM staCQCustomAgents WHERE missionAgentID = %u AND enabled = 1 LIMIT 1",
        protocolCharId))
    {
        _log(DATABASE__ERROR, "GetCQTableAgentIdFromProtocolId: %s", res.error.c_str());
        return false;
    }
    DBResultRow row;
    if (!res.GetRow(row)) {
        return false;
    }
    outStaAgentId = row.GetUInt(0);
    return true;
}

bool ServiceDB::GetCQCustomAgentByTableId(uint32 staAgentId, CQCustomAgentRow& out) {
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT ca.agentID, ca.missionAgentID, ca.appearanceCharID, ca.posX, ca.posY, ca.posZ, ca.yaw,"
        "  COALESCE(agt.corporationID, chapp.corporationID, 0),"
        "  IF(ca.appearanceCharID > 0 AND ca.missionAgentID > 0 AND ca.appearanceCharID != ca.missionAgentID,"
        "     chapp.gender, COALESCE(chNPC.gender, chapp.gender, 0)),"
        "  IF(ca.appearanceCharID > 0 AND ca.missionAgentID > 0 AND ca.appearanceCharID != ca.missionAgentID,"
        "     chapp.bloodlineID, COALESCE(bl.bloodlineID, chapp.bloodlineID, 0)),"
        "  IF(ca.appearanceCharID > 0 AND ca.missionAgentID > 0 AND ca.appearanceCharID != ca.missionAgentID,"
        "     chapp.raceID, COALESCE(chapp.raceID, 0))"
        " FROM staCQCustomAgents ca"
        "  LEFT JOIN agtAgents agt ON agt.agentID = ca.missionAgentID"
        "  LEFT JOIN chrNPCCharacters chNPC ON chNPC.characterID = ca.missionAgentID"
        "  LEFT JOIN bloodlineTypes bl ON bl.typeID = chNPC.typeID"
        "  LEFT JOIN chrCharacters chapp ON chapp.characterID = ca.appearanceCharID"
        " WHERE ca.agentID = %u AND ca.enabled = 1",
        staAgentId))
    {
        _log(DATABASE__ERROR, "GetCQCustomAgentByTableId: %s", res.error.c_str());
        return false;
    }
    DBResultRow row;
    if (!res.GetRow(row)) {
        return false;
    }
    out.agentID = row.GetUInt(0);
    out.missionAgentID = row.IsNull(1) ? 0 : row.GetUInt(1);
    out.appearanceCharID = row.GetUInt(2);
    out.posX = row.GetDouble(3);
    out.posY = row.GetDouble(4);
    out.posZ = row.GetDouble(5);
    out.yaw = static_cast<float>(row.GetDouble(6));
    out.corporationID = row.GetUInt(7);
    out.genderID = static_cast<uint8>(row.GetUInt(8));
    out.bloodlineID = row.GetUInt(9);
    out.raceID = row.GetUInt(10);
    out.instanceCharID = CQInstanceCharIDFromAgentID(out.agentID);
    return true;
}

bool ServiceDB::GetCQCustomAgentByInstance(uint32 instanceCharID, CQCustomAgentRow& out) {
    uint32 agentID = 0;
    if (!CQAgentIDFromInstanceCharID(instanceCharID, agentID)) {
        return false;
    }
    return GetCQCustomAgentByTableId(agentID, out);
}

bool ServiceDB::GetCQCustomAgentStationID(uint32 instanceCharID, uint32& outStationID) {
    outStationID = 0;
    uint32 agentID = 0;
    if (!GetCQTableAgentIdFromProtocolId(instanceCharID, agentID)) {
        return false;
    }
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT stationID FROM staCQCustomAgents WHERE agentID=%u", agentID)) {
        _log(DATABASE__ERROR, "GetCQCustomAgentStationID: %s", res.error.c_str());
        return false;
    }
    DBResultRow row;
    if (!res.GetRow(row)) {
        return false;
    }
    outStationID = row.GetUInt(0);
    return true;
}

void ServiceDB::SaveKillOrLoss(KillData &data) {
    /* Negative alliance IDs must not reach the client — Crucible resolves kill rows via EveOwners (RecordNotFound on -1). */
    const int32 victimAlliance = (data.victimAllianceID < 0 ? 0 : data.victimAllianceID);
    const uint32 finalAlliance = (data.finalAllianceID < 0 ? 0u : (uint32)data.finalAllianceID);
    const uint32 victimFaction = (data.victimFactionID < 0 ? 0u : (uint32)data.victimFactionID);
    const uint32 finalFaction = (data.finalFactionID < 0 ? 0u : (uint32)data.finalFactionID);

    std::string blobEsc;
    sDatabase.DoEscapeString(blobEsc, data.killBlob);

    DBerror err;
    if (!sDatabase.RunQuery(err,
            " INSERT INTO chrKillTable (solarSystemID, victimCharacterID, victimCorporationID, victimAllianceID, victimFactionID,"
            "victimShipTypeID, victimDamageTaken, finalCharacterID, finalCorporationID, finalAllianceID, finalFactionID, finalShipTypeID,"
            "finalWeaponTypeID, finalSecurityStatus, finalDamageDone, killBlob, killTime, moonID)"
            " VALUES (%u,%u,%u,%i,%u,%u,%u,%u,%u,%u,%u,%u,%u,%f,%u,'%s',%lli,%u)",
                data.solarSystemID, data.victimCharacterID, data.victimCorporationID,
                victimAlliance, victimFaction, data.victimShipTypeID, data.victimDamageTaken,
                data.finalCharacterID, data.finalCorporationID, finalAlliance, finalFaction,
                data.finalShipTypeID, data.finalWeaponTypeID, data.finalSecurityStatus, data.finalDamageDone,
                blobEsc.c_str(), data.killTime, data.moonID)) {
        codelog(DATABASE__ERROR, "SaveKillOrLoss failed: %s", err.c_str());
    }
}
