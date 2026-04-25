
/**
 * @name WorldSpaceServer.cpp
 *   Specific Class for
 *
 * @Author:         Allan
 * @date:   31August17
 */


#include "eve-server.h"

#include <cstdio>

#include "ServiceDB.h"
#include "system/WorldSpaceServer.h"
#include "system/CQManager.h"

WorldSpaceServer::WorldSpaceServer() :
    Service("worldSpaceServer")
{
    this->Add("GetWorldSpaceTypeIDFromWorldSpaceID", &WorldSpaceServer::GetWorldSpaceTypeIDFromWorldSpaceID);
    this->Add("GetWorldSpaceMachoAddress", &WorldSpaceServer::GetWorldSpaceMachoAddress);

    /*
        ws = world.GetWorldSpace(worldSpaceTypeID)
        return ws.GetDistrictID()


        currentRevs = sm.GetService('jessicaWorldSpaceClient').GetWorldSpace(self.id).GetWorldSpaceSpawnRevisionsList()
        */
}

PyResult WorldSpaceServer::GetWorldSpaceTypeIDFromWorldSpaceID(PyCallArgs &call, PyInt* worldSpaceID) {
    std::fprintf(stderr, "[CQ] worldSpaceServer.GetWorldSpaceTypeIDFromWorldSpaceID worldSpaceID=%u\n",
        worldSpaceID->value());
    std::fflush(stderr);
    /**
     *        worldSpaceTypeID = self.GetWorldSpaceTypeIDFromWorldSpaceID(worldSpaceID)
     */
    sLog.Green("CQ", "[CQ] worldSpaceServer.GetWorldSpaceTypeIDFromWorldSpaceID: worldSpaceID=%u (tuple size=%lu)",
        worldSpaceID->value(), call.tuple->size());
    call.Dump(SERVICE__CALL_DUMP);

    if (worldSpaceID->value() == 0) {
        return new PyInt(0);
    }

    uint32 stationID = 0;
    if (IsStationID(worldSpaceID->value())) {
        stationID = worldSpaceID->value();
    } else if (sCQMgr.IsInWorldSpace(worldSpaceID->value())) {
        stationID = sCQMgr.GetStationForWorldSpace(worldSpaceID->value());
    }

    if (stationID == 0) {
        return new PyInt(0);
    }

    const uint32 sceneID = ServiceDB::GetSceneIDForStation(stationID);
    if (sceneID == 0) {
        sLog.Error("CQ", "[CQ] worldSpaceServer.GetWorldSpaceTypeIDFromWorldSpaceID: no sceneID mapping for station=%u worldSpaceID=%u",
            stationID, worldSpaceID->value());
        return new PyInt(0);
    }

    sLog.Green("CQ", "[CQ] worldSpaceServer.GetWorldSpaceTypeIDFromWorldSpaceID: worldSpaceID=%u station=%u sceneID=%u",
        worldSpaceID->value(), stationID, sceneID);
    return new PyInt(sceneID);
}

PyResult WorldSpaceServer::GetWorldSpaceMachoAddress(PyCallArgs &call, PyString* address) {
    std::fprintf(stderr, "[CQ] worldSpaceServer.GetWorldSpaceMachoAddress address=\"%s\"\n",
        address->content().c_str());
    std::fflush(stderr);
    /**
     *       service, address = wss.GetWorldSpaceMachoAddress(address)
     */
    sLog.Green("CQ", "[CQ] worldSpaceServer.GetWorldSpaceMachoAddress: address=\"%s\" (tuple size=%lu)",
        address->content().c_str(), call.tuple->size());
    call.Dump(SERVICE__CALL_DUMP);

    PyTuple* rsp = new PyTuple(2);
    rsp->SetItem(0, new PyString("captainsQuartersSvc"));
    rsp->SetItem(1, new PyString(address->content()));
    return rsp;
}

