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
    PyResult UpdateTransform(PyCallArgs& call, PyFloat* x, PyFloat* y, PyFloat* z);

private:
    uint32 m_worldSpaceID;
};

#endif
