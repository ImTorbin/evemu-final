#include "eve-server.h"

#include <cstdio>

#include "EVE_Roles.h"
#include "ServiceDB.h"
#include "services/ServiceManager.h"
#include "system/CQManager.h"
#include "system/CQService.h"

namespace {
bool CqDebugRoleOk(Client* c) {
    if (c == nullptr) {
        return false;
    }
    const int64_t r = c->GetAccountRole();
    return (r & (int64_t(Acct::Role::GMH) | int64_t(Acct::Role::QA) | int64_t(Acct::Role::PROGRAMMER))) != 0;
}
} // namespace

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
    this->Add("SetActionState", &CQBound::SetActionState);
    this->Add("PlayEmote", &CQBound::PlayEmote);
    this->Add("CQDebugRegisterAgentHere", &CQBound::CQDebugRegisterAgentHere);
    this->Add("CQDebugUpdateAgentHere", &CQBound::CQDebugUpdateAgentHere);
    this->Add("CQDebugSetAgentTransform", &CQBound::CQDebugSetAgentTransform);
    this->Add("CQDebugDeleteAgent", &CQBound::CQDebugDeleteAgent);
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

PyResult CQBound::UpdateTransform(PyCallArgs& call, PyFloat* x, PyFloat* y, PyFloat* z, PyFloat* yaw) {
    const float yawRad = static_cast<float>(yaw->value());
    sLog.Green("CQ", "[CQ] UpdateTransform: char=%u (%s) worldSpace(bound)=%u pos=(%.4f,%.4f,%.4f) yaw=%.3f",
        call.client->GetCharacterID(), call.client->GetName(), m_worldSpaceID,
        x->value(), y->value(), z->value(), yawRad);
    sCQMgr.UpdateTransform(call.client, GPoint(x->value(), y->value(), z->value()), yawRad);
    return PyStatic.NewNone();
}

PyResult CQBound::SetActionState(PyCallArgs& call, PyInt* actionObjectUID, PyInt* actionStationIdx) {
    const uint32 uid = static_cast<uint32>(actionObjectUID->value());
    const int32  idx = static_cast<int32>(actionStationIdx->value());
    sLog.Green("CQ", "[CQ] SetActionState: char=%u (%s) worldSpace(bound)=%u aoUID=%u stationIdx=%d",
        call.client->GetCharacterID(), call.client->GetName(), m_worldSpaceID, uid, idx);
    sCQMgr.SetActionState(call.client, uid, idx);
    return PyStatic.NewNone();
}

namespace {
    // Must match client LiveUpdate CQ_EMOTE_MENU length (phase1_cq.py).
    constexpr int32 kMaxBroadcastEmote = 8;
}

PyResult CQBound::CQDebugRegisterAgentHere(PyCallArgs& call, PyInt* appearanceCharID) {
    if (!CqDebugRoleOk(call.client)) {
        return PyStatic.NewNone();
    }
    if (call.client->GetStationID() != sCQMgr.GetStationForWorldSpace(m_worldSpaceID)) {
        return PyStatic.NewNone();
    }
    const CQOccupantState* st = sCQMgr.GetOccupantStateForChar(call.client->GetCharacterID());
    if (st == nullptr) {
        sLog.Warning("CQ", "[CQ] CQDebugRegisterAgentHere: char=%u not in shared CQ", call.client->GetCharacterID());
        return PyStatic.NewNone();
    }
    std::string label = "cqagent";
    if (call.tuple != nullptr && call.tuple->size() > 1) {
        label = PyRep::StringContent(call.tuple->GetItem(1));
        if (label.empty()) {
            label = "cqagent";
        }
    }
    uint32 outInst = 0;
    if (!ServiceDB::InsertCQCustomAgent(
            call.client->GetStationID(),
            static_cast<uint32>(appearanceCharID->value()),
            st->position.x, st->position.y, st->position.z, st->yaw,
            label, outInst)) {
        return PyStatic.NewNone();
    }
    CQCustomAgentRow row;
    if (ServiceDB::GetCQCustomAgentByInstance(outInst, row)) {
        sCQMgr.BroadcastCustomAgentMove(m_worldSpaceID, row);
        return new PyInt(row.missionAgentID != 0 ? row.missionAgentID : outInst);
    }
    return new PyInt(outInst);
}

PyResult CQBound::CQDebugUpdateAgentHere(PyCallArgs& call, PyInt* instanceCharID) {
    if (!CqDebugRoleOk(call.client)) {
        return PyStatic.NewNone();
    }
    uint32 station = 0;
    if (!ServiceDB::GetCQCustomAgentStationID(static_cast<uint32>(instanceCharID->value()), station)
        || station != call.client->GetStationID()
        || station != sCQMgr.GetStationForWorldSpace(m_worldSpaceID)) {
        return PyStatic.NewNone();
    }
    const CQOccupantState* st = sCQMgr.GetOccupantStateForChar(call.client->GetCharacterID());
    if (st == nullptr) {
        return PyStatic.NewNone();
    }
    uint32 sta = 0;
    if (!ServiceDB::GetCQTableAgentIdFromProtocolId(static_cast<uint32>(instanceCharID->value()), sta)) {
        return PyStatic.NewNone();
    }
    if (!ServiceDB::UpdateCQCustomAgentTransformByInstance(
            static_cast<uint32>(instanceCharID->value()),
            st->position.x, st->position.y, st->position.z, st->yaw)) {
        return PyStatic.NewNone();
    }
    CQCustomAgentRow row;
    if (ServiceDB::GetCQCustomAgentByTableId(sta, row)) {
        sCQMgr.BroadcastCustomAgentMove(m_worldSpaceID, row);
    }
    return new PyInt(1);
}

PyResult CQBound::CQDebugSetAgentTransform(PyCallArgs& call, PyInt* instanceCharID, PyFloat* x, PyFloat* y, PyFloat* z, PyFloat* yaw) {
    if (!CqDebugRoleOk(call.client)) {
        return PyStatic.NewNone();
    }
    uint32 station = 0;
    if (!ServiceDB::GetCQCustomAgentStationID(static_cast<uint32>(instanceCharID->value()), station)
        || station != call.client->GetStationID()
        || station != sCQMgr.GetStationForWorldSpace(m_worldSpaceID)) {
        return PyStatic.NewNone();
    }
    uint32 sta = 0;
    if (!ServiceDB::GetCQTableAgentIdFromProtocolId(static_cast<uint32>(instanceCharID->value()), sta)) {
        return PyStatic.NewNone();
    }
    if (!ServiceDB::UpdateCQCustomAgentTransformByInstance(
            static_cast<uint32>(instanceCharID->value()),
            x->value(), y->value(), z->value(), static_cast<float>(yaw->value()))) {
        return PyStatic.NewNone();
    }
    CQCustomAgentRow row;
    if (ServiceDB::GetCQCustomAgentByTableId(sta, row)) {
        sCQMgr.BroadcastCustomAgentMove(m_worldSpaceID, row);
    }
    return new PyInt(1);
}

PyResult CQBound::CQDebugDeleteAgent(PyCallArgs& call, PyInt* instanceCharID) {
    if (!CqDebugRoleOk(call.client)) {
        return PyStatic.NewNone();
    }
    const uint32 inst = static_cast<uint32>(instanceCharID->value());
    uint32 station = 0;
    if (!ServiceDB::GetCQCustomAgentStationID(inst, station)
        || station != call.client->GetStationID()
        || station != sCQMgr.GetStationForWorldSpace(m_worldSpaceID)) {
        return PyStatic.NewNone();
    }
    uint32 sta = 0;
    if (!ServiceDB::GetCQTableAgentIdFromProtocolId(inst, sta)) {
        return PyStatic.NewNone();
    }
    CQCustomAgentRow row;
    if (!ServiceDB::GetCQCustomAgentByTableId(sta, row)) {
        return PyStatic.NewNone();
    }
    const uint32 clientKey = (row.missionAgentID != 0) ? row.missionAgentID : row.instanceCharID;
    if (!ServiceDB::DeleteCQCustomAgentByInstance(inst)) {
        return PyStatic.NewNone();
    }
    sCQMgr.BroadcastCustomAgentGone(m_worldSpaceID, clientKey);
    return new PyInt(1);
}

PyResult CQBound::PlayEmote(PyCallArgs& call, PyInt* emoteIndex) {
    const int32 idx = static_cast<int32>(emoteIndex->value());
    if (idx < 0 || idx >= kMaxBroadcastEmote) {
        sLog.Green("CQ", "[CQ] PlayEmote: char=%u (%s) invalid emoteIndex=%d (ignored)",
            call.client->GetCharacterID(), call.client->GetName(), idx);
        return PyStatic.NewNone();
    }
    sLog.Green("CQ", "[CQ] PlayEmote: char=%u (%s) worldSpace(bound)=%u idx=%d",
        call.client->GetCharacterID(), call.client->GetName(), m_worldSpaceID, idx);
    sCQMgr.BroadcastEmote(m_worldSpaceID, call.client->GetCharacterID(), idx);
    return PyStatic.NewNone();
}
