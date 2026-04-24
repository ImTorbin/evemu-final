
#ifndef _EVE_SHIP_MODULES_JUMP_PORTAL_MODULE_H_
#define _EVE_SHIP_MODULES_JUMP_PORTAL_MODULE_H_

#include "ship/modules/ActiveModule.h"

class JumpPortalModule : public ActiveModule
{
public:
    JumpPortalModule(ModuleItemRef mRef, ShipItemRef sRef);

    virtual void Activate(uint16 effectID, uint32 targetID = 0, int16 repeat = 0);
    virtual void DeactivateCycle(bool abort = false);
    virtual bool CanActivate();

private:
    Client* m_pClient;
    bool m_requireCovert;
};

#endif /* _EVE_SHIP_MODULES_JUMP_PORTAL_MODULE_H_ */
