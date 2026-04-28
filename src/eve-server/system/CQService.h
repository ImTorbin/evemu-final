#ifndef __EVE_SYSTEM_CQ_SERVICE_H_
#define __EVE_SYSTEM_CQ_SERVICE_H_

#include "services/BoundService.h"

class CQBound;

class CQService : public BindableService<CQService, CQBound> {
public:
    CQService(EVEServiceManager& mgr);
    void BoundReleased(CQBound* bound) override;

protected:
    BoundDispatcher* BindObject(Client* client, PyRep* bindParameters) override;

private:
    std::map<uint32, CQBound*> m_instances;
};

class CQBound : public EVEBoundObject<CQBound> {
public:
    CQBound(EVEServiceManager& mgr, CQService& parent, uint32 worldSpaceID);
    uint32 GetWorldSpaceID() const { return m_worldSpaceID; }

protected:
    PyResult JoinSharedQuarters(PyCallArgs& call);
    PyResult LeaveSharedQuarters(PyCallArgs& call);
    PyResult GetSnapshot(PyCallArgs& call);
    PyResult UpdateTransform(PyCallArgs& call, PyFloat* x, PyFloat* y, PyFloat* z, PyFloat* yaw);
    PyResult SetActionState(PyCallArgs& call, PyInt* actionObjectUID, PyInt* actionStationIdx);
    PyResult PlayEmote(PyCallArgs& call, PyInt* emoteIndex);
    /// Dev / GMH+QA+programmer: register a CQ static agent at your last streamed position (uses captainsQuartersSvc bound session).
    PyResult CQDebugRegisterAgentHere(PyCallArgs& call, PyInt* appearanceCharID);
    PyResult CQDebugUpdateAgentHere(PyCallArgs& call, PyInt* instanceCharID);
    PyResult CQDebugSetAgentTransform(PyCallArgs& call, PyInt* instanceCharID, PyFloat* x, PyFloat* y, PyFloat* z, PyFloat* yaw);
    PyResult CQDebugDeleteAgent(PyCallArgs& call, PyInt* instanceCharID);

private:
    uint32 m_worldSpaceID;
};

#endif
