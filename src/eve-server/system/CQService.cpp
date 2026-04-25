#include "eve-server.h"

#include <cstdio>

#include "services/ServiceManager.h"
#include "system/CQManager.h"
#include "system/CQService.h"

CQService::CQService(EVEServiceManager& mgr)
: BindableService("captainsQuartersSvc", mgr, eAccessLevel_Location) {}

BoundDispatcher* CQService::BindObject(Client* client, PyRep* bindParameters) {
    std::fprintf(stderr, "[CQ] captainsQuartersSvc.BindObject attempt char=%u station=%u\n",
        client != nullptr ? client->GetCharacterID() : 0u,
        client != nullptr ? client->GetStationID() : 0u);
    std::fflush(stderr);
    uint32 stationID = client->GetStationID();
    if (!IsStationID(stationID)) {
        sLog.Green("CQ", "[CQ] captainsQuartersSvc.BindObject: char=%u (%s) stationID=%u invalid -> nullptr",
            client->GetCharacterID(), client->GetName(), stationID);
        return nullptr;
    }

    uint32 worldSpaceID = sCQMgr.EnsureWorldSpaceForStation(stationID);
    auto it = m_instances.find(worldSpaceID);
    if (it != m_instances.end()) {
        sLog.Green("CQ", "[CQ] captainsQuartersSvc.BindObject: char=%u (%s) station=%u worldSpace=%u -> reuse bound",
            client->GetCharacterID(), client->GetName(), stationID, worldSpaceID);
        return it->second;
    }

    CQBound* bound = new CQBound(GetServiceManager(), *this, worldSpaceID);
    m_instances.insert_or_assign(worldSpaceID, bound);
    sLog.Green("CQ", "[CQ] captainsQuartersSvc.BindObject: char=%u (%s) station=%u worldSpace=%u -> new CQBound",
        client->GetCharacterID(), client->GetName(), stationID, worldSpaceID);
    return bound;
}

void CQService::BoundReleased(CQBound* bound) {
    const uint32 ws = bound->GetWorldSpaceID();
    auto it = m_instances.find(ws);
    if (it != m_instances.end()) {
        m_instances.erase(it);
        sLog.Green("CQ", "[CQ] captainsQuartersSvc.BoundReleased: worldSpace=%u (bound removed)", ws);
    } else {
        sLog.Green("CQ", "[CQ] captainsQuartersSvc.BoundReleased: worldSpace=%u (already gone)", ws);
    }
}

CQBound::CQBound(EVEServiceManager& mgr, CQService& parent, uint32 worldSpaceID)
: EVEBoundObject(mgr, parent),
  m_worldSpaceID(worldSpaceID) {
    this->Add("JoinSharedQuarters", &CQBound::JoinSharedQuarters);
    this->Add("LeaveSharedQuarters", &CQBound::LeaveSharedQuarters);
    this->Add("GetSnapshot", &CQBound::GetSnapshot);
    this->Add("UpdateTransform", &CQBound::UpdateTransform);
}

PyResult CQBound::JoinSharedQuarters(PyCallArgs& call) {
    const uint32 stationID = call.client->GetStationID();
    sLog.Green("CQ", "[CQ] JoinSharedQuarters: char=%u (%s) station=%u boundWorldSpace=%u",
        call.client->GetCharacterID(), call.client->GetName(), stationID, m_worldSpaceID);

    uint32 ws = sCQMgr.Join(call.client, stationID);

    PyList* snap = sCQMgr.BuildSnapshot(ws, call.client->GetCharacterID());
    sLog.Green("CQ", "[CQ] JoinSharedQuarters: cqInstance=%u station=%u snapshotOthers=%zu (session.worldspaceid unchanged)",
        ws, stationID, snap->size());

    PyTuple* rsp = new PyTuple(2);
    rsp->SetItem(0, new PyInt(ws));
    rsp->SetItem(1, snap);
    return rsp;
}

PyResult CQBound::LeaveSharedQuarters(PyCallArgs& call) {
    sLog.Green("CQ", "[CQ] LeaveSharedQuarters: char=%u (%s) station=%u boundWorldSpace=%u",
        call.client->GetCharacterID(), call.client->GetName(), call.client->GetStationID(), m_worldSpaceID);
    sCQMgr.Leave(call.client);
    call.client->UpdateSessionInt("worldspaceid", call.client->GetStationID());
    call.client->SendSessionChange();
    return PyStatic.NewNone();
}

PyResult CQBound::GetSnapshot(PyCallArgs& call) {
    sLog.Green("CQ", "[CQ] GetSnapshot: char=%u (%s) worldSpace=%u",
        call.client->GetCharacterID(), call.client->GetName(), m_worldSpaceID);
    return sCQMgr.BuildSnapshot(m_worldSpaceID, call.client->GetCharacterID());
}

PyResult CQBound::UpdateTransform(PyCallArgs& call, PyFloat* x, PyFloat* y, PyFloat* z) {
    sLog.Green("CQ", "[CQ] UpdateTransform: char=%u (%s) worldSpace(bound)=%u pos=(%.4f,%.4f,%.4f)",
        call.client->GetCharacterID(), call.client->GetName(), m_worldSpaceID,
        x->value(), y->value(), z->value());
    sCQMgr.UpdatePosition(call.client, GPoint(x->value(), y->value(), z->value()));
    return PyStatic.NewNone();
}
